/*
class IRRU_MortarComputerUI : ChimeraMenuBase
{
    protected SCR_MapEntity m_MapEntity;
    
    //------------------------------------------------------------------------------------------------
    override void OnMenuOpen()
    {
        if (!m_MapEntity)
            return;
        
        // Set up fullscreen map using your copied config
        MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(
            EMapEntityMode.FULLSCREEN, 
            "{3C6C98B0E342CAA1}Configs/Map/MapArtilleryComputer.conf", // Replace with your actual path
            GetRootWidget()
        );
        
        m_MapEntity.OpenMap(mapConfigFullscreen);
        m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
        
        // Add escape key listener
        InputManager inputMan = GetGame().GetInputManager();
        if (inputMan)
            inputMan.AddActionListener("MenuBack", EActionTrigger.DOWN, Close);
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnMenuClose()
    {
        if (m_MapEntity)
        {
            SCR_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
            m_MapEntity.CloseMap();
        }
        
        InputManager inputMan = GetGame().GetInputManager();
        if (inputMan)
            inputMan.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Close);
    }
    
    //------------------------------------------------------------------------------------------------
    override void OnMenuInit()
    {
        if (!m_MapEntity)
            m_MapEntity = SCR_MapEntity.GetMapInstance();
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapOpen(MapConfiguration config)
    {
        m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
        Print("Mortar Computer UI: Map interface opened");
    }
}