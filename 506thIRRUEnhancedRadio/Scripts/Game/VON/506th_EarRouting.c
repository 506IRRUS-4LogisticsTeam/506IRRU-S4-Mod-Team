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
    
    static SCR_IRRURadioEarSettings GetInstance()
    {
        if (!s_Instance)
            s_Instance = new SCR_IRRURadioEarSettings();
        
        return s_Instance;
    }
    
    // EAR ROUTING MAGIC :D
    
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
    
    // Beep type stuff
    
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
}