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
        // Convert screen coordinates to world coordinates
        float worldX, worldY, heightAtPos;
        m_MapEntity.ScreenToWorld(selectedPos[0], selectedPos[2], worldX, worldY);
        
        heightAtPos = m_MapEntity.GetWorld().GetSurfaceY(worldX, worldY);
        
        vector targetPos = Vector(worldX, heightAtPos, worldY);
        vector mortarPos = m_eOwner.GetOrigin();
        
        // Calculate distance and bearing
        vector toTarget = targetPos - mortarPos;
        
        // Calculate slant distance (direct 3D distance)
        float slantDistance = toTarget.Length();
        
        // Calculate horizontal distance (for ballistic tables)
        float horizontalDistance = Math.Sqrt(toTarget[0] * toTarget[0] + toTarget[2] * toTarget[2]);
        
        // Calculate elevation difference
        float elevationDifference = targetPos[1] - mortarPos[1];
        
        // Use horizontal distance for all calculations
        float distance = horizontalDistance;
        
        // Calculate azimuth
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
            float siteAngleMils = 0;
            float fullSiteAngleMils = 0;
            
            bool solutionFound = MortarBallisticTables.CalculateSolution(ammoType, distance, elevationMils, timeOfFlight, charge);
            
            // Calculate site angle first if there's elevation difference
            if (horizontalDistance > 0)
            {
                // Calculate site angle correction
                // Site angle = arctan(elevation difference / horizontal distance)
                // Convert to mils (1 radian = 1000 milliradians, 2π radians = 6400 mils)
                float siteAngleRadians = Math.Atan2(elevationDifference, horizontalDistance);
                fullSiteAngleMils = (siteAngleRadians * 6400.0) / (2.0 * Math.PI);
                
                // Apply scaling factor - game physics don't fully match real mortar ballistics
                // 0.20 factor provides optimal balance for Arma Reforger's simplified ballistics
                const float SITE_ANGLE_SCALE_FACTOR = 0.20;
                siteAngleMils = fullSiteAngleMils * SITE_ANGLE_SCALE_FACTOR;
                
            }
            
            // If we have a solution, check if it's still valid after site angle correction
            if (solutionFound)
            {
                float baseElevation = elevationMils;
                // For mortars: 800 mils (45°) = max range, 1515 mils (85°) = min range
                // When firing downhill, gravity helps, so we need MORE elevation (higher angle) to reduce range
                // Negative elevation difference → negative site angle → subtract to get higher elevation
                float correctedElevation = elevationMils - siteAngleMils;
                
                // Check if corrected elevation is within physical limits
                if (correctedElevation >= 800.0 && correctedElevation <= 1515.0)
                {
                    // Solution is valid - apply the correction
                    elevationMils = correctedElevation;
                }
                else
                {
                    // Corrected elevation exceeds limits - try to find alternative charge
                    // Try to find a charge that works with site angle correction
                    bool alternativeFound = false;
                    for (int altCharge = 0; altCharge <= 4; altCharge++)
                    {
                        float altElevation, altTimeOfFlight;
                        if (MortarBallisticTables.GetSolutionForCharge(ammoType, distance, altCharge, altElevation, altTimeOfFlight))
                        {
                            float altCorrected = altElevation - siteAngleMils;
                            if (altCorrected >= 800.0 && altCorrected <= 1515.0)
                            {
                                // Found valid alternative
                                charge = altCharge;
                                elevationMils = altCorrected;
                                timeOfFlight = altTimeOfFlight;
                                alternativeFound = true;
                                break;
                            }
                        }
                    }
                    
                    if (!alternativeFound)
                    {
                        // No valid alternative - clamp to limits as last resort
                        if (correctedElevation > 1515.0)
                            elevationMils = 1515.0;
                        else if (correctedElevation < 800.0)
                            elevationMils = 800.0;
                    }
                }
            }
            
            if (!solutionFound)
            {
                // No solution found - likely due to elevation limits
                string hint = string.Format("NO VALID SOLUTION!\n\nTarget at %1m requires elevation\noutside mortar's 800-1515 mil limits.\n\nMove closer/further to target or\nuse different firing position.", 
                    distance.ToString(0));
                SCR_HintManagerComponent.ShowCustomHint(hint, "Elevation Limit", 8.0, false);
            }
            else
            {
                // Convert mils to degrees (6400 mils = 360 degrees)
                float elevationDegrees = (elevationMils / 6400.0) * 360.0;
                
                // Display results - build in parts due to parameter limit
                string rangeInfo = string.Format(
                    "Horizontal Range: %1m\nSlant Range: %2m\nElevation Diff: %3m\nSite Angle: %4 mils (scaled from %5)",
                    distance.ToString(0),
                    slantDistance.ToString(0),
                    elevationDifference.ToString(0),
                    siteAngleMils.ToString(0),
                    fullSiteAngleMils.ToString(0)
                );
                
                string hint = string.Format(
                    "FIRING SOLUTION - %1\n\n%2\nAzimuth: %3°\nElevation: %4 mils (%5°)\n\nCharge: %6 rings\nTime of Flight: %7 sec",
                    ammoType,
                    rangeInfo,
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