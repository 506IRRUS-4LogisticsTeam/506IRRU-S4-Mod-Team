class IRRU_SignalManager
{
    private static ref IRRU_SignalManager s_Instance;

    static IRRU_SignalManager GetInstance()
    {
        if (!s_Instance)
            s_Instance = new IRRU_SignalManager();

        return s_Instance;
    }

    float GetSignalQuality(vector transmitterPos, vector receiverPos, float frequencyKHz = 0)
    {
        IRRU_RFPropagationModel propagation = IRRU_RFPropagationModel.GetInstance();
        return propagation.CalculateSignalQuality(transmitterPos, receiverPos, frequencyKHz);
    }

    float GetJammerStrength(vector receiverPos)
    {
        IRRU_JammerManager jammerManager = IRRU_JammerManager.GetInstance();
        return jammerManager.CalculateJammerDegradation(receiverPos);
    }
}