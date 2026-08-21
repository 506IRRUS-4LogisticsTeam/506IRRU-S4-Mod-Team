class IRRU_JammerManager
{
    private static ref IRRU_JammerManager s_Instance;
    protected ref array<IRRU_JammerComponent> m_aJammers = new array<IRRU_JammerComponent>();

    static IRRU_JammerManager GetInstance()
    {
        if (!s_Instance)
            s_Instance = new IRRU_JammerManager();

        return s_Instance;
    }

    void RegisterJammer(IRRU_JammerComponent jammer)
    {
        if (!m_aJammers.Contains(jammer))
            m_aJammers.Insert(jammer);
    }

    void UnregisterJammer(IRRU_JammerComponent jammer)
    {
        m_aJammers.RemoveItem(jammer);
    }

    //! Degradation from the strongest jammer covering the receiver: 0 = none, 1 = fully jammed
    float CalculateJammerDegradation(vector receiverPos)
    {
        float worstDegradation = 0.0;

        foreach (IRRU_JammerComponent jammer : m_aJammers)
        {
            if (!jammer || !jammer.IsJammerActive())
                continue;

            float degradation = CalculateSingleJammerEffect(jammer, receiverPos);
            if (degradation > worstDegradation)
                worstDegradation = degradation;
        }

        return worstDegradation;
    }

    protected float CalculateSingleJammerEffect(IRRU_JammerComponent jammer, vector receiverPos)
    {
        vector jammerPos = jammer.GetPosition();
        float range = jammer.GetRange();
        float distance = vector.Distance(receiverPos, jammerPos);

        if (distance >= range)
            return 0.0;

        float coneAngle = jammer.GetConeAngle();
        if (coneAngle < 180)
        {
            vector dirToTarget = (receiverPos - jammerPos).Normalized();
            float angle = Math.Acos(vector.Dot(dirToTarget, jammer.GetForwardVector())) * Math.RAD2DEG;
            if (angle > coneAngle * 0.5)
                return 0.0;
        }

        // Area-denial falloff: full strength near the emitter, fading as distance^2 toward the edge
        float ratio = distance / range;
        return 1.0 - ratio * ratio;
    }
}
