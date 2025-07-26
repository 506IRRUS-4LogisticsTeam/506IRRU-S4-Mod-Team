[EntityEditorProps(category: "GameScripted/Weapons", description: "Mortar artillery computer component")]
class IRRU_MortarArtilleryComputerComponentClass : ScriptComponentClass {}

class IRRU_MortarArtilleryComputerComponent : ScriptComponent
{
    protected SCR_MapEntity m_MapEntity;
    protected IEntity m_eOwner;
    protected bool m_bMapOpen = false;
    protected string m_sSelectedAmmoType = "HE"; // Default to HE
    protected ref array<string> m_aAvailableAmmoTypes = {"HE", "Smoke", "Illumination"};
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        m_eOwner = owner;
        
        // Initialize ballistic tables
        MortarBallisticTables.Initialize();
    }
    
    //------------------------------------------------------------------------------------------------
    protected string GetCurrentAmmoType()
    {
        // Return the manually selected ammo type
        return m_sSelectedAmmoType;
    }
    
    //------------------------------------------------------------------------------------------------
    void CycleAmmoType()
    {
        int currentIndex = m_aAvailableAmmoTypes.Find(m_sSelectedAmmoType);
        currentIndex++;
        
        if (currentIndex >= m_aAvailableAmmoTypes.Count())
            currentIndex = 0;
            
        m_sSelectedAmmoType = m_aAvailableAmmoTypes.Get(currentIndex);
        
        // Display hint to player about ammo type change
        string hint = string.Format("Shell Type: %1", m_sSelectedAmmoType);
        SCR_HintManagerComponent.ShowCustomHint(hint, "Mortar Computer", 3.0, false);
    }
    
    //------------------------------------------------------------------------------------------------
    string GetSelectedAmmoType()
    {
        return m_sSelectedAmmoType;
    }
    
    
    //------------------------------------------------------------------------------------------------
    void OpenComputer(SCR_MapEntity mapEntity, IEntity user)
    {
        
        if (!mapEntity)
        {
            Print("[MORTAR] Component OpenComputer - ERROR: MapEntity is null!");
            return;
        }
        
        m_MapEntity = mapEntity;
        
        // Set up map callbacks
        m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
        m_MapEntity.GetOnSelection().Insert(OnMapSelection);
        m_MapEntity.GetOnMapClose().Insert(OnMapClose);
        
        // Open the standard map menu
        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MapMenu);
        m_bMapOpen = true;
        
        // Add escape key handler
        GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, CloseComputer);
        
        // Show initial hint about controls
        string controlHint = string.Format("Current Shell: %1\n\nClick map to calculate firing solution\nEscape to close", m_sSelectedAmmoType);
        SCR_HintManagerComponent.ShowCustomHint(controlHint, "Mortar Computer", 8.0, false);
    }
    
    //------------------------------------------------------------------------------------------------
    void CloseComputer()
    {
        
        if (m_bMapOpen)
        {
            GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
            m_bMapOpen = false;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapOpen(MapConfiguration config)
    {
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapSelection(vector selectedPos)
    {
        
        // Convert screen coordinates to world coordinates
        float worldX, worldY, heightAtPos;
        m_MapEntity.ScreenToWorld(selectedPos[0], selectedPos[2], worldX, worldY);
        
        heightAtPos = m_MapEntity.GetWorld().GetSurfaceY(worldX, worldY);
        
        vector targetPos = Vector(worldX, heightAtPos, worldY);
        vector mortarPos = m_eOwner.GetOrigin();
        
        
        // Calculate distance and bearing
        vector toTarget = targetPos - mortarPos;
        float distance = toTarget.Length();
        float azimuth = Math.Atan2(toTarget[0], toTarget[2]) * Math.RAD2DEG;
        if (azimuth < 0) 
            azimuth = azimuth + 360;
        
        
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
            
            
            // Display error hint to player
            SCR_HintManagerComponent.ShowCustomHint(hint, "Range Error", 8.0, false);
        }
        else if (distance > maxRange)
        {
            string hint = string.Format(
                "TARGET OUT OF RANGE!\n\nMaximum Range: %1m\nTarget Distance: %2m\n\nSelect a closer target!",
                maxRange.ToString(0),
                distance.ToString(0)
            );
            
            
            // Display error hint to player
            SCR_HintManagerComponent.ShowCustomHint(hint, "Range Error", 8.0, false);
        }
        else
        {
            // Target is in valid range - calculate firing solution
            
            float elevationMils;
            float timeOfFlight;
            int charge;
            
            bool solutionFound = MortarBallisticTables.CalculateSolution(ammoType, distance, elevationMils, timeOfFlight, charge);
            
            if (!solutionFound)
            {
                // No solution found - likely due to elevation limits
                string hint = string.Format("NO VALID SOLUTION!\n\nTarget at %1m requires elevation\nabove mortar's %2 mil limit.\n\nMove closer to target or\nuse different firing position.", 
                    distance.ToString(0), "1515");
                SCR_HintManagerComponent.ShowCustomHint(hint, "Elevation Limit", 8.0, false);
            }
            else
            {
                // Convert mils to degrees (6400 mils = 360 degrees)
                float elevationDegrees = (elevationMils / 6400.0) * 360.0;
                
                // Display results
                string hint = string.Format(
                    "FIRING SOLUTION - %7\n\nRange: %1m\nAzimuth: %2°\nElevation: %3 mils (%4°)\n\nCharge: %5 rings\nTime of Flight: %6 sec",
                    distance.ToString(0),
                    azimuth.ToString(1),
                    elevationMils.ToString(0),
                    elevationDegrees.ToString(1),
                    charge.ToString(),
                    timeOfFlight.ToString(1),
                    ammoType
                );
                
                
                // Display hint to player using SCR_HintManagerComponent
                SCR_HintManagerComponent.ShowCustomHint(hint, "Mortar Computer", 30.0, false);
            }
        }
        
        // Close the map after selection
        GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
    }
    
    //------------------------------------------------------------------------------------------------
    protected void OnMapClose(MapConfiguration config)
    {
        
        // Clean up callbacks
        m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
        m_MapEntity.GetOnSelection().Remove(OnMapSelection);
        m_MapEntity.GetOnMapClose().Remove(OnMapClose);
        
        // Remove escape key handler
        GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, CloseComputer);
        
        m_bMapOpen = false;
        
    }
}