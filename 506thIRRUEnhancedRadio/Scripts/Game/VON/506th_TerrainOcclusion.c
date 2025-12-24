class IRRU_TerrainOcclusion
{
    private static ref IRRU_TerrainOcclusion s_Instance;

    static IRRU_TerrainOcclusion GetInstance()
    {
        if (!s_Instance)
            s_Instance = new IRRU_TerrainOcclusion();

        return s_Instance;
    }

    // Returns degradation: 0.0 = clear LOS, 1.0 = fully blocked
    float CalculateOcclusion(vector transmitterPos, vector receiverPos)
    {
        // Offset positions slightly up to account for character height
        vector startPos = transmitterPos + Vector(0, 1.5, 0);
        vector endPos = receiverPos + Vector(0, 1.5, 0);

        BaseWorld world = GetGame().GetWorld();
        if (!world)
            return 0.0;

        TraceParam trace = new TraceParam();
        trace.Start = startPos;
        trace.End = endPos;
        trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
        trace.LayerMask = EPhysicsLayerPresets.Projectile;

        float traceResult = world.TraceMove(trace, null);

        // traceResult: 1.0 = no hit (clear LOS), < 1.0 = hit something
        if (traceResult < 1.0)
        {
            // Hit terrain/building
            // Cap at 0.8 - radios can partially punch through obstacles
            float occlusion = Math.Clamp((1.0 - traceResult) * 0.8, 0.0, 0.8);

            Print(string.Format("[IRRU Occlusion] Blocked - trace: %1, degradation: %2", traceResult, occlusion), LogLevel.NORMAL);

            return occlusion;
        }

        return 0.0;
    }
}