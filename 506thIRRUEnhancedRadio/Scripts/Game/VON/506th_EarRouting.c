enum IRRUEarRouting
{
    CENTER = 0,
    RIGHT = 1,
    LEFT = 2
}

class SCR_IRRURadioEarSettings
{
    private static ref SCR_IRRURadioEarSettings s_Instance;
    
    protected ref map<BaseTransceiver, IRRUEarRouting> m_mRoutingByTransceiver = new map<BaseTransceiver, IRRUEarRouting>();
    
    //------------------------------------------------------------------------------------------------
    static SCR_IRRURadioEarSettings GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new SCR_IRRURadioEarSettings();
            Print("[IRRU] Settings manager initialized");
        }
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    IRRUEarRouting GetRouting(BaseTransceiver transceiver)
    {
        if (!transceiver)
        {
            Print("[IRRU] GetRouting - NULL transceiver, returning CENTER");
            return IRRUEarRouting.CENTER;
        }
        
        if (!m_mRoutingByTransceiver.Contains(transceiver))
        {
            Print("[IRRU] GetRouting - No routing set for transceiver, returning CENTER");
            return IRRUEarRouting.CENTER;
        }
        
        IRRUEarRouting routing = m_mRoutingByTransceiver.Get(transceiver);
        Print("[IRRU] GetRouting - Found routing: " + GetRoutingDisplayText(routing));
        return routing;
    }
    
    //------------------------------------------------------------------------------------------------
    void SetRouting(BaseTransceiver transceiver, IRRUEarRouting routing)
    {
        if (!transceiver)
        {
            Print("[IRRU] SetRouting - NULL transceiver, aborting");
            return;
        }
        
        m_mRoutingByTransceiver.Set(transceiver, routing);
        Print("[IRRU] SetRouting - Set to: " + GetRoutingDisplayText(routing));
    }
    
    //------------------------------------------------------------------------------------------------
    IRRUEarRouting CycleRouting(BaseTransceiver transceiver)
    {
        if (!transceiver)
        {
            Print("[IRRU] CycleRouting - NULL transceiver, returning CENTER");
            return IRRUEarRouting.CENTER;
        }
        
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
        
        Print("[IRRU] CycleRouting - " + GetRoutingDisplayText(current) + " -> " + GetRoutingDisplayText(next));
        SetRouting(transceiver, next);
        return next;
    }
    
    //------------------------------------------------------------------------------------------------
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
}