[EntityEditorProps(category: "GameScripted/Weapons", description: "Mortar artillery computer component")]
class IRRU_MortarArtilleryComputerComponentClass : ScriptComponentClass {}

class IRRU_MortarArtilleryComputerComponent : ScriptComponent
{
    protected SCR_MapEntity m_MapEntity;
    protected IEntity m_eOwner;
    protected bool m_bMapOpen = false;
    protected string m_sSelectedAmmoType = "HE"; // Default to HE
    protected ref array<string> m_aAvailableAmmoTypes = {"HE", "Smoke", "Illumination"};
    protected bool m_bHintShown = false;
    
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
            return;
            
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
        m_bHintShown = false;
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
        
        // Calculate horizontal distance (used for ballistic table lookup)
        float horizontalDistance = Math.Sqrt(toTarget[0] * toTarget[0] + toTarget[2] * toTarget[2]);
        
        // Calculate azimuth
        float azimuth = Math.Atan2(toTarget[0], toTarget[2]) * Math.RAD2DEG;
        if (azimuth < 0) 
            azimuth = azimuth + 360;
        
        // Check if target is in range
        string ammoType = GetCurrentAmmoType();
        float minRange, maxRange;
        MortarBallisticTables.GetMinMaxRange(ammoType, minRange, maxRange);
        
        if (horizontalDistance < minRange)
        {
            string hint = string.Format(
                "TARGET TOO CLOSE!\n\nMinimum Range: %1m\nTarget Distance: %2m\n\nSelect a target further away!",
                minRange.ToString(0),
                horizontalDistance.ToString(0)
            );
            
            // Display error hint to player
            SCR_HintManagerComponent.ShowCustomHint(hint, "Range Error", 8.0, false);
        }
        else if (horizontalDistance > maxRange)
        {
            string hint = string.Format(
                "TARGET OUT OF RANGE!\n\nMaximum Range: %1m\nTarget Distance: %2m\n\nSelect a closer target!",
                maxRange.ToString(0),
                horizontalDistance.ToString(0)
            );
            
            // Display error hint to player
            SCR_HintManagerComponent.ShowCustomHint(hint, "Range Error", 8.0, false);
        }
        else
        {
            // Target is in valid range - calculate firing solution using ballistic tables
            float elevationMils;
            float timeOfFlight;
            int charge;
            int dElevCorrection;
            
            bool solutionFound = MortarBallisticTables.CalculateSolution(ammoType, horizontalDistance, elevationMils, timeOfFlight, charge, dElevCorrection);
            
            if (!solutionFound)
            {
                // No solution found - likely due to elevation limits
                string hint = string.Format("NO VALID SOLUTION!\n\nTarget at %1m requires elevation\noutside mortar's physical limits.\n\nMove closer/further to target or\nuse different firing position.", 
                    horizontalDistance.ToString(0));
                SCR_HintManagerComponent.ShowCustomHint(hint, "Elevation Limit", 8.0, false);
            }
            else
            {
                // Calculate elevation difference for altitude correction
                float elevationDifference = targetPos[1] - mortarPos[1];
                
                // Apply D_ELEV altitude correction
                if (dElevCorrection > 0 && elevationDifference != 0)
                {
                    float altitudeCorrection = (elevationDifference / 100.0) * dElevCorrection;
                    
                    // When firing downhill (negative elevation difference), we need to INCREASE elevation to shorten the shot
                    // When firing uphill (positive elevation difference), we need to DECREASE elevation to lengthen the shot
                    elevationMils = elevationMils - altitudeCorrection;
                }
                
                // Check if final elevation is within limits
                if (elevationMils < 800.0 || elevationMils > 1515.0)
                {
                    // Clamp to limits
                    if (elevationMils < 800.0) elevationMils = 800.0;
                    if (elevationMils > 1515.0) elevationMils = 1515.0;
                }
                
                // Convert mils to degrees (6400 mils = 360 degrees)
                float elevationDegrees = (elevationMils / 6400.0) * 360.0;
                
                // Calculate slant distance for display purposes
                float slantDistance = toTarget.Length();
                
                // Convert azimuth to mils (6400 mils = 360 degrees)
                float azimuthMils = (azimuth / 360.0) * 6400.0;
                
                // Display firing solution - split into parts due to parameter limit
                string hint1 = string.Format(
                    "FIRING SOLUTION - %1\n\nRange: %2m\nSlant Range: %3m\nElev Diff: %4m\nAzimuth: %5° (%6 mils)",
                    ammoType,
                    horizontalDistance.ToString(0),
                    slantDistance.ToString(0),
                    elevationDifference.ToString(0),
                    azimuth.ToString(1),
                    azimuthMils.ToString(0)
                );
                
                string hint2 = string.Format(
                    "\nElevation: %1 mils (%2°)\n\nCharge: %3 rings\nTime of Flight: %4 sec",
                    elevationMils.ToString(0),
                    elevationDegrees.ToString(1),
                    charge.ToString(),
                    timeOfFlight.ToString(1)
                );
                
                string hint = hint1 + hint2;
                
                // Display hint to player using SCR_HintManagerComponent
                SCR_HintManagerComponent.ShowCustomHint(hint, "Mortar Computer", 30.0, false);
                m_bHintShown = true;
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
        
        // Reset hint shown flag when closing
        if (m_bHintShown)
        {
            m_bHintShown = false;
        }
        
        m_bMapOpen = false;
    }
}