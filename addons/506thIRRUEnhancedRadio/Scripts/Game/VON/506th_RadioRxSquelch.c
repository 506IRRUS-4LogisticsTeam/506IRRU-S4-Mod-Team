//! Per-frequency receive-side squelch state, one channel per frequency.
//! Fed by two sources: authoritative key-state RPCs relayed through
//! IRRU_RFPropagationNetworkComponent (instant open/close, dead keys audible)
//! and the voice packet stream from SCR_VoNComponent (fallback for senders
//! that never sent a key RPC, e.g. other mods or GM transmissions).
//! A keyed channel stays open through dead air; a voice-only channel closes
//! by silence timeout via Tick().
class IRRU_RxChannelState
{
    //! Sender id -> last key-start time; only senders whose key-start passed
    //! this receiver's tuning/reachability gates are entered, so their stops
    //! are the only ones that can release the channel.
    ref map<int, float> m_mKeyedSenders = new map<int, float>();
    bool m_bVoiceActive;
    float m_fLastVoiceMs;
    bool m_bOpen;
    float m_fClosedAtMs;
    float m_fRpcClosedAtMs;
}

//! Latest crypto fill hash announced by a sender's key-up, kept OUTSIDE the
//! per-frequency squelch channel state and written before any tuning or
//! reachability gate: a receiver who tunes in, powers a radio on, walks into
//! range or joins mid-transmission must still be able to verdict crypto, or
//! encrypted traffic would play in the clear through normal gameplay.
class IRRU_SenderKeyInfo
{
    int m_iFrequency;
    int m_iFillHash;
    float m_fStampedMs;
}

class IRRU_RadioRxSquelch
{
    //! Voice capture does silence detection, so the timeout must ride through
    //! natural speech pauses; the grace keeps borderline gaps from replaying
    //! the open beep. MAX_KEY_HOLD is a failsafe against a lost key-stop RPC.
    protected static const float SILENCE_TIMEOUT_MS = 600;
    protected static const float REOPEN_GRACE_MS = 500;
    protected static const float MAX_KEY_HOLD_MS = 120000;
    protected static const float MIN_SIGNAL_QUALITY = 0.05;
    //! Sender-key entries have their own failsafe lifetime, deliberately far
    //! above MAX_KEY_HOLD: the squelch's 2-minute stuck-key expiry must never
    //! delete the hash of a still-transmitting sender mid-stream. Entries are
    //! normally removed by key-stop or the server-relayed disconnect stop.
    protected static const float IRRU_SENDER_KEY_TTL_MS = 600000;
    //! Voice frames still in flight behind the reliable key-stop RPC must not
    //! reopen the channel (that would earn a second close beep from Tick);
    //! kept below REOPEN_GRACE_MS so a genuine still-talking voice-only sender
    //! resumes silently right after the window.
    protected static const float VOICE_TAIL_DISCARD_MS = 400;

    private static ref IRRU_RadioRxSquelch s_Instance;

    protected ref map<int, ref IRRU_RxChannelState> m_mChannels = new map<int, ref IRRU_RxChannelState>();

    //! Frequency whose values an opening voice packet last wrote into the
    //! global audio variables. The engine reads them in that same frame, so a
    //! beep borrowing the slots in between must put these back.
    protected int m_iAudioSlotFrequency = -1;
    protected float m_fAudioSlotStartedAtMs = -1;
    //! Whether the stream that started the slot this frame carried an
    //! ENCRYPTED crypto verdict; an intelligible same-frame START yields to
    //! it (fail closed) instead of overwriting it with cleartext values.
    protected bool m_bAudioSlotEncrypted = false;

    protected ref map<int, ref IRRU_SenderKeyInfo> m_mSenderKeys = new map<int, ref IRRU_SenderKeyInfo>();

    //------------------------------------------------------------------------------------------------
    static IRRU_RadioRxSquelch GetInstance()
    {
        if (!s_Instance)
            s_Instance = new IRRU_RadioRxSquelch();

        return s_Instance;
    }

    //------------------------------------------------------------------------------------------------
    //! Remote player key state relayed by the server. Key-start is gated by
    //! frequency tuning and reachability so squelch mirrors what the voice
    //! path could actually deliver; key-stop is always processed so counts
    //! cannot wedge when the receiver moved out of range mid-transmission.
    void OnRemoteKeyState(int senderPlayerId, int frequency, float range, bool keyed, vector senderPos, int fillHash)
    {
        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
            return;

        if (playerController.GetPlayerId() == senderPlayerId)
            return;

        float nowMs = GetGame().GetWorld().GetWorldTime();

        // Sender-key bookkeeping runs UNCONDITIONALLY, before every gate
        // below: the gates only decide squelch beeps, never crypto knowledge.
        // The hash argument is meaningful only on key-starts; stops just
        // clean up (their hash is always 0 and must be ignored).
        if (keyed)
            IRRU_StoreSenderKey(senderPlayerId, frequency, fillHash, nowMs, playerController);
        else
            m_mSenderKeys.Remove(senderPlayerId);

        if (keyed)
        {
            BaseTransceiver transceiver = FindTunedTransceiver(frequency);
            if (!transceiver)
                return;

            if (!IsReachable(frequency, range, senderPos, playerController))
                return;

            IRRU_RxChannelState state = GetOrCreateState(frequency);
            ExpireStuckKeys(state, nowMs);
            state.m_mKeyedSenders.Set(senderPlayerId, nowMs);
            Open(state, transceiver, nowMs);

            // Encrypted senders: load the scramble values into the audio
            // slots now, ahead of the sound event the incoming voice will
            // spawn (see IRRU_PreArmEncryptedSlots for the probe evidence)
            SCR_VoNComponent von = SCR_VoNComponent.Cast(playerController.FindComponent(SCR_VoNComponent));
            if (von)
                von.IRRU_PreArmEncryptedSlots(senderPlayerId, frequency, transceiver);
        }
        else
        {
            IRRU_RxChannelState state;
            if (!m_mChannels.Find(frequency, state))
                return;

            // Only honor stops whose start was accepted for this receiver, so a
            // filtered-out sender's release cannot close a channel someone else
            // is still keying.
            if (!state.m_mKeyedSenders.Contains(senderPlayerId))
                return;

            state.m_mKeyedSenders.Remove(senderPlayerId);

            // Voice stops with the key, so close immediately instead of waiting
            // out the silence timeout; the tail-discard window swallows voice
            // frames that were still in flight behind this RPC.
            if (state.m_mKeyedSenders.Count() == 0)
            {
                state.m_bVoiceActive = false;
                state.m_fRpcClosedAtMs = nowMs;
                Close(state, frequency, nowMs);
            }
        }
    }

    //------------------------------------------------------------------------------------------------
    //! Voice packet on a tuned radio; the caller filters out own transmissions.
    void OnVoicePacket(int frequency, BaseTransceiver receiver)
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();

        IRRU_RxChannelState state = GetOrCreateState(frequency);

        if (nowMs - state.m_fRpcClosedAtMs < VOICE_TAIL_DISCARD_MS)
            return;

        state.m_bVoiceActive = true;
        state.m_fLastVoiceMs = nowMs;
        Open(state, receiver, nowMs);
    }

    //------------------------------------------------------------------------------------------------
    //! Track the latest fill hash a sender announced; runs before the squelch
    //! gates (see OnRemoteKeyState). Also nudges the VoN component: voice can
    //! outrun this RPC, and a stream opened with a wrong assumed verdict gets
    //! re-verdicted on its next packet instead of leaking for its lifetime.
    protected void IRRU_StoreSenderKey(int senderPlayerId, int frequency, int fillHash, float nowMs, PlayerController playerController)
    {
        IRRU_SenderKeyInfo info;
        if (!m_mSenderKeys.Find(senderPlayerId, info))
        {
            info = new IRRU_SenderKeyInfo();
            m_mSenderKeys.Set(senderPlayerId, info);
        }

        info.m_iFrequency = frequency;
        info.m_iFillHash = fillHash;
        info.m_fStampedMs = nowMs;

        // ===== IRRU_CPROBE (temporary diagnostic - remove after crypto timing investigation) =====
        Print(string.Format("[IRRU_CPROBE] KeyStart: sender=%1 freq=%2 hash=%3 t=%4",
            senderPlayerId, frequency, fillHash, nowMs), LogLevel.WARNING);
        // ===== /IRRU_CPROBE =====

        SCR_VoNComponent von = SCR_VoNComponent.Cast(playerController.FindComponent(SCR_VoNComponent));
        if (von)
            von.IRRU_OnSenderKeyInfo(senderPlayerId, frequency);
    }

    //------------------------------------------------------------------------------------------------
    //! Fill hash the sender announced for its current transmission.
    //! 0 = unknown or plaintext - both deliberately fail open (intelligible).
    int GetSenderFillHash(int senderPlayerId, int frequency)
    {
        IRRU_SenderKeyInfo info;
        if (!m_mSenderKeys.Find(senderPlayerId, info))
            return 0;

        if (GetGame().GetWorld().GetWorldTime() - info.m_fStampedMs > IRRU_SENDER_KEY_TTL_MS)
        {
            m_mSenderKeys.Remove(senderPlayerId);
            return 0;
        }

        if (info.m_iFrequency != frequency)
            return 0;

        return info.m_iFillHash;
    }

    //------------------------------------------------------------------------------------------------
    //! \param eventStart true when the engine certainly starts a sound event
    //! on the packet that wrote, so the values are about to be read
    //! \param encrypted whether the writing stream carried an ENCRYPTED verdict
    void OnAudioSlotWritten(int frequency, bool eventStart, bool encrypted = false)
    {
        m_iAudioSlotFrequency = frequency;
        if (eventStart)
        {
            m_fAudioSlotStartedAtMs = GetGame().GetWorld().GetWorldTime();
            m_bAudioSlotEncrypted = encrypted;
        }
    }

    //------------------------------------------------------------------------------------------------
    //! Whether a stream wrote the slots for a certain event start in the
    //! current frame; world time is constant within a frame.
    bool WasAudioSlotStartedThisFrame()
    {
        return GetGame().GetWorld().GetWorldTime() - m_fAudioSlotStartedAtMs < 1.0;
    }

    //------------------------------------------------------------------------------------------------
    //! Whether this frame's slot-starting stream was ENCRYPTED. Same-frame
    //! collisions are last-writer-wins for BOTH sound events, so an
    //! intelligible START yields to an encrypted one (a briefly scrambled
    //! friendly stream is recoverable; leaked crypto audio is not).
    bool WasAudioSlotEncryptedThisFrame()
    {
        if (!WasAudioSlotStartedThisFrame())
            return false;

        return m_bAudioSlotEncrypted;
    }

    //------------------------------------------------------------------------------------------------
    //! Put the last opening stream's channel values back into the audio
    //! variables after a one-shot (beep) borrowed them.
    void RestoreAudioSlot()
    {
        if (m_iAudioSlotFrequency == -1)
            return;

        BaseTransceiver transceiver = FindTunedTransceiver(m_iAudioSlotFrequency);
        if (!transceiver)
            return;

        IRRU_RadioBeepHelper.ApplyChannelAudioVariables(transceiver);
    }

    //------------------------------------------------------------------------------------------------
    //! Advance timeouts; close voice-silent channels and drop idle state.
    //! \return true while any channel still needs ticking
    bool Tick(float nowMs)
    {
        array<int> idle = {};
        foreach (int frequency, IRRU_RxChannelState state : m_mChannels)
        {
            ExpireStuckKeys(state, nowMs);

            if (state.m_bVoiceActive && nowMs - state.m_fLastVoiceMs > SILENCE_TIMEOUT_MS)
                state.m_bVoiceActive = false;

            if (state.m_bOpen && state.m_mKeyedSenders.Count() == 0 && !state.m_bVoiceActive)
                Close(state, frequency, nowMs);

            if (!state.m_bOpen && state.m_mKeyedSenders.Count() == 0 && !state.m_bVoiceActive && nowMs - state.m_fClosedAtMs > REOPEN_GRACE_MS)
                idle.Insert(frequency);
        }

        foreach (int frequency : idle)
            m_mChannels.Remove(frequency);

        return m_mChannels.Count() > 0;
    }

    //------------------------------------------------------------------------------------------------
    protected void Open(IRRU_RxChannelState state, BaseTransceiver transceiver, float nowMs)
    {
        if (state.m_bOpen)
            return;

        state.m_bOpen = true;

        if (nowMs - state.m_fClosedAtMs < REOPEN_GRACE_MS)
            return;

        IRRU_RadioBeepHelper.PlayRxOpen(transceiver);
    }

    //------------------------------------------------------------------------------------------------
    protected void Close(IRRU_RxChannelState state, int frequency, float nowMs)
    {
        if (!state.m_bOpen)
            return;

        state.m_bOpen = false;
        state.m_fClosedAtMs = nowMs;

        BaseTransceiver transceiver = FindTunedTransceiver(frequency);
        if (transceiver)
            IRRU_RadioBeepHelper.PlayRxClose(transceiver);
    }

    //------------------------------------------------------------------------------------------------
    protected void ExpireStuckKeys(IRRU_RxChannelState state, float nowMs)
    {
        if (state.m_mKeyedSenders.Count() == 0)
            return;

        array<int> stale = {};
        foreach (int senderId, float lastKeyMs : state.m_mKeyedSenders)
        {
            if (nowMs - lastKeyMs > MAX_KEY_HOLD_MS)
                stale.Insert(senderId);
        }

        foreach (int senderId : stale)
            state.m_mKeyedSenders.Remove(senderId);
    }

    //------------------------------------------------------------------------------------------------
    protected IRRU_RxChannelState GetOrCreateState(int frequency)
    {
        IRRU_RxChannelState state;
        if (m_mChannels.Find(frequency, state))
            return state;

        state = new IRRU_RxChannelState();
        m_mChannels.Set(frequency, state);
        return state;
    }

    //------------------------------------------------------------------------------------------------
    protected SCR_VONController GetLocalVonController()
    {
        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
            return null;

        return SCR_VONController.Cast(playerController.FindComponent(SCR_VONController));
    }

    //------------------------------------------------------------------------------------------------
    protected BaseTransceiver FindTunedTransceiver(int frequency)
    {
        SCR_VONController vonController = GetLocalVonController();
        if (!vonController)
            return null;

        // The engine only delivers voice to powered radios; mirror that gate.
        // The powered-preferring lookup matters with two radios on one
        // frequency: the first-listed being switched off must not hide the
        // powered second (that would drop key-starts while voice still flows).
        SCR_VONEntryRadio entry = vonController.IRRU_FindPoweredEntryByFrequency(frequency);
        if (!entry)
            return null;

        return entry.GetTransceiver();
    }

    //------------------------------------------------------------------------------------------------
    protected bool IsReachable(int frequency, float range, vector senderPos, PlayerController playerController)
    {
        if (senderPos == vector.Zero || range <= 0)
            return true;

        IEntity myEntity = playerController.GetControlledEntity();
        if (!myEntity)
            return true;

        vector myPos = myEntity.GetOrigin();
        if (vector.Distance(senderPos, myPos) > range)
            return false;

        if (IRRU_RFPropagationNetworkComponent.IsRFPropagationEnabled())
        {
            float quality = IRRU_RFPropagationModel.GetInstance().CalculateSignalQuality(senderPos, myPos, frequency);
            if (quality < MIN_SIGNAL_QUALITY)
                return false;
        }

        return true;
    }
}
