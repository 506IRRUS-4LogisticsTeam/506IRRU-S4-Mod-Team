[EntityEditorProps(category: "GameScripted/Weapons", description: "Mortar artillery computer component")]
class IRRU_MortarArtilleryComputerComponentClass : ScriptComponentClass {}

class IRRU_MortarArtilleryComputerComponent : ScriptComponent
{
    protected SCR_MapEntity m_MapEntity;
    protected IEntity m_eOwner;
    protected bool m_bMapOpen = false;
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        Print("[MORTAR] Component OnPostInit - Starting");
        Print(string.Format("[MORTAR] Component OnPostInit - Owner: %1", owner));
        Print(string.Format("[MORTAR] Component OnPostInit - Owner name: %1", owner.GetName()));
        
        m_eOwner = owner;
        
        Print("[MORTAR] Component OnPostInit - Completed");
    }
    
    //------------------------------------------------------------------------------------------------
    void OpenComputer(SCR_MapEntity mapEntity, IEntity user)
    {
        Print("[MORTAR] Component OpenComputer - Starting");
        Print(string.Format("[MORTAR] Component OpenComputer - MapEntity: %1", mapEntity));
        Print(string.Format("[MORTAR] Component OpenComputer - User: %1", user));
        
        if (!mapEntity)
        {
            Print("[MORTAR] Component OpenComputer - ERROR: MapEntity is null!");
            return;
        }
        
        m_MapEntity = mapEntity;
        
        Print("[MORTAR] Component OpenComputer - Setting up map callbacks");
        
        // Set up map callbacks
        m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
        m_MapEntity.GetOnSelection().Insert(OnMapSelection);
        m_MapEntity.GetOnMapClose().Insert(OnMapClose);
        
        Print("[MORTAR] Component OpenComputer - Callbacks registered");
        
        // Just open the standard map menu - simpler approach
        Print("[MORTAR] Component OpenComputer - Opening standard map menu");
        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MapMenu);
        
        m_bMapOpen = true;
        
        // Add escape key handler
        Print("[MORTAR] Component OpenComputer - Adding escape key handler");
        GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, CloseComputer);
        
        Print("[MORTAR] Component OpenComputer - Completed successfully");
    }
    
    //------------------------------------------------------------------------------------------------
    void CloseComputer()
    {
        Print("[MORTAR] Component CloseComputer - Called");
        Print(string.Format("[MORTAR] Component CloseComputer - MapOpen: %1", m_bMapOpen));
        
        if (m_bMapOpen)
        {
            Print("[MORTAR] Component CloseComputer - Closing map menu");
            GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
            m_bMapOpen = false;
        }
        else
        {
            Print("[MORTAR] Component CloseComputer - Map not open");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapOpen(MapConfiguration config)
    {
        Print("[MORTAR] Component OnMapOpen - Map opened successfully");
        Print(string.Format("[MORTAR] Component OnMapOpen - Config: %1", config));
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapSelection(vector selectedPos)
    {
        Print("[MORTAR] Component OnMapSelection - Map clicked!");
        Print(string.Format("[MORTAR] Component OnMapSelection - Selected position: %1", selectedPos));
        
        // Convert screen coordinates to world coordinates
        float worldX, worldY, heightAtPos;
        m_MapEntity.ScreenToWorld(selectedPos[0], selectedPos[2], worldX, worldY);
        
        Print(string.Format("[MORTAR] Component OnMapSelection - Screen to world: X=%1, Y=%2", worldX, worldY));
        
        heightAtPos = m_MapEntity.GetWorld().GetSurfaceY(worldX, worldY);
        
        Print(string.Format("[MORTAR] Component OnMapSelection - Surface height: %1", heightAtPos));
        
        vector targetPos = Vector(worldX, heightAtPos, worldY);
        vector mortarPos = m_eOwner.GetOrigin();
        
        Print(string.Format("[MORTAR] Component OnMapSelection - Target pos: %1", targetPos));
        Print(string.Format("[MORTAR] Component OnMapSelection - Mortar pos: %1", mortarPos));
        
        // Calculate distance and bearing
        vector toTarget = targetPos - mortarPos;
        float distance = toTarget.Length();
        float azimuth = Math.Atan2(toTarget[0], toTarget[2]) * Math.RAD2DEG;
        if (azimuth < 0) 
            azimuth = azimuth + 360;
        
        Print(string.Format("[MORTAR] Component OnMapSelection - Distance: %1m", distance));
        Print(string.Format("[MORTAR] Component OnMapSelection - Azimuth: %1 degrees", azimuth));
        
        // For now, use placeholder elevation (you'll add real ballistics later)
        float elevation = 45.0; // Placeholder - will be replaced with ballistic calculation
        
        Print(string.Format("[MORTAR] Component OnMapSelection - Elevation (placeholder): %1 degrees", elevation));
        
        // Display results
        string hint = string.Format(
            "Firing Solution:\nTarget: %1\nDistance: %2m\nAzimuth: %3°\nElevation: %4°\n\n(Ballistic calculations pending)",
            targetPos.ToString(),
            distance.ToString(0),
            azimuth.ToString(1),
            elevation.ToString(1)
        );
        
        Print("[MORTAR] Component OnMapSelection - Hint content:");
        Print(hint);
        
        // TODO: Display hint to player using SCR_HintManagerComponent
        // SCR_HintManagerComponent.GetInstance().ShowCustomHint(hint, "Artillery Computer", 10.0);
        
        Print("[MORTAR] Component OnMapSelection - Closing computer");
        // Close the map after selection
        GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapClose(MapConfiguration config)
    {
        Print("[MORTAR] Component OnMapClose - Map closing");
        Print(string.Format("[MORTAR] Component OnMapClose - Config: %1", config));
        
        // Clean up callbacks
        Print("[MORTAR] Component OnMapClose - Removing callbacks");
        m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
        m_MapEntity.GetOnSelection().Remove(OnMapSelection);
        m_MapEntity.GetOnMapClose().Remove(OnMapClose);
        
        // Remove escape key handler
        Print("[MORTAR] Component OnMapClose - Removing escape key handler");
        GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, CloseComputer);
        
        m_bMapOpen = false;
        
        Print("[MORTAR] Component OnMapClose - Cleanup completed");
    }
}