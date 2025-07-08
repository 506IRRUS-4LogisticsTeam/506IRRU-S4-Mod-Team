[EntityEditorProps(category: "GameScripted/Weapons", description: "Mortar artillery computer component")]
class IRRU_MortarArtilleryComputerComponentClass : ScriptComponentClass {}

class IRRU_MortarArtilleryComputerComponent : ScriptComponent
{
    protected SCR_MapEntity m_MapEntity;
    protected IEntity m_eOwner;
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        m_eOwner = owner;
    }
    
    //------------------------------------------------------------------------------------------------
    void OpenComputer(SCR_MapEntity mapEntity, IEntity user)
    {
        m_MapEntity = mapEntity;
        
        // Set up map callbacks
        m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
        m_MapEntity.GetOnSelection().Insert(OnMapSelection);
        m_MapEntity.GetOnMapClose().Insert(OnMapClose);
        
        // Use the standard map menu for now
        m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
        m_MapEntity.GetOnSelection().Insert(OnMapSelection); 
        m_MapEntity.GetOnMapClose().Insert(OnMapClose);
        
        // Open map directly without custom menu
        MapConfiguration mapConfig = m_MapEntity.SetupMapConfig(
            EMapEntityMode.FULLSCREEN,
            "{3C6C98B0E342CAA1}Configs/Map/MapArtilleryComputer.conf",
            null
        );
        m_MapEntity.OpenMap(mapConfig);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapOpen(MapConfiguration config)
    {
        Print("Mortar Computer: Map opened");
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapSelection(vector selectedPos)
    {
        float worldX, worldY, heightAtPos;
        m_MapEntity.ScreenToWorld(selectedPos[0], selectedPos[2], worldX, worldY);
        heightAtPos = m_MapEntity.GetWorld().GetSurfaceY(worldX, worldY);
        
        vector targetPos = Vector(worldX, heightAtPos, worldY);
        
        Print(string.Format("Mortar Computer: Target selected at %1", targetPos.ToString()));
        
        // Close the map directly
        m_MapEntity.CloseMap();
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapClose(MapConfiguration config)
    {
        Print("Mortar Computer: Map closed");
        
        // Clean up callbacks
        m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
        m_MapEntity.GetOnSelection().Remove(OnMapSelection);
        m_MapEntity.GetOnMapClose().Remove(OnMapClose);
    }
}