//! Per-sender receive state: stream timing plus cached RF results (the
//! terrain walk behind signal quality is far too heavy to repeat per packet).
class IRRU_SenderStream
{
    float m_fLastPacketMs;
    float m_fSignalQuality;
    float m_fJamStrength;
    float m_fComputedAtMs;
    int m_iFrequency;
    //! Crypto verdict written for this stream's current sound event; compared
    //! against fresh key-state broadcasts to catch voice-outran-the-RPC races
    bool m_bEncryptedForMe;
    //! Whether the stream's packets carry the editor (GM) sender flag; the
    //! race-correction path must re-verdict with the SAME flag or a GM whose
    //! announced hash mismatches would get its stream restarted forever
    bool m_bSenderEditor;
}

enum IRRU_EStreamOpening
{
    NONE,
    //! Resume inside the verified keep-alive window: the engine may not
    //! actually restart the sound event on this packet
    RESUME,
    //! First packet, frequency change or long gap: the engine starts (or
    //! restarts) a sound event on this packet
    START
}

modded class SCR_VoNComponent : VoNComponent
{
    //! Voice packets feed IRRU_RadioRxSquelch as the fallback squelch trigger
    //! (key-state RPCs are the primary); this component only hosts the timeout
    //! ticker, which is all the state machine needs while voice is flowing.
    protected static const int IRRU_RX_WATCHDOG_TICK_MS = 150;
    protected static const float IRRU_SIGNAL_REFRESH_MS = 500;
    //! Packets arrive every ~20-60ms; any gap above this is treated as a
    //! possible restart (a PTT release + re-key inside the keep-alive window
    //! is unprobed, so it stays well below it).
    protected static const float IRRU_STREAM_GAP_MS = 400;
    //! In-game probe: one sound event survived a 0.8s mid-key silence and
    //! restarted after 4.9s, so a resume below this is known not to restart.
    protected static const float IRRU_VERIFIED_KEEPALIVE_MS = 800;

    protected bool m_bIRRU_RxWatchdogRunning = false;
    protected ref map<int, ref IRRU_SenderStream> m_mIRRU_Streams = new map<int, ref IRRU_SenderStream>();

    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {
        // DO NOT TOUCH DIRECT VOICE PACKETS, IT WILL MESS UP THE STATE MACHINE - RADIO TRANSMISSIONS ONLY
        if (receiver)
        {
            PlayerController playerController = GetGame().GetPlayerController();
            if (!playerController || playerController.GetPlayerId() != playerId)
            {
                IRRU_TrackIncomingTransmission(receiver, frequency);

                // The audio variables are single global slots, but the engine
                // reads them once per sound event, in the frame of the packet
                // that starts it, and restarts the event after a silence gap
                // (verified in-game: later writes are ignored). So each stream
                // writes its own values only on its opening/resume packet -
                // steady-state writes would just race other streams' opening
                // writes in the same frame.
                IRRU_EStreamOpening opening = IRRU_GetStreamOpening(playerId, frequency);
                if (opening != IRRU_EStreamOpening.NONE)
                    IRRU_ApplyAudioVariables(playerId, receiver, frequency, playerController, opening == IRRU_EStreamOpening.START, isSenderEditor);
            }
        }

        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
    }

    protected void IRRU_TrackIncomingTransmission(BaseTransceiver receiver, int frequency)
    {
        IRRU_RadioRxSquelch.GetInstance().OnVoicePacket(frequency, receiver);

        if (!m_bIRRU_RxWatchdogRunning)
        {
            m_bIRRU_RxWatchdogRunning = true;
            GetGame().GetCallqueue().CallLater(IRRU_RxWatchdogTick, IRRU_RX_WATCHDOG_TICK_MS, false);
        }
    }

    protected void IRRU_RxWatchdogTick()
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();

        if (IRRU_RadioRxSquelch.GetInstance().Tick(nowMs))
            GetGame().GetCallqueue().CallLater(IRRU_RxWatchdogTick, IRRU_RX_WATCHDOG_TICK_MS, false);
        else
            m_bIRRU_RxWatchdogRunning = false;
    }

    //! Classifies a packet by whether the engine may start a sound event on it.
    //! A resume known not to restart the event yields to a genuine start that
    //! another stream already wrote this frame, instead of overwriting it
    //! before the engine reads it.
    protected IRRU_EStreamOpening IRRU_GetStreamOpening(int senderPlayerId, int frequency)
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();
        IRRU_SenderStream stream = IRRU_GetSenderStream(senderPlayerId);
        float gapMs = nowMs - stream.m_fLastPacketMs;
        stream.m_fLastPacketMs = nowMs;

        if (stream.m_iFrequency != frequency || gapMs > IRRU_VERIFIED_KEEPALIVE_MS)
            return IRRU_EStreamOpening.START;

        if (gapMs <= IRRU_STREAM_GAP_MS)
            return IRRU_EStreamOpening.NONE;

        if (IRRU_RadioRxSquelch.GetInstance().WasAudioSlotStartedThisFrame())
            return IRRU_EStreamOpening.NONE;

        return IRRU_EStreamOpening.RESUME;
    }

    //! Below this floor the SignalQuality sig attenuates the WHOLE radio
    //! sound (jam noise included -> silence, not scramble); 0.15 keeps
    //! encrypted noise present but still range-scaled, so it never plays
    //! louder than legitimate marginal-range speech would.
    protected static const float IRRU_ENCRYPTED_MIN_QUALITY = 0.15;

    protected void IRRU_ApplyAudioVariables(int senderPlayerId, BaseTransceiver receiver, int frequency, PlayerController playerController, bool eventStart, bool isSenderEditor)
    {
        bool encrypted = IRRU_IsStreamEncrypted(senderPlayerId, frequency, isSenderEditor);
        IRRU_SenderStream verdictStream = IRRU_GetSenderStream(senderPlayerId);
        verdictStream.m_bEncryptedForMe = encrypted;
        verdictStream.m_bSenderEditor = isSenderEditor;

        // ===== IRRU_CPROBE (temporary diagnostic - remove after crypto timing investigation) =====
        Print(string.Format("[IRRU_CPROBE] StreamOpen: sender=%1 freq=%2 start=%3 senderHash=%4 myHash=%5 verdict=%6 t=%7",
            senderPlayerId, frequency, eventStart,
            IRRU_RadioRxSquelch.GetInstance().GetSenderFillHash(senderPlayerId, frequency),
            SCR_IRRURadioEarSettings.GetInstance().GetFillHash(frequency),
            encrypted, GetGame().GetWorld().GetWorldTime()), LogLevel.WARNING);
        // ===== /IRRU_CPROBE =====

        // Same-frame START collisions are last-writer-wins for BOTH sound
        // events. An intelligible stream yields to an already-written
        // encrypted one (fail closed): briefly scrambling friendly traffic is
        // recoverable, leaking encrypted traffic in the clear is not.
        if (!encrypted && eventStart && IRRU_RadioRxSquelch.GetInstance().WasAudioSlotEncryptedThisFrame())
            return;

        IRRU_RadioBeepHelper.ApplyChannelAudioVariables(receiver);
        IRRU_RadioRxSquelch.GetInstance().OnAudioSlotWritten(frequency, eventStart, encrypted);

        vector receiverPos = vector.Zero;
        if (playerController)
        {
            IEntity receiverEntity = playerController.GetControlledEntity();
            if (receiverEntity)
                receiverPos = receiverEntity.GetOrigin();
        }

        IRRU_SenderStream stream = IRRU_GetSenderSignals(senderPlayerId, frequency, receiverPos);

        if (encrypted)
        {
            // JamStrength 0 = maximum jam: the graph fully mutes the voice
            // bus and drives the synthesized radio-band noise at full - the
            // jammer path IS the scramble sound (see ENCRYPTION_DESIGN.md)
            AudioSystem.SetVariableByName("SignalQuality", Math.Max(stream.m_fSignalQuality, IRRU_ENCRYPTED_MIN_QUALITY), IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
            AudioSystem.SetVariableByName("JamStrength", 0, IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
        }
        else
        {
            AudioSystem.SetVariableByName("SignalQuality", stream.m_fSignalQuality, IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
            AudioSystem.SetVariableByName("JamStrength", stream.m_fJamStrength, IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
        }
    }

    //! ENCRYPTED = the sender announced a fill for this frequency and it does
    //! not match ours. Every ambiguity fails open (intelligible): plaintext
    //! senders, senders with no key RPC, GM traffic in either direction.
    protected bool IRRU_IsStreamEncrypted(int senderPlayerId, int frequency, bool isSenderEditor)
    {
        if (!IRRU_RFPropagationNetworkComponent.IsEncryptionEnabled())
            return false;

        if (isSenderEditor)
            return false;

        if (IRRU_IsLocalPlayerGM())
            return false;

        int senderHash = IRRU_RadioRxSquelch.GetInstance().GetSenderFillHash(senderPlayerId, frequency);
        if (senderHash == 0)
            return false;

        return senderHash != SCR_IRRURadioEarSettings.GetInstance().GetFillHash(frequency);
    }

    //! GM omniscience is keyed to the LOCAL player's editor state, not the
    //! receiving transceiver: the verdict lands in shared global audio slots,
    //! and a GM with both an editor radio and a possessed character's radio
    //! tuned to one net must not get whichever transceiver's verdict fired
    //! first. A GM who closes the editor and walks as a character is treated
    //! as a player - deliberately.
    protected bool IRRU_IsLocalPlayerGM()
    {
        SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
        if (!core)
            return false;

        SCR_EditorManagerEntity editorManager = core.GetEditorManager();
        if (!editorManager)
            return false;

        return editorManager.IsOpened() && !editorManager.IsLimited();
    }

    //! Called by the squelch layer when a sender's key-state broadcast lands.
    //! Voice can outrun the two-hop RPC: a stream opened on a wrong assumed
    //! verdict is aged so its next packet classifies as START and re-verdicts
    //! immediately (best effort - the engine re-reads the audio variables
    //! only when it restarts the sound event).
    void IRRU_OnSenderKeyInfo(int senderPlayerId, int frequency)
    {
        if (!IRRU_RFPropagationNetworkComponent.IsEncryptionEnabled())
            return;

        IRRU_SenderStream stream;
        if (!m_mIRRU_Streams.Find(senderPlayerId, stream))
            return;

        if (stream.m_iFrequency != frequency)
            return;

        float nowMs = GetGame().GetWorld().GetWorldTime();
        if (nowMs - stream.m_fLastPacketMs > IRRU_VERIFIED_KEEPALIVE_MS)
            return;

        bool shouldBeEncrypted = IRRU_IsStreamEncrypted(senderPlayerId, frequency, stream.m_bSenderEditor);
        if (shouldBeEncrypted == stream.m_bEncryptedForMe)
            return;

        stream.m_fLastPacketMs = nowMs - IRRU_VERIFIED_KEEPALIVE_MS - 100;
    }

    //! Pre-arm the global audio slots the moment an ENCRYPTED sender keys up,
    //! BEFORE any voice packet arrives. Probe-verified (2026-08-23): the
    //! voice-mute side of JamStrength applies from the event's first frame,
    //! but the jam-noise generators latch their level at event spawn - a
    //! same-frame write is one beat too late, so without this the first
    //! transmission after a fill change renders as dead silence instead of
    //! scramble (no leak, but confusing). The key-state RPC beats the first
    //! voice packet by at least a frame, which is exactly the window needed.
    void IRRU_PreArmEncryptedSlots(int senderPlayerId, int frequency, BaseTransceiver receiver)
    {
        if (!IRRU_IsStreamEncrypted(senderPlayerId, frequency, false))
            return;

        IRRU_RadioRxSquelch squelch = IRRU_RadioRxSquelch.GetInstance();
        if (squelch.WasAudioSlotStartedThisFrame())
            return;

        PlayerController playerController = GetGame().GetPlayerController();
        vector receiverPos = vector.Zero;
        if (playerController)
        {
            IEntity receiverEntity = playerController.GetControlledEntity();
            if (receiverEntity)
                receiverPos = receiverEntity.GetOrigin();
        }

        if (receiver)
            IRRU_RadioBeepHelper.ApplyChannelAudioVariables(receiver);

        IRRU_SenderStream stream = IRRU_GetSenderSignals(senderPlayerId, frequency, receiverPos);
        AudioSystem.SetVariableByName("SignalQuality", Math.Max(stream.m_fSignalQuality, IRRU_ENCRYPTED_MIN_QUALITY), IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
        AudioSystem.SetVariableByName("JamStrength", 0, IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
        squelch.OnAudioSlotWritten(frequency, true, true);
    }

    //! Kill-switch immediacy: called on the client when the server disables
    //! encryption mid-session, so streams currently rendering as scramble
    //! re-verdict to plaintext on their next packet instead of finishing the
    //! transmission as noise.
    void IRRU_ForceReverdictEncryptedStreams()
    {
        float staleMs = GetGame().GetWorld().GetWorldTime() - IRRU_VERIFIED_KEEPALIVE_MS - 100;
        foreach (int senderPlayerId, IRRU_SenderStream stream : m_mIRRU_Streams)
        {
            if (stream.m_bEncryptedForMe)
                stream.m_fLastPacketMs = staleMs;
        }
    }

    protected IRRU_SenderStream IRRU_GetSenderStream(int senderPlayerId)
    {
        IRRU_SenderStream stream;
        if (m_mIRRU_Streams.Find(senderPlayerId, stream))
            return stream;

        stream = new IRRU_SenderStream();
        m_mIRRU_Streams.Set(senderPlayerId, stream);
        return stream;
    }

    protected IRRU_SenderStream IRRU_GetSenderSignals(int senderPlayerId, int frequency, vector receiverPos)
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();

        IRRU_SenderStream stream = IRRU_GetSenderStream(senderPlayerId);
        if (stream.m_iFrequency == frequency && nowMs - stream.m_fComputedAtMs < IRRU_SIGNAL_REFRESH_MS)
            return stream;

        stream.m_iFrequency = frequency;
        stream.m_fComputedAtMs = nowMs;
        stream.m_fSignalQuality = IRRU_ComputeSignalQuality(senderPlayerId, frequency, receiverPos);
        // The audio graph expects 1 = clean, 0 = fully jammed
        stream.m_fJamStrength = 1.0 - IRRU_JammerManager.GetInstance().CalculateJammerDegradation(receiverPos);
        return stream;
    }

    protected float IRRU_ComputeSignalQuality(int senderPlayerId, int frequencyKHz, vector receiverPos)
    {
        if (!IRRU_RFPropagationNetworkComponent.IsRFPropagationEnabled())
            return 1.0;

        IEntity transmitter = GetGame().GetPlayerManager().GetPlayerControlledEntity(senderPlayerId);
        if (!transmitter)
            return 1.0;

        return IRRU_RFPropagationModel.GetInstance().CalculateSignalQuality(transmitter.GetOrigin(), receiverPos, frequencyKHz);
    }
}
