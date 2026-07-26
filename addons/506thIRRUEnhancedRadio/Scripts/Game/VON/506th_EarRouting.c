enum IRRUEarRouting
{
    CENTER = 0,
    RIGHT = 1,
    LEFT = 2
}

enum IRRUBeepType
{
    OFF = 0,
    ACE_HIGH = 1,
    ACE_LOW = 2,
    GRS = 3
}

class SCR_IRRURadioEarSettings
{
    private static ref SCR_IRRURadioEarSettings s_Instance;

    protected ref map<BaseTransceiver, IRRUEarRouting> m_mRoutingByTransceiver = new map<BaseTransceiver, IRRUEarRouting>();
    protected ref map<BaseTransceiver, IRRUBeepType> m_mBeepTypeByTransceiver = new map<BaseTransceiver, IRRUBeepType>();
    protected ref map<BaseTransceiver, float> m_mVolumeByTransceiver = new map<BaseTransceiver, float>();
    protected int m_iAlternateFrequency = -1;
    protected bool m_bTransmittingOnAlternate = false;

    static SCR_IRRURadioEarSettings GetInstance()
    {
        if (!s_Instance)
            s_Instance = new SCR_IRRURadioEarSettings();

        return s_Instance;
    }

    IRRUEarRouting GetRouting(BaseTransceiver transceiver)
    {
        if (!transceiver)
            return IRRUEarRouting.CENTER;

        if (!m_mRoutingByTransceiver.Contains(transceiver))
            return IRRUEarRouting.CENTER;

        return m_mRoutingByTransceiver.Get(transceiver);
    }

    void SetRouting(BaseTransceiver transceiver, IRRUEarRouting routing)
    {
        if (!transceiver)
            return;

        m_mRoutingByTransceiver.Set(transceiver, routing);
    }

    IRRUEarRouting CycleRouting(BaseTransceiver transceiver)
    {
        if (!transceiver)
            return IRRUEarRouting.CENTER;

        IRRUEarRouting current = GetRouting(transceiver);
        IRRUEarRouting next;

        switch (current)
        {
            case IRRUEarRouting.CENTER:
                next = IRRUEarRouting.LEFT;
                break;
            case IRRUEarRouting.LEFT:
                next = IRRUEarRouting.RIGHT;
                break;
            case IRRUEarRouting.RIGHT:
                next = IRRUEarRouting.CENTER;
                break;
            default:
                next = IRRUEarRouting.CENTER;
        }

        SetRouting(transceiver, next);
        return next;
    }

    string GetRoutingDisplayText(IRRUEarRouting routing)
    {
        switch (routing)
        {
            case IRRUEarRouting.LEFT:
                return "L";
            case IRRUEarRouting.RIGHT:
                return "R";
            default:
                return "C";
        }

        return "C";
    }

    IRRUBeepType GetBeepType(BaseTransceiver transceiver)
    {
        if (!transceiver)
            return IRRUBeepType.ACE_HIGH;

        if (!m_mBeepTypeByTransceiver.Contains(transceiver))
            return IRRUBeepType.ACE_HIGH;

        return m_mBeepTypeByTransceiver.Get(transceiver);
    }

    void SetBeepType(BaseTransceiver transceiver, IRRUBeepType beepType)
    {
        if (!transceiver)
            return;

        m_mBeepTypeByTransceiver.Set(transceiver, beepType);
    }

    IRRUBeepType CycleBeepType(BaseTransceiver transceiver)
    {
        if (!transceiver)
            return IRRUBeepType.ACE_HIGH;

        IRRUBeepType current = GetBeepType(transceiver);
        IRRUBeepType next;

        switch (current)
        {
            case IRRUBeepType.OFF:
                next = IRRUBeepType.ACE_HIGH;
                break;
            case IRRUBeepType.ACE_HIGH:
                next = IRRUBeepType.ACE_LOW;
                break;
            case IRRUBeepType.ACE_LOW:
                next = IRRUBeepType.GRS;
                break;
            case IRRUBeepType.GRS:
                next = IRRUBeepType.OFF;
                break;
            default:
                next = IRRUBeepType.ACE_LOW;
        }

        SetBeepType(transceiver, next);
        return next;
    }

    string GetBeepTypeDisplayText(IRRUBeepType beepType)
    {
        switch (beepType)
        {
            case IRRUBeepType.OFF:
                return "OFF";
            case IRRUBeepType.ACE_HIGH:
                return "ACE-H";
            case IRRUBeepType.ACE_LOW:
                return "ACE-L";
            case IRRUBeepType.GRS:
                return "GRS";
            default:
                return "ACE-L";
        }

        return "ACE-H";
    }

    float GetVolume(BaseTransceiver transceiver)
    {
        if (!transceiver)
            return 1.0;

        if (!m_mVolumeByTransceiver.Contains(transceiver))
            return 1.0;

        return m_mVolumeByTransceiver.Get(transceiver);
    }

    void SetVolume(BaseTransceiver transceiver, float volume)
    {
        if (!transceiver)
            return;

        m_mVolumeByTransceiver.Set(transceiver, Math.Clamp(volume, 0.0, 1.0));
    }

    float AdjustVolume(BaseTransceiver transceiver, float delta)
    {
        float current = GetVolume(transceiver);
        float newVolume = Math.Clamp(current + delta, 0.0, 1.0);
        SetVolume(transceiver, newVolume);
        return newVolume;
    }

    int GetVolumePercent(BaseTransceiver transceiver)
    {
        return Math.Round(GetVolume(transceiver) * 100);
    }

    string GetVolumeDisplayText(BaseTransceiver transceiver)
    {
        int percent = GetVolumePercent(transceiver);
        return percent.ToString() + "%";
    }

    int GetAlternateFrequency()
    {
        return m_iAlternateFrequency;
    }

    bool IsAlternate(BaseTransceiver transceiver)
    {
        if (!transceiver || m_iAlternateFrequency < 0)
            return false;

        return transceiver.GetFrequency() == m_iAlternateFrequency;
    }

    void SetAlternateFrequency(int frequency)
    {
        m_iAlternateFrequency = frequency;
    }

    void ClearAlternateFrequency()
    {
        m_iAlternateFrequency = -1;
    }

    bool ToggleAlternate(BaseTransceiver transceiver)
    {
        if (!transceiver)
            return false;

        if (IsAlternate(transceiver))
        {
            ClearAlternateFrequency();
            return false;
        }
        else
        {
            SetAlternateFrequency(transceiver.GetFrequency());
            return true;
        }
    }

    bool IsTransmittingOnAlternate()
    {
        return m_bTransmittingOnAlternate;
    }

    void SetTransmittingOnAlternate(bool transmitting)
    {
        m_bTransmittingOnAlternate = transmitting;
    }
}
