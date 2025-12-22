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
            s_Instance = new SCR_IRRURadioEarSettings();
        
        return s_Instance;
    }
    
    //------------------------------------------------------------------------------------------------
    IRRUEarRouting GetRouting(BaseTransceiver transceiver)
    {
        if (!transceiver)
            return IRRUEarRouting.CENTER;
        
        if (!m_mRoutingByTransceiver.Contains(transceiver))
            return IRRUEarRouting.CENTER;
        
        return m_mRoutingByTransceiver.Get(transceiver);
    }
    
    //------------------------------------------------------------------------------------------------
    void SetRouting(BaseTransceiver transceiver, IRRUEarRouting routing)
    {
        if (!transceiver)
            return;
        
        m_mRoutingByTransceiver.Set(transceiver, routing);
    }
    
    //------------------------------------------------------------------------------------------------
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