modded class SCR_VONController
{
    const string IRRU_SOUND_CYCLE = "{8382F79979658ABA}Sounds/VON/GL_Sounds/RadioCycle.wav";
    const string IRRU_SOUND_LOCAL_OFF = "{F60574D50A8FA527}Sounds/VON/GL_Sounds/RadioLocalOff.wav";
    const string IRRU_SOUND_LOCAL_ON = "{2E10BC6B1FF478BC}Sounds/VON/GL_Sounds/RadioLocalOn.wav";

    protected ref IRRU_FrequencyInput m_FrequencyInput;
    protected AudioHandle m_AudioHandleCycle;
    protected AudioHandle m_AudioHandleLocalOn;
    protected AudioHandle m_AudioHandleLocalOff;
    protected bool m_bAlternatePTTActive = false;
    protected SCR_VONEntry m_SavedPrimaryEntry;

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
            BaseTransceiver transceiver = radioEntry.GetTransceiver();
            if (transceiver)
                PlayBeepStart(transceiver);
        }

        super.SetActiveTransmit(entry);
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
        }

        super.DeactivateVON(transmitType);
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
