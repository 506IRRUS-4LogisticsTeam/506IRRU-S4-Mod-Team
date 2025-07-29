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
        Print("[MORTAR DEBUG] ========== NEW FIRING SOLUTION CALCULATION ==========");
        
        // Convert screen coordinates to world coordinates
        float worldX, worldY, heightAtPos;
        m_MapEntity.ScreenToWorld(selectedPos[0], selectedPos[2], worldX, worldY);
        
        heightAtPos = m_MapEntity.GetWorld().GetSurfaceY(worldX, worldY);
        
        vector targetPos = Vector(worldX, heightAtPos, worldY);
        vector mortarPos = m_eOwner.GetOrigin();
        
        Print(string.Format("[MORTAR DEBUG] Mortar Position: X=%1, Y=%2, Z=%3", 
            mortarPos[0], mortarPos[1], mortarPos[2]));
        Print(string.Format("[MORTAR DEBUG] Target Position: X=%1, Y=%2, Z=%3", 
            targetPos[0], targetPos[1], targetPos[2]));
        
        // Calculate distance and bearing
        vector toTarget = targetPos - mortarPos;
        
        Print(string.Format("[MORTAR DEBUG] Vector to Target: X=%1, Y=%2, Z=%3", 
            toTarget[0], toTarget[1], toTarget[2]));
        
        // Calculate horizontal distance (used for ballistic table lookup)
        float horizontalDistance = Math.Sqrt(toTarget[0] * toTarget[0] + toTarget[2] * toTarget[2]);
        
        Print(string.Format("[MORTAR DEBUG] Horizontal Distance: %1m", horizontalDistance));
        Print(string.Format("[MORTAR DEBUG] Elevation Difference: %1m (Target Y - Mortar Y)", 
            targetPos[1] - mortarPos[1]));
        Print(string.Format("[MORTAR DEBUG] Slant Distance: %1m", toTarget.Length()));
        
        // Calculate azimuth
        float azimuth = Math.Atan2(toTarget[0], toTarget[2]) * Math.RAD2DEG;
        if (azimuth < 0) 
            azimuth = azimuth + 360;
            
        Print(string.Format("[MORTAR DEBUG] Azimuth: %1 degrees", azimuth));
        
        // Check if target is in range
        string ammoType = GetCurrentAmmoType();
        float minRange, maxRange;
        MortarBallisticTables.GetMinMaxRange(ammoType, minRange, maxRange);
        
        Print(string.Format("[MORTAR DEBUG] Selected Ammo Type: %1", ammoType));
        Print(string.Format("[MORTAR DEBUG] Min Range for %1: %2m", ammoType, minRange));
        Print(string.Format("[MORTAR DEBUG] Max Range for %1: %2m", ammoType, maxRange));
        
        if (horizontalDistance < minRange)
        {
            Print(string.Format("[MORTAR DEBUG] ERROR: Target too close! Distance %1m < Min %2m", 
                horizontalDistance, minRange));
                
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
            Print(string.Format("[MORTAR DEBUG] ERROR: Target out of range! Distance %1m > Max %2m", 
                horizontalDistance, maxRange));
                
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
            Print("[MORTAR DEBUG] Target is in valid range, calculating firing solution...");
            
            // Target is in valid range - calculate firing solution using ballistic tables
            float elevationMils;
            float timeOfFlight;
            int charge;
            int dElevCorrection;
            
            Print(string.Format("[MORTAR DEBUG] Calling CalculateSolution for %1 at %2m", ammoType, horizontalDistance));
            
            bool solutionFound = MortarBallisticTables.CalculateSolution(ammoType, horizontalDistance, elevationMils, timeOfFlight, charge, dElevCorrection);
            
            if (!solutionFound)
            {
                Print("[MORTAR DEBUG] ERROR: No valid solution found!");
                
                // No solution found - likely due to elevation limits
                string hint = string.Format("NO VALID SOLUTION!\n\nTarget at %1m requires elevation\noutside mortar's physical limits.\n\nMove closer/further to target or\nuse different firing position.", 
                    horizontalDistance.ToString(0));
                SCR_HintManagerComponent.ShowCustomHint(hint, "Elevation Limit", 8.0, false);
            }
            else
            {
                Print("[MORTAR DEBUG] Solution found!");
                Print(string.Format("[MORTAR DEBUG] Best Charge: %1 rings", charge));
                Print(string.Format("[MORTAR DEBUG] Base Elevation (before altitude correction): %1 mils", elevationMils));
                Print(string.Format("[MORTAR DEBUG] Time of Flight: %1 seconds", timeOfFlight));
                Print(string.Format("[MORTAR DEBUG] D_ELEV value: %1 mils/100m", dElevCorrection));
                
                // Calculate elevation difference for altitude correction
                float elevationDifference = targetPos[1] - mortarPos[1];
                
                // Apply D_ELEV altitude correction
                if (dElevCorrection > 0 && elevationDifference != 0)
                {
                    float altitudeCorrection = (elevationDifference / 100.0) * dElevCorrection;
                    
                    // When firing downhill (negative elevation difference), we need to INCREASE elevation to shorten the shot
                    // When firing uphill (positive elevation difference), we need to DECREASE elevation to lengthen the shot
                    elevationMils = elevationMils - altitudeCorrection;
                    
                    Print(string.Format("[MORTAR DEBUG] Altitude correction: elev diff %1m / 100 * D_ELEV %2 = %3 mils", 
                        elevationDifference, dElevCorrection, altitudeCorrection));
                    Print(string.Format("[MORTAR DEBUG] Final elevation: %1 - %2 = %3 mils", 
                        elevationMils + altitudeCorrection, altitudeCorrection, elevationMils));
                }
                else
                {
                    Print("[MORTAR DEBUG] No altitude correction applied (D_ELEV = 0 or no elevation difference)");
                }
                
                // Check if final elevation is within limits
                if (elevationMils < 800.0 || elevationMils > 1515.0)
                {
                    Print(string.Format("[MORTAR DEBUG] WARNING: Final elevation %1 mils exceeds physical limits [800-1515]", elevationMils));
                    // Clamp to limits
                    if (elevationMils < 800.0) elevationMils = 800.0;
                    if (elevationMils > 1515.0) elevationMils = 1515.0;
                }
                
                // Convert mils to degrees (6400 mils = 360 degrees)
                float elevationDegrees = (elevationMils / 6400.0) * 360.0;
                
                Print(string.Format("[MORTAR DEBUG] Final elevation in degrees: %1°", elevationDegrees));
                
                // Calculate slant distance for display purposes
                float slantDistance = toTarget.Length();
                
                Print("[MORTAR DEBUG] ========== FIRING SOLUTION COMPLETE ==========");
                Print(string.Format("[MORTAR DEBUG] SUMMARY: %1 at %2m, Charge %3, Elevation %4 mils (%5°)", 
                    ammoType, horizontalDistance, charge, elevationMils, elevationDegrees));
                
                // Display firing solution
                string hint = string.Format(
                    "FIRING SOLUTION - %1\n\nRange: %2m\nSlant Range: %3m\nElev Diff: %4m\nAzimuth: %5°\nElevation: %6 mils (%7°)\n\nCharge: %8 rings\nTime of Flight: %9 sec",
                    ammoType,
                    horizontalDistance.ToString(0),
                    slantDistance.ToString(0),
                    elevationDifference.ToString(0),
                    azimuth.ToString(1),
                    elevationMils.ToString(0),
                    elevationDegrees.ToString(1),
                    charge.ToString(),
                    timeOfFlight.ToString(1)
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