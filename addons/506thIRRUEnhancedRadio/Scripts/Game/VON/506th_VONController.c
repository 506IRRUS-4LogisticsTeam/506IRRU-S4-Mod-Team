modded class SCR_VONController
{
    const string IRRU_SOUND_CYCLE = "{8382F79979658ABA}Sounds/VON/GL_Sounds/RadioCycle.wav";
    const string IRRU_SOUND_LOCAL_OFF = "{F60574D50A8FA527}Sounds/VON/GL_Sounds/RadioLocalOff.wav";
    const string IRRU_SOUND_LOCAL_ON = "{2E10BC6B1FF478BC}Sounds/VON/GL_Sounds/RadioLocalOn.wav";
    const string IRRU_SOUND_ERROR = "{BB24E9E96BDD524F}Sounds/IRRU_Sound/errorbeep.wav";
    const string IRRU_SOUND_ROGER = "{AAACA964A5A37618}Sounds/IRRU_Sound/rogerbeep.wav";

    //! Key-up spam lockout: more radio key-ups than the limit inside the window
    //! refuses transmission for the lockout period, answering each denied
    //! attempt with the deny tone - like a trunked system rejecting the channel.
    protected static const int IRRU_KEY_SPAM_MAX_KEYS = 4;
    protected static const float IRRU_KEY_SPAM_WINDOW_MS = 4000;
    protected static const float IRRU_KEY_SPAM_LOCKOUT_MS = 2000;

    protected ref IRRU_FrequencyInput m_FrequencyInput;
    protected ref IRRU_CryptoFillInput m_CryptoFillInput;
    protected AudioHandle m_AudioHandleCycle;
    protected AudioHandle m_AudioHandleLocalOn;
    protected AudioHandle m_AudioHandleLocalOff;
    protected bool m_bAlternatePTTActive = false;
    protected SCR_VONEntry m_SavedPrimaryEntry;
    protected int m_iIRRU_KeyedFrequency = -1;
    protected float m_fIRRU_KeyedAtMs = -1;
    protected static const float IRRU_KEY_FAILSAFE_GRACE_MS = 1000;
    protected ref array<float> m_aIRRU_KeyUpTimesMs = new array<float>();
    protected float m_fIRRU_KeyLockoutUntilMs = -1;
    protected AudioHandle m_AudioHandleError;
    protected bool m_bIRRU_RadioCheckPlayed = false;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        AudioSystem.PlayEventInitialize(IRRU_RadioBeepHelper.BEEP_CONFIG);
        IRRU_RadioChatCommands.EnsureRegistered();
    }

    override void SetActiveTransmit(notnull SCR_VONEntry entry)
    {
        SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
        if (radioEntry)
        {
            // Denied key-ups never reach super, so no transmission starts, no
            // TX beep plays and no key RPC is sent - just the deny tone.
            if (IRRU_IsKeySpamLocked())
            {
                IRRU_PlayOneShot(IRRU_SOUND_ERROR, m_AudioHandleError);
                return;
            }

            BaseTransceiver transceiver = radioEntry.GetTransceiver();
            if (transceiver)
            {
                IRRU_RadioBeepHelper.PlayTxStart(transceiver);
                IRRU_NotifyKeyStart(transceiver);
            }
        }

        super.SetActiveTransmit(entry);
    }

    //! Records this radio key-up and reports whether the spam lockout is engaged.
    protected bool IRRU_IsKeySpamLocked()
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();

        if (nowMs < m_fIRRU_KeyLockoutUntilMs)
            return true;

        m_aIRRU_KeyUpTimesMs.Insert(nowMs);

        for (int i = m_aIRRU_KeyUpTimesMs.Count() - 1; i >= 0; i--)
        {
            if (nowMs - m_aIRRU_KeyUpTimesMs[i] > IRRU_KEY_SPAM_WINDOW_MS)
                m_aIRRU_KeyUpTimesMs.Remove(i);
        }

        if (m_aIRRU_KeyUpTimesMs.Count() > IRRU_KEY_SPAM_MAX_KEYS)
        {
            m_fIRRU_KeyLockoutUntilMs = nowMs + IRRU_KEY_SPAM_LOCKOUT_MS;
            m_aIRRU_KeyUpTimesMs.Clear();
            return true;
        }

        return false;
    }

    //! Restart-safe one-shot: a still-playing previous instance is cut first
    protected void IRRU_PlayOneShot(string sound, inout AudioHandle handle)
    {
        if (handle != 0 && AudioSystem.IsSoundPlayed(handle))
            AudioSystem.TerminateSound(handle);

        handle = AudioSystem.PlaySound(sound);
    }

    //! One-time radio check on first spawn: tells the player the Enhanced Radio
    //! mod is up, TFAR/ACRE style. The VON controller instance lives exactly one
    //! server session on the client, so the flag resets naturally on reconnect
    //! and never replays on respawn.
    protected void IRRU_TryPlayRadioCheck()
    {
        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
            return;

        IEntity controlledEntity = playerController.GetControlledEntity();
        if (!controlledEntity)
            return;

        if (!SCR_ChimeraCharacter.Cast(controlledEntity))
            return;

        m_bIRRU_RadioCheckPlayed = true;

        // Reliable fallback for the COMSEC join notice: initial-replication
        // onRplName timing is engine-defined, this path is not (it runs once
        // the local player demonstrably exists). Self-guards against repeats.
        IRRU_RFPropagationNetworkComponent net = IRRU_RFPropagationNetworkComponent.GetInstance();
        if (net)
            net.IRRU_AnnounceEncryptionState();

        if (!IRRU_RadioUserSettings.GetInstance().IsRadioCheckEnabled())
            return;

        AudioSystem.PlaySound(IRRU_SOUND_ROGER);

        SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(playerController.FindComponent(SCR_ChatComponent));
        if (chatComponent)
            chatComponent.ShowMessage("***ENHANCED RADIO INITIALIZED***");
    }

    override void DeactivateVON(EVONTransmitType transmitType = EVONTransmitType.NONE)
    {
        if (m_bIsActive && transmitType != EVONTransmitType.DIRECT)
        {
            SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(m_ActiveEntry);
            if (radioEntry)
            {
                BaseTransceiver transceiver = radioEntry.GetTransceiver();
                if (transceiver)
                    IRRU_RadioBeepHelper.PlayTxEnd(transceiver);
            }

            IRRU_NotifyKeyStop();
        }

        super.DeactivateVON(transmitType);
    }

    //! Tell the server this client keyed a radio so receivers can squelch even
    //! when no voice packets flow (dead key). Tracks the keyed frequency
    //! locally so start/stop RPCs always pair up, including active-entry swaps
    //! mid-key (alternate channel PTT).
    protected void IRRU_NotifyKeyStart(BaseTransceiver transceiver)
    {
        int frequency = transceiver.GetFrequency();
        if (m_iIRRU_KeyedFrequency == frequency)
            return;

        if (m_iIRRU_KeyedFrequency >= 0)
            Rpc(RpcAsk_IRRU_KeyState, m_iIRRU_KeyedFrequency, 0.0, false, 0);

        m_iIRRU_KeyedFrequency = frequency;
        m_fIRRU_KeyedAtMs = GetGame().GetWorld().GetWorldTime();
        // The fill hash of the keyed frequency rides the key-start so every
        // receiver can verdict crypto with zero additional traffic. The
        // alternate-channel PTT swap passes the alternate transceiver here,
        // so its net's fill is picked up for free. Stops always carry 0.
        int fillHash = SCR_IRRURadioEarSettings.GetInstance().GetFillHash(frequency);
        Rpc(RpcAsk_IRRU_KeyState, frequency, transceiver.GetRange(), true, fillHash);
    }

    protected void IRRU_NotifyKeyStop()
    {
        if (m_iIRRU_KeyedFrequency < 0)
            return;

        Rpc(RpcAsk_IRRU_KeyState, m_iIRRU_KeyedFrequency, 0.0, false, 0);
        m_iIRRU_KeyedFrequency = -1;
    }

    //! Called by IRRU_FrequencyInput after committing a retune. If the edited
    //! transceiver is the one actively transmitting, voice moves to the new
    //! frequency while receivers there never saw a key-start (or its fill
    //! hash) - re-announce so squelch and crypto stay coherent on both nets.
    //! Identity is checked against the ACTIVE entry's transceiver, not just
    //! the frequency: retuning radio B while transmitting on same-frequency
    //! radio A must not tear down A's announced state (that would strip the
    //! live transmission's hash and play encrypted traffic in the clear).
    void IRRU_OnTransceiverRetuned(BaseTransceiver transceiver, int oldFrequency, int newFrequency)
    {
        if (m_iIRRU_KeyedFrequency != oldFrequency || oldFrequency == newFrequency)
            return;

        if (!m_bIsActive)
            return;

        SCR_VONEntryRadio activeRadio = SCR_VONEntryRadio.Cast(m_ActiveEntry);
        if (!activeRadio || activeRadio.GetTransceiver() != transceiver)
            return;

        Rpc(RpcAsk_IRRU_KeyState, oldFrequency, 0.0, false, 0);
        m_iIRRU_KeyedFrequency = newFrequency;
        m_fIRRU_KeyedAtMs = GetGame().GetWorld().GetWorldTime();
        int fillHash = SCR_IRRURadioEarSettings.GetInstance().GetFillHash(newFrequency);
        Rpc(RpcAsk_IRRU_KeyState, newFrequency, transceiver.GetRange(), true, fillHash);
    }

    //! Server-side: push one keyed sender's state to THIS controller's owning
    //! client only (JIP / mid-transmission joiner sync) - the broadcast path
    //! stays reserved for genuine key transitions so other clients never see
    //! re-announces (which could re-open silence-closed squelch channels).
    void IRRU_ServerSyncKeyState(int senderPlayerId, int frequency, float range, vector senderPos, int fillHash)
    {
        Rpc(RpcDo_IRRU_KeyStateSync, senderPlayerId, frequency, range, senderPos, fillHash);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Owner)]
    protected void RpcDo_IRRU_KeyStateSync(int senderPlayerId, int frequency, float range, vector senderPos, int fillHash)
    {
        IRRU_RadioRxSquelch.GetInstance().OnRemoteKeyState(senderPlayerId, frequency, range, true, senderPos, fillHash);

        IRRU_RFPropagationNetworkComponent net = IRRU_RFPropagationNetworkComponent.GetInstance();
        if (net)
            net.IRRU_EnsureSquelchTicker();
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RpcAsk_IRRU_KeyState(int frequency, float range, bool keyed, int fillHash)
    {
        PlayerController playerController = PlayerController.Cast(GetOwner());
        if (!playerController)
            return;

        IRRU_RFPropagationNetworkComponent net = IRRU_RFPropagationNetworkComponent.GetInstance();
        if (net)
            net.IRRU_RelayKeyState(playerController.GetPlayerId(), frequency, range, keyed, fillHash);
    }

    //! Client entry for the radiocrypto chat command; the server verifies the
    //! requester holds full GM rights before flipping anything.
    void IRRU_RequestEncryptionToggle(bool enabled)
    {
        Rpc(RpcAsk_IRRU_SetEncryption, enabled);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RpcAsk_IRRU_SetEncryption(bool enabled)
    {
        PlayerController playerController = PlayerController.Cast(GetOwner());
        if (!playerController)
            return;

        IRRU_RFPropagationNetworkComponent net = IRRU_RFPropagationNetworkComponent.GetInstance();
        if (net)
            net.IRRU_RequestSetEncryption(playerController.GetPlayerId(), enabled);
    }

    override protected void ActionVONProximityToggle(float value, EActionTrigger reason = EActionTrigger.UP)
    {
        if (!m_VONComp)
            return;

        bool wasToggled = m_bIsToggledDirect;

        super.ActionVONProximityToggle(value, reason);

        if (m_bIsToggledDirect && !wasToggled)
            IRRU_PlayOneShot(IRRU_SOUND_LOCAL_ON, m_AudioHandleLocalOn);
        else if (!m_bIsToggledDirect && wasToggled)
            IRRU_PlayOneShot(IRRU_SOUND_LOCAL_OFF, m_AudioHandleLocalOff);
    }

    override protected void ActionVONTransceiverCycle(float value, EActionTrigger reason = EActionTrigger.UP)
    {
        if (reason == EActionTrigger.DOWN)
            IRRU_PlayOneShot(IRRU_SOUND_CYCLE, m_AudioHandleCycle);

        super.ActionVONTransceiverCycle(value, reason);
    }

    override void Update(float timeSlice)
    {
        super.Update(timeSlice);

        if (!m_bIRRU_RadioCheckPlayed)
            IRRU_TryPlayRadioCheck();

        // Failsafe: a missed key-stop would otherwise suppress every future
        // key RPC (and its crypto fill hash) on this frequency for the whole
        // session - the receivers' side would go stale with no recovery path.
        // The grace window covers any engine ordering where m_bIsActive is
        // set later than our key-start hook (entry activation sequencing).
        if (m_iIRRU_KeyedFrequency >= 0 && !m_bIsActive
            && GetGame().GetWorld().GetWorldTime() - m_fIRRU_KeyedAtMs > IRRU_KEY_FAILSAFE_GRACE_MS)
            IRRU_NotifyKeyStop();

        InputManager inputMgr = GetGame().GetInputManager();
        if (!inputMgr)
            return;

        float altValue = inputMgr.GetActionValue("IRRU_AlternateChannel");
        if (altValue > 0 && !m_bAlternatePTTActive)
            IRRU_OnAlternatePTTStart();
        else if (altValue <= 0 && m_bAlternatePTTActive)
            IRRU_OnAlternatePTTEnd();

        if (m_FrequencyInput && m_FrequencyInput.IsOpen())
        {
            if (!m_FrequencyInput.IsInWriteMode())
                m_FrequencyInput.Close(true);

            return;
        }

        if (m_CryptoFillInput && m_CryptoFillInput.IsOpen())
        {
            // Radial closing under the modal cancels it (never commits)
            bool radialStillOpen = false;
            if (m_VONMenu)
            {
                SCR_RadialMenu cryptoRadial = m_VONMenu.GetRadialMenu();
                radialStillOpen = cryptoRadial && cryptoRadial.IsOpened();
            }

            if (radialStillOpen)
                m_CryptoFillInput.Poll();
            else
                m_CryptoFillInput.Cancel();

            return;
        }

        if (!m_VONMenu)
            return;

        SCR_RadialMenu radialMenu = m_VONMenu.GetRadialMenu();
        if (!radialMenu || !radialMenu.IsOpened())
            return;

        if (inputMgr.GetActionTriggered("IRRU_VONRoutingAction"))
            IRRU_OnEarRoutingToggle();

        if (inputMgr.GetActionTriggered("IRRU_SetFrequencyAction"))
            IRRU_OnSetFrequencyPressed();

        if (IRRU_RFPropagationNetworkComponent.IsEncryptionEnabled()
            && inputMgr.GetActionTriggered("IRRU_SetCryptoFillAction"))
            IRRU_OnSetFillPressed();

        if (inputMgr.GetActionTriggered("IRRU_VONBeepTypeAction"))
            IRRU_OnBeepTypeToggle();

        float volumeValue = inputMgr.GetActionValue("IRRU_VolumeAction");
        if (volumeValue != 0)
            IRRU_OnVolumeAdjust(volumeValue);

        if (inputMgr.GetActionTriggered("IRRU_VolumeUp"))
            IRRU_OnVolumeAdjust(1);

        if (inputMgr.GetActionTriggered("IRRU_VolumeDown"))
            IRRU_OnVolumeAdjust(-1);

        if (inputMgr.GetActionTriggered("IRRU_AlternateChannelAction"))
            IRRU_OnAlternateChannelToggle();
    }

    //! Transceiver of the radio entry highlighted in the VON radial menu, if any
    protected BaseTransceiver IRRU_GetRadialSelection(out SCR_RadialMenu radialMenu, out SCR_VONEntryRadio radioEntry)
    {
        radialMenu = m_VONMenu.GetRadialMenu();
        if (!radialMenu)
            return null;

        radioEntry = SCR_VONEntryRadio.Cast(radialMenu.GetSelectionEntry());
        if (!radioEntry)
            return null;

        return radioEntry.GetTransceiver();
    }

    protected void IRRU_OnEarRoutingToggle()
    {
        SCR_RadialMenu radialMenu;
        SCR_VONEntryRadio radioEntry;
        BaseTransceiver transceiver = IRRU_GetRadialSelection(radialMenu, radioEntry);
        if (!transceiver)
            return;

        SCR_IRRURadioEarSettings.GetInstance().CycleRouting(transceiver);
        radialMenu.UpdateEntries();
    }

    protected void IRRU_OnBeepTypeToggle()
    {
        SCR_RadialMenu radialMenu;
        SCR_VONEntryRadio radioEntry;
        BaseTransceiver transceiver = IRRU_GetRadialSelection(radialMenu, radioEntry);
        if (!transceiver)
            return;

        SCR_IRRURadioEarSettings.GetInstance().CycleBeepType(transceiver);
        radialMenu.UpdateEntries();
    }

    protected void IRRU_OnSetFrequencyPressed()
    {
        SCR_RadialMenu radialMenu;
        SCR_VONEntryRadio radioEntry;
        BaseTransceiver transceiver = IRRU_GetRadialSelection(radialMenu, radioEntry);
        if (!transceiver)
            return;

        if (!m_FrequencyInput)
            m_FrequencyInput = new IRRU_FrequencyInput();

        m_FrequencyInput.Open(transceiver, radioEntry);
    }

    protected void IRRU_OnSetFillPressed()
    {
        SCR_RadialMenu radialMenu;
        SCR_VONEntryRadio radioEntry;
        BaseTransceiver transceiver = IRRU_GetRadialSelection(radialMenu, radioEntry);
        if (!transceiver)
            return;

        if (!m_CryptoFillInput)
            m_CryptoFillInput = new IRRU_CryptoFillInput();

        m_CryptoFillInput.Open(transceiver, radioEntry);
    }

    protected void IRRU_OnVolumeAdjust(float value)
    {
        SCR_RadialMenu radialMenu;
        SCR_VONEntryRadio radioEntry;
        BaseTransceiver transceiver = IRRU_GetRadialSelection(radialMenu, radioEntry);
        if (!transceiver)
            return;

        float delta = -0.1;
        if (value > 0)
            delta = 0.1;

        SCR_IRRURadioEarSettings.GetInstance().AdjustVolume(transceiver, delta);
        radialMenu.UpdateEntries();
    }

    protected void IRRU_OnAlternateChannelToggle()
    {
        SCR_RadialMenu radialMenu;
        SCR_VONEntryRadio radioEntry;
        BaseTransceiver transceiver = IRRU_GetRadialSelection(radialMenu, radioEntry);
        if (!transceiver)
            return;

        SCR_IRRURadioEarSettings.GetInstance().ToggleAlternate(transceiver);
        radialMenu.UpdateEntries();
    }

    protected void IRRU_OnAlternatePTTStart()
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        int altFrequency = settings.GetAlternateFrequency();

        if (altFrequency < 0)
            return;

        SCR_VONEntryRadio altEntry = IRRU_FindEntryByFrequency(altFrequency);
        if (!altEntry)
            return;

        m_bAlternatePTTActive = true;
        settings.SetTransmittingOnAlternate(true);

        m_SavedPrimaryEntry = m_ActiveEntry;
        m_ActiveEntry = altEntry;
        ActivateVON(EVONTransmitType.CHANNEL);
    }

    protected void IRRU_OnAlternatePTTEnd()
    {
        if (!m_bAlternatePTTActive)
            return;

        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        settings.SetTransmittingOnAlternate(false);
        m_bAlternatePTTActive = false;

        DeactivateVON(EVONTransmitType.CHANNEL);

        if (m_SavedPrimaryEntry)
        {
            m_ActiveEntry = m_SavedPrimaryEntry;
            m_SavedPrimaryEntry = null;
        }
    }

    SCR_VONEntryRadio IRRU_FindEntryByFrequency(int frequency)
    {
        if (frequency < 0)
            return null;

        array<ref SCR_VONEntry> entries = {};
        GetVONEntries(entries);

        foreach (SCR_VONEntry entry : entries)
        {
            SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
            if (radioEntry)
            {
                BaseTransceiver transceiver = radioEntry.GetTransceiver();
                if (transceiver && transceiver.GetFrequency() == frequency)
                    return radioEntry;
            }
        }

        return null;
    }

    //! Like IRRU_FindEntryByFrequency but skips unpowered radios, so a
    //! duplicate-frequency radio that is switched off cannot shadow a powered
    //! one (first-match would drop squelch and crypto for the powered radio)
    SCR_VONEntryRadio IRRU_FindPoweredEntryByFrequency(int frequency)
    {
        if (frequency < 0)
            return null;

        array<ref SCR_VONEntry> entries = {};
        GetVONEntries(entries);

        foreach (SCR_VONEntry entry : entries)
        {
            SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
            if (!radioEntry)
                continue;

            BaseTransceiver transceiver = radioEntry.GetTransceiver();
            if (!transceiver || transceiver.GetFrequency() != frequency)
                continue;

            BaseRadioComponent radio = transceiver.GetRadio();
            if (!radio || !radio.IsPowered())
                continue;

            return radioEntry;
        }

        return null;
    }
}
