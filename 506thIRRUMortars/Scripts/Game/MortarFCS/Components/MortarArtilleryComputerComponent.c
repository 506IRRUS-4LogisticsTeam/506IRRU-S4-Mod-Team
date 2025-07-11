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
        
        // Initialize ballistic tables
        MortarBallisticTables.Initialize();
        
        Print("[MORTAR] Component OnPostInit - Completed");
    }
    
    //------------------------------------------------------------------------------------------------
    protected string GetCurrentAmmoType()
    {
        // Try to get weapon component from mortar
        BaseWeaponComponent weapon = BaseWeaponComponent.Cast(m_eOwner.FindComponent(BaseWeaponComponent));
        if (!weapon)
        {
            Print("[MORTAR] GetCurrentAmmoType - No weapon component found!");
            return "HE"; // Default
        }
        
        // Get current magazine
        BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
        if (!magazine)
        {
            Print("[MORTAR] GetCurrentAmmoType - No magazine found!");
            return "HE"; // Default
        }
        
        // Get magazine type - this is where we'd determine HE/Smoke/Illum
        // TODO: Check actual magazine resource name or ammo type
        IEntity magEntity = magazine.GetOwner();
        if (magEntity)
        {
            string magName = magEntity.GetPrefabData().GetPrefabName();
            Print(string.Format("[MORTAR] GetCurrentAmmoType - Magazine: %1", magName));
            
            // Parse magazine name to determine type
            if (magName.Contains("Smoke") || magName.Contains("smoke"))
                return "Smoke";
            else if (magName.Contains("Illum") || magName.Contains("illum"))
                return "Illumination";
        }
        
        return "HE"; // Default to HE
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
        
        // Check if target is in range
        string ammoType = GetCurrentAmmoType();
        float minRange, maxRange;
        MortarBallisticTables.GetMinMaxRange(ammoType, minRange, maxRange);
        
        if (distance < minRange)
        {
            string hint = string.Format(
                "TARGET TOO CLOSE!\n\nMinimum Range: %1m\nTarget Distance: %2m\n\nSelect a target further away!",
                minRange.ToString(0),
                distance.ToString(0)
            );
            
            Print("[MORTAR] Component OnMapSelection - Target too close!");
            Print(hint);
            
            // TODO: Display error hint to player
            // SCR_HintManagerComponent.GetInstance().ShowCustomHint(hint, "Range Error", 5.0);
        }
        else if (distance > maxRange)
        {
            string hint = string.Format(
                "TARGET OUT OF RANGE!\n\nMaximum Range: %1m\nTarget Distance: %2m\n\nSelect a closer target!",
                maxRange.ToString(0),
                distance.ToString(0)
            );
            
            Print("[MORTAR] Component OnMapSelection - Target out of range!");
            Print(hint);
            
            // TODO: Display error hint to player
            // SCR_HintManagerComponent.GetInstance().ShowCustomHint(hint, "Range Error", 5.0);
        }
        else
        {
            // Target is in valid range - calculate firing solution
            Print("[MORTAR] Component OnMapSelection - Target in range, calculating solution...");
            
            float elevationMils;
            float timeOfFlight;
            int charge;
            
            bool solutionFound = MortarBallisticTables.CalculateSolution(ammoType, distance, elevationMils, timeOfFlight, charge);
            
            if (!solutionFound)
            {
                // Shouldn't happen if range check is correct, but just in case
                string hint = "ERROR: No valid firing solution found!";
                Print("[MORTAR] Component OnMapSelection - No solution found!");
                Print(hint);
            }
            else
            {
                // Convert mils to degrees (6400 mils = 360 degrees)
                float elevationDegrees = (elevationMils / 6400.0) * 360.0;
                
                Print(string.Format("[MORTAR] Component OnMapSelection - Solution found: Charge %1, Elevation %2 mils (%3 degrees), TOF %4s", 
                    charge, elevationMils, elevationDegrees, timeOfFlight));
                
                // Display results
                string hint = string.Format(
                    "FIRING SOLUTION\n\nRange: %1m\nAzimuth: %2°\nElevation: %3 mils (%4°)\n\nCharge: %5 rings\nTime of Flight: %6 sec\nAmmo: %7",
                    distance.ToString(0),
                    azimuth.ToString(1),
                    elevationMils.ToString(0),
                    elevationDegrees.ToString(1),
                    charge.ToString(),
                    timeOfFlight.ToString(1),
                    ammoType
                );
                
                Print("[MORTAR] Component OnMapSelection - Hint content:");
                Print(hint);
                
                // TODO: Display hint to player using SCR_HintManagerComponent
                // SCR_HintManagerComponent.GetInstance().ShowCustomHint(hint, "Artillery Computer", 10.0);
            }
        }
        
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