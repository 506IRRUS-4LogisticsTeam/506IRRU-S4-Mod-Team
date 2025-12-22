modded class SCR_VONController
{
    //------------------------------------------------------------------------------------------------
    override void Update(float timeSlice)
    {
        super.Update(timeSlice);
        
        // Check for ear routing input when menu is open
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
        Print("[IRRU] OnEarRoutingToggle triggered");
        
        SCR_RadialMenu radialMenu = m_VONMenu.GetRadialMenu();
        if (!radialMenu)
        {
            Print("[IRRU] No radial menu");
            return;
        }
        
        SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(radialMenu.GetSelectionEntry());
        if (!radioEntry)
        {
            Print("[IRRU] No radio entry selected");
            return;
        }
        
        BaseTransceiver transceiver = radioEntry.GetTransceiver();
        if (!transceiver)
        {
            Print("[IRRU] No transceiver found on entry");
            return;
        }
        
        Print("[IRRU] Cycling ear routing for frequency: " + radioEntry.GetEntryFrequency());
        
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        IRRUEarRouting newRouting = settings.CycleRouting(transceiver);
        
        radialMenu.UpdateEntries();
        
        Print("[IRRU] Ear routing changed to: " + settings.GetRoutingDisplayText(newRouting));
    }
}