[EntityEditorProps(category: "GameScripted/Weapons", description: "Mortar artillery computer component")]
class IRRU_MortarArtilleryComputerComponentClass : ScriptComponentClass 
{
}

//! Mortar artillery computer component for calculating firing solutions
class IRRU_MortarArtilleryComputerComponent : ScriptComponent
{
    protected SCR_MapEntity m_MapEntity;
    protected IEntity m_Owner;
    protected bool m_bMapOpen = false;
    protected string m_sSelectedAmmoType = "HE";
    protected ref array<string> m_aAvailableAmmoTypes = {"HE", "Smoke", "Illumination"};
    protected bool m_bHintShown = false;

    // Turret control components
    protected TurretControllerComponent m_TurretController;
    protected TurretComponent m_TurretComponent;
    protected bool m_bAutoAimEnabled = false;
    protected vector m_vTargetAngles = vector.Zero; // [azimuth, elevation] in radians
    protected vector m_vCurrentAngles = vector.Zero;
    
    protected const float MIN_ELEVATION_MILS = 800.0;
    protected const float MAX_ELEVATION_MILS = 1515.0;
    protected const float MILS_TO_DEGREES = 360.0 / 6400.0;
    protected const float DEGREES_TO_MILS = 6400.0 / 360.0;
    protected const float ALTITUDE_CORRECTION_FACTOR = 100.0;

    // Configurable attributes for turret control
    [Attribute(defvalue: "false", desc: "Keep map open after selecting target")]
    protected bool m_bKeepMapOpen;

    [Attribute(defvalue: "2.0", desc: "Delay before closing map after selection (seconds)")]
    protected float m_fMapCloseDelay;

    [Attribute(defvalue: "90.0", desc: "Horizontal rotation speed in degrees per second")]
    protected float m_fHorizontalRotationSpeed;

    [Attribute(defvalue: "45.0", desc: "Vertical rotation speed in degrees per second")]
    protected float m_fVerticalRotationSpeed;
    
    //------------------------------------------------------------------------------------------------
    //! Initialize component and ballistic tables
    //! \param owner Entity that owns this component
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        
        m_Owner = owner;
        MortarBallisticTables.Initialize();
    }
    
    //------------------------------------------------------------------------------------------------
    //! Get currently selected ammo type
    //! \return Current ammo type string
    protected string GetCurrentAmmoType()
    {
        return m_sSelectedAmmoType;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Cycle through available ammunition types
    void CycleAmmoType()
    {
        if (!m_aAvailableAmmoTypes || m_aAvailableAmmoTypes.Count() == 0)
            return;
            
        int currentIndex = m_aAvailableAmmoTypes.Find(m_sSelectedAmmoType);
        currentIndex++;
        
        if (currentIndex >= m_aAvailableAmmoTypes.Count())
            currentIndex = 0;
            
        m_sSelectedAmmoType = m_aAvailableAmmoTypes.Get(currentIndex);
        
        string hint = string.Format("Shell Type: %1", m_sSelectedAmmoType);
        SCR_HintManagerComponent.ShowCustomHint(hint, "Mortar Computer", 3.0, false);
    }
    
    //------------------------------------------------------------------------------------------------
    //! Get selected ammunition type
    //! \return Selected ammo type string
    string GetSelectedAmmoType()
    {
        return m_sSelectedAmmoType;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Open mortar computer interface
    //! \param mapEntity Map entity to use for target selection
    //! \param user Entity using the computer
    void OpenComputer(SCR_MapEntity mapEntity, IEntity user)
    {
        if (!mapEntity)
            return;

        if (m_bMapOpen)
            return;

        // Try to get turret component directly from the mortar entity
        m_TurretComponent = TurretComponent.Cast(m_Owner.FindComponent(TurretComponent));

        if (!m_TurretComponent)
        {
            Print("[MORTAR] ERROR: Could not find TurretComponent on mortar entity");
        }
        else
        {
            Print("[MORTAR] TurretComponent found successfully");
        }

        m_MapEntity = mapEntity;
        
        m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
        m_MapEntity.GetOnSelection().Insert(OnMapSelection);
        m_MapEntity.GetOnMapClose().Insert(OnMapClose);
        
        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MapMenu);
        m_bMapOpen = true;

        string controlHint = string.Format("Current Shell: %1\n\nClick map to auto-aim mortar", m_sSelectedAmmoType);
        SCR_HintManagerComponent.ShowCustomHint(controlHint, "Mortar Computer", 8.0, false);
        m_bHintShown = false;

        // Enable frame updates for smooth rotation
        SetEventMask(m_Owner, EntityEvent.POSTFRAME);
    }
    
    //------------------------------------------------------------------------------------------------
    //! Close mortar computer interface
    void CloseComputer()
    {
        if (!m_bMapOpen)
            return;
            
        GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
        m_bMapOpen = false;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Handle map open event
    //! \param config Map configuration
    protected void OnMapOpen(MapConfiguration config)
    {
    }
    
    //------------------------------------------------------------------------------------------------
    //! Calculate firing solution for selected map position
    //! \param selectedPos Selected position on map
    protected void OnMapSelection(vector selectedPos)
    {
        Print("[MORTAR] OnMapSelection called");

        if (!m_MapEntity || !m_Owner)
        {
            Print("[MORTAR] ERROR: m_MapEntity or m_Owner is null");
            return;
        }

        float worldX, worldY;
        m_MapEntity.ScreenToWorld(selectedPos[0], selectedPos[2], worldX, worldY);
        Print(string.Format("[MORTAR] Map clicked at world pos: %1, %2", worldX, worldY));
        
        float heightAtPos = m_MapEntity.GetWorld().GetSurfaceY(worldX, worldY);
        
        vector targetPos = Vector(worldX, heightAtPos, worldY);
        vector mortarPos = m_Owner.GetOrigin();
        
        vector toTarget = targetPos - mortarPos;
        float horizontalDistance = Math.Sqrt(toTarget[0] * toTarget[0] + toTarget[2] * toTarget[2]);
        
        float azimuth = Math.Atan2(toTarget[0], toTarget[2]) * Math.RAD2DEG;
        if (azimuth < 0) 
            azimuth = azimuth + 360;
        
        string ammoType = GetCurrentAmmoType();
        float minRange, maxRange;
        MortarBallisticTables.GetMinMaxRange(ammoType, minRange, maxRange);
        
        Print(string.Format("[MORTAR] Distance: %1m, Min: %2m, Max: %3m", horizontalDistance, minRange, maxRange));

        if (horizontalDistance < minRange)
        {
            Print("[MORTAR] Target too close");
            DisplayRangeError(true, minRange, maxRange, horizontalDistance);
        }
        else if (horizontalDistance > maxRange)
        {
            Print("[MORTAR] Target too far");
            DisplayRangeError(false, minRange, maxRange, horizontalDistance);
        }
        else
        {
            Print("[MORTAR] Calculating firing solution");
            CalculateAndDisplaySolution(ammoType, horizontalDistance, toTarget, targetPos, mortarPos, azimuth);
        }

        if (!m_bKeepMapOpen)
        {
            if (m_fMapCloseDelay > 0)
            {
                // Close map after delay
                GetGame().GetCallqueue().CallLater(CloseComputer, m_fMapCloseDelay * 1000, false);
            }
            else
            {
                // Close map immediately
                CloseComputer();
            }
        }
    }
    
    //------------------------------------------------------------------------------------------------
    //! Handle map close event
    //! \param config Map configuration
    protected void OnMapClose(MapConfiguration config)
    {
        if (!m_MapEntity)
            return;
            
        m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
        m_MapEntity.GetOnSelection().Remove(OnMapSelection);
        m_MapEntity.GetOnMapClose().Remove(OnMapClose);
        
        m_bHintShown = false;
        m_bMapOpen = false;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Calculate and display firing solution
    //! \param ammoType Type of ammunition
    //! \param horizontalDistance Horizontal distance to target
    //! \param toTarget Vector from mortar to target
    //! \param targetPos Target position
    //! \param mortarPos Mortar position
    //! \param azimuth Azimuth in degrees
    protected void CalculateAndDisplaySolution(string ammoType, float horizontalDistance, vector toTarget, vector targetPos, vector mortarPos, float azimuth)
    {
        float elevationMils;
        float timeOfFlight;
        int charge;
        int dElevCorrection;
        
        bool solutionFound = MortarBallisticTables.CalculateSolution(ammoType, horizontalDistance, elevationMils, timeOfFlight, charge, dElevCorrection);
        
        if (!solutionFound)
        {
            DisplayNoSolutionHint(horizontalDistance);
            return;
        }
        
        float elevationDifference = targetPos[1] - mortarPos[1];
        elevationMils = ApplyAltitudeCorrection(elevationMils, elevationDifference, dElevCorrection);
        elevationMils = ClampElevation(elevationMils);
        
        DisplayFiringSolution(ammoType, horizontalDistance, toTarget, azimuth, elevationMils, elevationDifference, charge, timeOfFlight);

        // Enable auto-aiming if turret is available
        if (m_TurretComponent)
        {
            // Convert azimuth and elevation to radians for turret control
            float azimuthRad = azimuth * Math.DEG2RAD;
            float elevationRad = elevationMils * MILS_TO_DEGREES * Math.DEG2RAD;

            m_vTargetAngles = Vector(azimuthRad, elevationRad, 0);
            m_bAutoAimEnabled = true;

            Print(string.Format("[MORTAR] Target angles set - Azimuth: %1 rad (%2 deg), Elevation: %3 rad (%4 deg)",
                azimuthRad, azimuth, elevationRad, elevationMils * MILS_TO_DEGREES));
        }
        else
        {
            Print("[MORTAR] WARNING: TurretComponent not available, auto-aim disabled");
        }
    }
    
    //------------------------------------------------------------------------------------------------
    //! Display range error message
    //! \param tooClose True if target is too close, false if too far
    //! \param minRange Minimum range in meters
    //! \param maxRange Maximum range in meters
    //! \param distance Current target distance
    protected void DisplayRangeError(bool tooClose, float minRange, float maxRange, float distance)
    {
        string hint;
        
        if (tooClose)
        {
            hint = string.Format(
                "TARGET TOO CLOSE!\n\nMinimum Range: %1m\nTarget Distance: %2m\n\nSelect a target further away!",
                minRange.ToString(0),
                distance.ToString(0)
            );
        }
        else
        {
            hint = string.Format(
                "TARGET OUT OF RANGE!\n\nMaximum Range: %1m\nTarget Distance: %2m\n\nSelect a closer target!",
                maxRange.ToString(0),
                distance.ToString(0)
            );
        }
        
        SCR_HintManagerComponent.ShowCustomHint(hint, "Range Error", 8.0, false);
    }
    
    //------------------------------------------------------------------------------------------------
    //! Apply altitude correction to elevation
    //! \param elevationMils Base elevation in mils
    //! \param elevationDifference Height difference between mortar and target
    //! \param dElevCorrection Correction factor from ballistic tables
    //! \return Corrected elevation in mils
    protected float ApplyAltitudeCorrection(float elevationMils, float elevationDifference, int dElevCorrection)
    {
        if (dElevCorrection <= 0 || elevationDifference == 0)
            return elevationMils;
            
        float altitudeCorrection = (elevationDifference / ALTITUDE_CORRECTION_FACTOR) * dElevCorrection;
        return elevationMils - altitudeCorrection;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Clamp elevation to physical mortar limits
    //! \param elevationMils Elevation to clamp
    //! \return Clamped elevation value
    protected float ClampElevation(float elevationMils)
    {
        if (elevationMils < MIN_ELEVATION_MILS)
            return MIN_ELEVATION_MILS;
        if (elevationMils > MAX_ELEVATION_MILS)
            return MAX_ELEVATION_MILS;
        return elevationMils;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Display firing solution to player
    //! \param ammoType Type of ammunition
    //! \param horizontalDistance Horizontal distance to target
    //! \param toTarget Vector from mortar to target
    //! \param azimuth Azimuth in degrees
    //! \param elevationMils Elevation in mils
    //! \param elevationDifference Height difference
    //! \param charge Number of charge rings
    //! \param timeOfFlight Time of flight in seconds
    protected void DisplayFiringSolution(string ammoType, float horizontalDistance, vector toTarget, float azimuth, float elevationMils, float elevationDifference, int charge, float timeOfFlight)
    {
        float elevationDegrees = elevationMils * MILS_TO_DEGREES;
        float slantDistance = toTarget.Length();
        float azimuthMils = azimuth * DEGREES_TO_MILS;
        
        string hint1 = string.Format(
            "FIRING SOLUTION - %1\n\nRange: %2m\nSlant Range: %3m\nElev Diff: %4m\nAzimuth: %5 mils (%6°)",
            ammoType,
            horizontalDistance.ToString(0),
            slantDistance.ToString(0),
            elevationDifference.ToString(0),
            azimuthMils.ToString(0),
            azimuth.ToString(1)
        );
        
        string hint2 = string.Format(
            "\nElevation: %1 mils (%2°)\n\nCharge: %3 rings\nTime of Flight: %4 sec",
            elevationMils.ToString(0),
            elevationDegrees.ToString(1),
            charge.ToString(),
            timeOfFlight.ToString(1)
        );
        
        SCR_HintManagerComponent.ShowCustomHint(hint1 + hint2, "Mortar Computer", 30.0, false);
        m_bHintShown = true;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Display no solution hint
    //! \param distance Distance to target
    protected void DisplayNoSolutionHint(float distance)
    {
        string hint = string.Format(
            "NO VALID SOLUTION!\n\nTarget at %1m requires elevation\noutside mortar's physical limits.\n\nMove closer/further to target or\nuse different firing position.",
            distance.ToString(0)
        );
        SCR_HintManagerComponent.ShowCustomHint(hint, "Elevation Limit", 8.0, false);
    }

    //------------------------------------------------------------------------------------------------
    //! Frame update for smooth turret rotation
    //! \param owner Entity that owns this component
    //! \param timeSlice Time since last frame
    override void EOnPostFrame(IEntity owner, float timeSlice)
    {
        if (!m_bAutoAimEnabled)
        {
            return;
        }

        if (!m_TurretComponent)
        {
            Print("[MORTAR] EOnPostFrame: No TurretComponent");
            m_bAutoAimEnabled = false;
            return;
        }

        // Validate target angles are set
        if (m_vTargetAngles == vector.Zero)
        {
            Print("[MORTAR] ERROR: Target angles not set");
            m_bAutoAimEnabled = false;
            return;
        }

        // Get current turret angles (returned in DEGREES)
        m_vCurrentAngles = m_TurretComponent.GetAimingRotation();

        // Convert current angles from degrees to radians for comparison
        vector currentAnglesRad = Vector(m_vCurrentAngles[0] * Math.DEG2RAD, m_vCurrentAngles[1] * Math.DEG2RAD, 0);

        // Calculate angle difference (both in radians)
        vector angleDiff = m_vTargetAngles - currentAnglesRad;

        // Normalize azimuth difference to shortest path
        if (angleDiff[0] > Math.PI)
            angleDiff[0] = angleDiff[0] - Math.PI2;
        else if (angleDiff[0] < -Math.PI)
            angleDiff[0] = angleDiff[0] + Math.PI2;

        // Apply rotation speed limits (in radians)
        float maxYaw = m_fHorizontalRotationSpeed * Math.DEG2RAD * timeSlice;
        float maxPitch = m_fVerticalRotationSpeed * Math.DEG2RAD * timeSlice;

        angleDiff[0] = Math.Clamp(angleDiff[0], -maxYaw, maxYaw);
        angleDiff[1] = Math.Clamp(angleDiff[1], -maxPitch, maxPitch);

        // Apply rotation (SetAimingRotation expects RADIANS)
        vector newAnglesRad = currentAnglesRad + angleDiff;
        m_TurretComponent.SetAimingRotation(newAnglesRad);

        // Check if we've reached the target
        float azimuthError = Math.AbsFloat(m_vTargetAngles[0] - newAnglesRad[0]);
        float elevationError = Math.AbsFloat(m_vTargetAngles[1] - newAnglesRad[1]);

        if (azimuthError < 0.002 && elevationError < 0.002) // Within ~0.1 degrees
        {
            m_bAutoAimEnabled = false;
            Print("[MORTAR] Target angles reached, auto-aim disabled");

            if (!m_bMapOpen)
                ClearEventMask(owner, EntityEvent.POSTFRAME);
        }
    }

    //------------------------------------------------------------------------------------------------
    //! Cleanup on deletion
    //! \param owner Entity that owns this component
    override void OnDelete(IEntity owner)
    {
        ClearEventMask(owner, EntityEvent.POSTFRAME);
        super.OnDelete(owner);
    }
}