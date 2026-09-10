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
        IRRUEarRouting routing;
        if (transceiver && m_mRoutingByTransceiver.Find(transceiver, routing))
            return routing;

        return IRRUEarRouting.CENTER;
    }

    void SetRouting(BaseTransceiver transceiver, IRRUEarRouting routing)
    {
        if (transceiver)
            m_mRoutingByTransceiver.Set(transceiver, routing);
    }

    //! Cycle order is deliberately C -> L -> R, not enum order
    IRRUEarRouting CycleRouting(BaseTransceiver transceiver)
    {
        IRRUEarRouting next;
        switch (GetRouting(transceiver))
        {
            case IRRUEarRouting.CENTER:
                next = IRRUEarRouting.LEFT;
                break;
            case IRRUEarRouting.LEFT:
                next = IRRUEarRouting.RIGHT;
                break;
            default:
                next = IRRUEarRouting.CENTER;
        }

        SetRouting(transceiver, next);
        return next;
    }

    string GetRoutingDisplayText(IRRUEarRouting routing)
    {
        if (routing == IRRUEarRouting.LEFT)
            return "L";
        if (routing == IRRUEarRouting.RIGHT)
            return "R";
        return "C";
    }

    IRRUBeepType GetBeepType(BaseTransceiver transceiver)
    {
        IRRUBeepType beepType;
        if (transceiver && m_mBeepTypeByTransceiver.Find(transceiver, beepType))
            return beepType;

        return IRRUBeepType.ACE_HIGH;
    }

    void SetBeepType(BaseTransceiver transceiver, IRRUBeepType beepType)
    {
        if (transceiver)
            m_mBeepTypeByTransceiver.Set(transceiver, beepType);
    }

    IRRUBeepType CycleBeepType(BaseTransceiver transceiver)
    {
        IRRUBeepType next;
        switch (GetBeepType(transceiver))
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
            default:
                next = IRRUBeepType.OFF;
        }

        SetBeepType(transceiver, next);
        return next;
    }

    float GetVolume(BaseTransceiver transceiver)
    {
        float volume;
        if (transceiver && m_mVolumeByTransceiver.Find(transceiver, volume))
            return volume;

        return 1.0;
    }

    void SetVolume(BaseTransceiver transceiver, float volume)
    {
        if (transceiver)
            m_mVolumeByTransceiver.Set(transceiver, Math.Clamp(volume, 0.0, 1.0));
    }

    float AdjustVolume(BaseTransceiver transceiver, float delta)
    {
        SetVolume(transceiver, GetVolume(transceiver) + delta);
        return GetVolume(transceiver);
    }

    int GetVolumePercent(BaseTransceiver transceiver)
    {
        return Math.Round(GetVolume(transceiver) * 100);
    }

    //------------------------------------------------------------------------------------------------
    // Crypto fills (COMSEC). Fills are keyed by FREQUENCY, not transceiver
    // object: frequency-keyed state survives respawn/radio swaps and matches
    // the channel identity of the key-state RPC layer ("the net has a fill").
    // Storage and persistence live in IRRU_RadioUserSettings so fills survive
    // reconnects; these wrappers keep radio-settings reads in one class.
    //------------------------------------------------------------------------------------------------

    protected static const string IRRU_BASE36 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    string GetFill(int frequencyKHz)
    {
        return IRRU_RadioUserSettings.GetInstance().GetFill(frequencyKHz);
    }

    void SetFill(int frequencyKHz, string fill)
    {
        IRRU_RadioUserSettings.GetInstance().SetFill(frequencyKHz, fill);
    }

    void ClearFill(int frequencyKHz)
    {
        IRRU_RadioUserSettings.GetInstance().ClearFill(frequencyKHz);
    }

    //! 0 = plaintext (no fill stored for this frequency)
    int GetFillHash(int frequencyKHz)
    {
        return IRRU_HashFill(GetFill(frequencyKHz));
    }

    //! Deterministic djb2 over the digit string; 0 is reserved for plaintext
    static int IRRU_HashFill(string fill)
    {
        if (fill.IsEmpty())
            return 0;

        int hash = 5381;
        for (int i = 0; i < fill.Length(); i++)
        {
            hash = hash * 33 + fill.Get(i).ToAscii();
        }

        if (hash == 0)
            hash = 1;

        return hash;
    }

    static bool IRRU_IsDigits(string text)
    {
        if (text.IsEmpty())
            return false;

        for (int i = 0; i < text.Length(); i++)
        {
            int code = text.Get(i).ToAscii();
            if (code < 48 || code > 57)
                return false;
        }

        return true;
    }

    //! Two-character non-secret check value: many-to-one from the fill, so it
    //! leaks nothing on streams, but lets players confirm a changeover by
    //! glance or voice ("confirm Kilo Seven Alpha")
    static string IRRU_CheckTextFromHash(int hash)
    {
        if (hash == 0)
            return "--";

        int value = hash % 1296;
        if (value < 0)
            value += 1296;

        return IRRU_BASE36.Get(value / 36) + IRRU_BASE36.Get(value % 36);
    }

    //! Radial segment: "K7A" when filled, "--" when plaintext
    string GetFillDisplayText(int frequencyKHz)
    {
        int hash = GetFillHash(frequencyKHz);
        if (hash == 0)
            return "--";

        return "K" + IRRU_CheckTextFromHash(hash);
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

    bool ToggleAlternate(BaseTransceiver transceiver)
    {
        if (!transceiver)
            return false;

        if (IsAlternate(transceiver))
        {
            m_iAlternateFrequency = -1;
            return false;
        }

        m_iAlternateFrequency = transceiver.GetFrequency();
        return true;
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
