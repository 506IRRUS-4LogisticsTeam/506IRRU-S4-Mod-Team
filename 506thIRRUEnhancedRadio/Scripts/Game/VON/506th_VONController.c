modded class SCR_VONController
{
    protected ref IRRU_FrequencyInput m_FrequencyInput;

    //------------------------------------------------------------------------------------------------
    override void Update(float timeSlice)
    {
        super.Update(timeSlice);

        if (m_FrequencyInput && m_FrequencyInput.IsOpen())
        {
            if (!m_FrequencyInput.IsInWriteMode())
                m_FrequencyInput.Close(true);
            
            return;
        }

        if (m_VONMenu && m_VONMenu.GetRadialMenu() && m_VONMenu.GetRadialMenu().IsOpened())
        {
            InputManager inputMgr = GetGame().GetInputManager();

            if (inputMgr && inputMgr.GetActionTriggered("IRRU_VONRoutingAction"))
                OnEarRoutingToggle();

            if (inputMgr && inputMgr.GetActionTriggered("IRRU_SetFrequencyAction"))
                OnSetFrequencyPressed();
        }
    }

    //------------------------------------------------------------------------------------------------
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

    //------------------------------------------------------------------------------------------------
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
}