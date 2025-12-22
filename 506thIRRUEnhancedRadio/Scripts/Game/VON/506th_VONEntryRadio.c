modded class SCR_VONEntryRadio
{
    //------------------------------------------------------------------------------------------------
    void SetEntryFrequency(int freqKHz)
    {
        m_iFrequency = freqKHz;
        
        float fFrequency = Math.Round(m_iFrequency * 0.1) * 0.01;
        m_sText = fFrequency.ToString(3, 1) + " " + LABEL_FREQUENCY_UNITS;
    }
    
    //------------------------------------------------------------------------------------------------
    override void Update()
    {
        super.Update();

        SCR_VONEntryComponent entryComp = SCR_VONEntryComponent.Cast(m_EntryComponent);
        if (!entryComp)
            return;

        if (!m_RadioTransceiver)
            return;

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

        entryComp.SetFrequencyText(m_sText + " [" + routingText + "]");
    }
}