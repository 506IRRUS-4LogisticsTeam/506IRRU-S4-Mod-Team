modded class SCR_VONEntryRadio
{
    //------------------------------------------------------------------------------------------------
    override void Update()
    {
        super.Update();
        
        SCR_VONEntryComponent entryComp = SCR_VONEntryComponent.Cast(m_EntryComponent);
        if (!entryComp)
            return;
        
        if (!m_RadioTransceiver)
            return;
        
        // Get current ear routing and append to frequency text
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        IRRUEarRouting routing = settings.GetRouting(m_RadioTransceiver);
        
        string routingText;
        switch (routing)
        {
            case IRRUEarRouting.LEFT:
                routingText = "LEFT";
                break;
            case IRRUEarRouting.RIGHT:
                routingText = "RIGHT";
                break;
            default:
                routingText = "BOTH";
                break;
        }
        
        // Update frequency text with routing indicator
        entryComp.SetFrequencyText(m_sText + " [" + routingText + "]");
    }
}