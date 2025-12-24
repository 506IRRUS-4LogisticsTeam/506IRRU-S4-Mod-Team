class IRRU_SignalManager
{
    private static ref IRRU_SignalManager s_Instance;

    static IRRU_SignalManager GetInstance()
    {
        if (!s_Instance)
            s_Instance = new IRRU_SignalManager();

        return s_Instance;
    }

    // Main entry point - calculates final signal quality
    // Returns: 1.0 = clear, 0.0 = fully degraded
    float GetSignalQuality(vector transmitterPos, vector receiverPos)
    {
        float quality = 1.0;

        // DISABLED: Terrain occlusion - needs more work
        // IRRU_TerrainOcclusion terrainOcclusion = IRRU_TerrainOcclusion.GetInstance();
        // float terrainDegradation = terrainOcclusion.CalculateOcclusion(transmitterPos, receiverPos);
        // quality = quality * (1.0 - terrainDegradation);

        // Jammer effects on receiver
        IRRU_JammerManager jammerManager = IRRU_JammerManager.GetInstance();
        float jammerDegradation = jammerManager.CalculateJammerDegradation(receiverPos);
        quality = quality * (1.0 - jammerDegradation);

        quality = Math.Clamp(quality, 0.0, 1.0);

        //Print(string.Format("[IRRU Signal] Quality: %1 (jammer: %2)", quality, jammerDegradation), LogLevel.NORMAL);

        return quality;
    }
}