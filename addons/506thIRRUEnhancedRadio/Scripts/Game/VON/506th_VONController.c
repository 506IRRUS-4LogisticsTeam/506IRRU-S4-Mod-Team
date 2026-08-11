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
    protected AudioHandle m_AudioHandleCycle;
    protected AudioHandle m_AudioHandleLocalOn;
    protected AudioHandle m_AudioHandleLocalOff;
    protected bool m_bAlternatePTTActive = false;
    protected SCR_VONEntry m_SavedPrimaryEntry;
    protected int m_iIRRU_KeyedFrequency = -1;
    protected ref array<float> m_aIRRU_KeyUpTimesMs = new array<float>();
    protected float m_fIRRU_KeyLockoutUntilMs = -1;
    protected AudioHandle m_AudioHandleError;
    protected bool m_bIRRU_RadioCheckPlayed = false;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        AudioSystem.PlayEventInitialize(IRRU_RadioBeepHelper.BEEP_CONFIG);
        IRRU_RFPropagationSettings.GetInstance();
    }

    protected void PlayBeepStart(BaseTransceiver transceiver)
    {
        IRRU_RadioBeepHelper.PlayTxStart(transceiver);
    }

    protected void PlayBeepEnd(BaseTransceiver transceiver)
    {
        IRRU_RadioBeepHelper.PlayTxEnd(transceiver);
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
                IRRU_PlayErrorBeep();
                return;
            }

            BaseTransceiver transceiver = radioEntry.GetTransceiver();
            if (transceiver)
            {
                PlayBeepStart(transceiver);
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

    protected void IRRU_PlayErrorBeep()
    {
        if (m_AudioHandleError != 0 && AudioSystem.IsSoundPlayed(m_AudioHandleError))
            AudioSystem.TerminateSound(m_AudioHandleError);

        m_AudioHandleError = AudioSystem.PlaySound(IRRU_SOUND_ERROR);
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
                    PlayBeepEnd(transceiver);
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
            Rpc(RpcAsk_IRRU_KeyState, m_iIRRU_KeyedFrequency, 0.0, false);

        m_iIRRU_KeyedFrequency = frequency;
        Rpc(RpcAsk_IRRU_KeyState, frequency, transceiver.GetRange(), true);
    }

    protected void IRRU_NotifyKeyStop()
    {
        if (m_iIRRU_KeyedFrequency < 0)
            return;

        Rpc(RpcAsk_IRRU_KeyState, m_iIRRU_KeyedFrequency, 0.0, false);
        m_iIRRU_KeyedFrequency = -1;
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RpcAsk_IRRU_KeyState(int frequency, float range, bool keyed)
    {
        PlayerController playerController = PlayerController.Cast(GetOwner());
        if (!playerController)
            return;

        IRRU_RFPropagationNetworkComponent net = IRRU_RFPropagationNetworkComponent.GetInstance();
        if (net)
            net.IRRU_RelayKeyState(playerController.GetPlayerId(), frequency, range, keyed);
    }

    SCR_VONEntryRadio IRRU_FindRadioEntryByFrequency(int frequency)
    {
        return FindEntryByFrequency(frequency);
    }

    override protected void ActionVONProximityToggle(float value, EActionTrigger reason = EActionTrigger.UP)
    {
        if (!m_VONComp)
            return;

        bool wasToggled = m_bIsToggledDirect;

        super.ActionVONProximityToggle(value, reason);

        if (m_bIsToggledDirect && !wasToggled)
        {
            if (m_AudioHandleLocalOn != 0 && AudioSystem.IsSoundPlayed(m_AudioHandleLocalOn))
                AudioSystem.TerminateSound(m_AudioHandleLocalOn);

            m_AudioHandleLocalOn = AudioSystem.PlaySound(IRRU_SOUND_LOCAL_ON);
        }
        else if (!m_bIsToggledDirect && wasToggled)
        {
            if (m_AudioHandleLocalOff != 0 && AudioSystem.IsSoundPlayed(m_AudioHandleLocalOff))
                AudioSystem.TerminateSound(m_AudioHandleLocalOff);

            m_AudioHandleLocalOff = AudioSystem.PlaySound(IRRU_SOUND_LOCAL_OFF);
        }
    }

    override protected void ActionVONTransceiverCycle(float value, EActionTrigger reason = EActionTrigger.UP)
    {
        if (reason == EActionTrigger.DOWN)
        {
            if (m_AudioHandleCycle != 0 && AudioSystem.IsSoundPlayed(m_AudioHandleCycle))
                AudioSystem.TerminateSound(m_AudioHandleCycle);

            m_AudioHandleCycle = AudioSystem.PlaySound(IRRU_SOUND_CYCLE);
        }

        super.ActionVONTransceiverCycle(value, reason);
    }

    override void Update(float timeSlice)
    {
        super.Update(timeSlice);

        if (!m_bIRRU_RadioCheckPlayed)
            IRRU_TryPlayRadioCheck();

        InputManager inputMgr = GetGame().GetInputManager();
        if (inputMgr)
        {
            float altValue = inputMgr.GetActionValue("IRRU_AlternateChannel");
            if (altValue > 0 && !m_bAlternatePTTActive)
                OnAlternatePTTStart();
            else if (altValue <= 0 && m_bAlternatePTTActive)
                OnAlternatePTTEnd();
        }

        if (m_FrequencyInput && m_FrequencyInput.IsOpen())
        {
            if (!m_FrequencyInput.IsInWriteMode())
                m_FrequencyInput.Close(true);

            return;
        }

        if (m_VONMenu && m_VONMenu.GetRadialMenu() && m_VONMenu.GetRadialMenu().IsOpened())
        {
            if (inputMgr && inputMgr.GetActionTriggered("IRRU_VONRoutingAction"))
                OnEarRoutingToggle();

            if (inputMgr && inputMgr.GetActionTriggered("IRRU_SetFrequencyAction"))
                OnSetFrequencyPressed();

            if (inputMgr && inputMgr.GetActionTriggered("IRRU_VONBeepTypeAction"))
                OnBeepTypeToggle();

            float volumeValue = inputMgr.GetActionValue("IRRU_VolumeAction");
            if (volumeValue != 0)
                OnVolumeAdjust(volumeValue);

            if (inputMgr.GetActionTriggered("IRRU_VolumeUp"))
                OnVolumeAdjust(1);

            if (inputMgr.GetActionTriggered("IRRU_VolumeDown"))
                OnVolumeAdjust(-1);

            if (inputMgr && inputMgr.GetActionTriggered("IRRU_AlternateChannelAction"))
                OnAlternateChannelToggle();
        }
    }

    protected void OnEarRoutingToggle()
    {
        SCR_RadialMenu radialMenu = m_VONMenu.GetRadialMenu();
        if (!radialMenu)
            return;

        SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(radialMenu.GetSelectionEntry());
        if (!radioEntry)
            return;

        BaseTransceiver transceiver = radioEntry.GetTransceiver();
        if (!transceiver)
            return;

        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        settings.CycleRouting(transceiver);

        radialMenu.UpdateEntries();
    }

    protected void OnBeepTypeToggle()
    {
        SCR_RadialMenu radialMenu = m_VONMenu.GetRadialMenu();
        if (!radialMenu)
            return;

        SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(radialMenu.GetSelectionEntry());
        if (!radioEntry)
            return;

        BaseTransceiver transceiver = radioEntry.GetTransceiver();
        if (!transceiver)
            return;

        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        settings.CycleBeepType(transceiver);

        radialMenu.UpdateEntries();
    }

    protected void OnSetFrequencyPressed()
    {
        SCR_RadialMenu radialMenu = m_VONMenu.GetRadialMenu();
        if (!radialMenu)
            return;

        SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(radialMenu.GetSelectionEntry());
        if (!radioEntry)
            return;

        BaseTransceiver transceiver = radioEntry.GetTransceiver();
        if (!transceiver)
            return;

        if (!m_FrequencyInput)
            m_FrequencyInput = new IRRU_FrequencyInput();

        m_FrequencyInput.Open(transceiver, radioEntry);
    }

    protected void OnVolumeAdjust(float value)
    {
        SCR_RadialMenu radialMenu = m_VONMenu.GetRadialMenu();
        if (!radialMenu)
            return;

        SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(radialMenu.GetSelectionEntry());
        if (!radioEntry)
            return;

        BaseTransceiver transceiver = radioEntry.GetTransceiver();
        if (!transceiver)
            return;

        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();

        float delta;
        if (value > 0)
            delta = 0.1;
        else
            delta = -0.1;

        settings.AdjustVolume(transceiver, delta);
        radialMenu.UpdateEntries();
    }

    protected void OnAlternateChannelToggle()
    {
        SCR_RadialMenu radialMenu = m_VONMenu.GetRadialMenu();
        if (!radialMenu)
            return;

        SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(radialMenu.GetSelectionEntry());
        if (!radioEntry)
            return;

        BaseTransceiver transceiver = radioEntry.GetTransceiver();
        if (!transceiver)
            return;

        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        settings.ToggleAlternate(transceiver);
        radialMenu.UpdateEntries();
    }

    protected void OnAlternatePTTStart()
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        int altFrequency = settings.GetAlternateFrequency();

        if (altFrequency < 0)
            return;

        SCR_VONEntryRadio altEntry = FindEntryByFrequency(altFrequency);
        if (!altEntry)
            return;

        m_bAlternatePTTActive = true;
        settings.SetTransmittingOnAlternate(true);

        m_SavedPrimaryEntry = m_ActiveEntry;
        m_ActiveEntry = altEntry;
        ActivateVON(EVONTransmitType.CHANNEL);
    }

    protected void OnAlternatePTTEnd()
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

    protected SCR_VONEntryRadio FindEntryByFrequency(int frequency)
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
}
