modded class SCR_VONController
{
    //------------------------------------------------------------------------------------------------
    override void Update(float timeSlice)
    {
        super.Update(timeSlice);
        
        if (m_VONMenu && m_VONMenu.GetRadialMenu() && m_VONMenu.GetRadialMenu().IsOpened())
        {
            InputManager inputMgr = GetGame().GetInputManager();
            if (inputMgr && inputMgr.GetActionTriggered("IRRU_VONRoutingAction"))
            {
                OnEarRoutingToggle();
            }
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
}