modded class SCR_VONEntryRadio
{
    void SetEntryFrequency(int freqKHz)
    {
        m_iFrequency = freqKHz;

        float fFrequency = Math.Round(m_iFrequency * 0.1) * 0.01;
        m_sText = fFrequency.ToString(3, 1) + " " + LABEL_FREQUENCY_UNITS;
    }

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
        IRRUBeepType beepType = settings.GetBeepType(m_RadioTransceiver);
        int volume = settings.GetVolumePercent(m_RadioTransceiver);
        bool isAlternate = settings.IsAlternate(m_RadioTransceiver);

        string routingText = settings.GetRoutingDisplayText(routing);

        string beepText;
        switch (beepType)
        {
            case IRRUBeepType.OFF: beepText = "-"; break;
            case IRRUBeepType.ACE_HIGH: beepText = "BH"; break;
            case IRRUBeepType.ACE_LOW: beepText = "BL"; break;
            case IRRUBeepType.GRS: beepText = "GRS"; break;
            default: beepText = "BH";
        }

        string displayText = m_sText + " " + routingText + "|" + beepText + "|" + volume.ToString();
        entryComp.SetFrequencyText(displayText);

        if (isAlternate)
            entryComp.SetFrequencyColor(Color.FromInt(Color.CYAN));
    }
}
