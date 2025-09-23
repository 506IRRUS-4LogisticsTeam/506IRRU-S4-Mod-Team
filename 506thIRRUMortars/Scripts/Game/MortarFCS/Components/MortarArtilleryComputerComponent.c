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
    protected int m_iSelectedCharge = -1;

    // System error handling
    protected bool m_bBSODActive = false;
    protected float m_fBSODTimer = 0.0;
    protected bool m_bBSODCooldown = false;

    // Turret control components
    protected TurretControllerComponent m_TurretController;
    protected TurretComponent m_TurretComponent;
    protected bool m_bAutoAimActive = false;
    protected bool m_bAutoAimSystemEnabled = false;
    protected vector m_vTargetAngles = vector.Zero;
    protected vector m_vCurrentAngles = vector.Zero;
    
    protected const float MIN_ELEVATION_MILS = 800.0;
    protected const float MAX_ELEVATION_MILS = 1515.0;
    protected const float MILS_TO_DEGREES = 360.0 / 6400.0;
    protected const float DEGREES_TO_MILS = 6400.0 / 360.0;
    protected const float ALTITUDE_CORRECTION_FACTOR = 100.0;

    // Configurable attributes for turret control
    [Attribute(defvalue: "true", desc: "Enable auto-aim system by default")]
    protected bool m_bDefaultAutoAimEnabled;

    [Attribute(defvalue: "true", desc: "Keep map open after selecting target")]
    protected bool m_bKeepMapOpen;

    [Attribute(defvalue: "2.0", desc: "Delay before closing map after selection (seconds)")]
    protected float m_fMapCloseDelay;

    [Attribute(defvalue: "90.0", desc: "Horizontal rotation speed in degrees per second")]
    protected float m_fHorizontalRotationSpeed;

    [Attribute(defvalue: "45.0", desc: "Vertical rotation speed in degrees per second")]
    protected float m_fVerticalRotationSpeed;
    
    //------------------------------------------------------------------------------------------------
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        m_Owner = owner;
        m_bAutoAimSystemEnabled = m_bDefaultAutoAimEnabled;
        MortarBallisticTables.Initialize();
    }
    
    //------------------------------------------------------------------------------------------------
    string GetSelectedAmmoType()
    {
        return m_sSelectedAmmoType;
    }

    //------------------------------------------------------------------------------------------------
    void ToggleAutoAim()
    {
        m_bAutoAimSystemEnabled = !m_bAutoAimSystemEnabled;

        if (!m_bAutoAimSystemEnabled && m_bAutoAimActive)
        {
            m_bAutoAimActive = false;
            ClearEventMask(m_Owner, EntityEvent.POSTFRAME);
        }

        UpdateMainHint();
    }

    //------------------------------------------------------------------------------------------------
    void CycleChargeSelection()
    {
        m_iSelectedCharge++;
        if (m_iSelectedCharge > 4)
            m_iSelectedCharge = -1;

        string chargeText;
        if (m_iSelectedCharge == -1)
        {
            chargeText = "AUTO";
        }
        else
        {
            array<ref MortarBallisticEntry> table = MortarBallisticTables.GetTable("HE", m_iSelectedCharge);
            if (table && table.Count() > 0)
            {
                MortarBallisticEntry firstEntry = table.Get(0);
                MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);
                chargeText = string.Format("CHARGE %1 (%2m-%3m)", m_iSelectedCharge, firstEntry.range, lastEntry.range);
            }
            else
            {
                chargeText = string.Format("CHARGE %1", m_iSelectedCharge);
            }
        }

        UpdateMainHint();
    }

    //------------------------------------------------------------------------------------------------
    void UpdateMainHint()
    {
        if (!m_bMapOpen)
            return;

        string autoAimStatus;
        if (m_bAutoAimSystemEnabled)
            autoAimStatus = "ENABLED";
        else
            autoAimStatus = "DISABLED";

        string chargeText;
        if (m_iSelectedCharge == -1)
        {
            chargeText = "AUTO";
        }
        else
        {
            array<ref MortarBallisticEntry> table = MortarBallisticTables.GetTable("HE", m_iSelectedCharge);
            if (table && table.Count() > 0)
            {
                MortarBallisticEntry firstEntry = table.Get(0);
                MortarBallisticEntry lastEntry = table.Get(table.Count() - 1);
                chargeText = string.Format("CHARGE %1 (%2-%3m)", m_iSelectedCharge, firstEntry.range, lastEntry.range);
            }
            else
            {
                chargeText = string.Format("CHARGE %1", m_iSelectedCharge);
            }
        }

        string controlHint;

        // Check if gamepad is being used
        InputManager inputManager = GetGame().GetInputManager();
        if (inputManager && inputManager.IsUsingMouseAndKeyboard())
        {
            controlHint = string.Format("Shell: %1 | Charge: %2 | Auto-Aim: %3\n\nClick map to auto-aim mortar\n[T] Toggle auto-aim | [C] Cycle charge | [X/ESC] Exit",
                m_sSelectedAmmoType, chargeText, autoAimStatus);
        }
        else
        {
            controlHint = string.Format("Shell: %1 | Charge: %2 | Auto-Aim: %3\n\nClick map to auto-aim mortar\n[L3] Toggle auto-aim | [RB] Cycle charge | [B] Exit",
                m_sSelectedAmmoType, chargeText, autoAimStatus);
        }

        SCR_HintManagerComponent.ShowCustomHint(controlHint, "Mortar Computer", 12.0, false);
    }
    
    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    void OpenComputer(SCR_MapEntity mapEntity, IEntity user)
    {
        if (!mapEntity)
            return;

        if (m_bMapOpen)
            return;

        // Check for rare system error
        if (!m_bBSODCooldown && Math.RandomInt(0, 1506) == 506)
        {
            ShowBSOD();
            m_bBSODCooldown = true;
            return;
        }

        m_TurretComponent = TurretComponent.Cast(m_Owner.FindComponent(TurretComponent));

        m_MapEntity = mapEntity;

        m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
        m_MapEntity.GetOnSelection().Insert(OnMapSelection);
        m_MapEntity.GetOnMapClose().Insert(OnMapClose);

        GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.MapMenu);
        m_bMapOpen = true;

        UpdateMainHint();
        m_bHintShown = false;

        SetEventMask(m_Owner, EntityEvent.POSTFRAME);
    }
    
    //------------------------------------------------------------------------------------------------
    void ShowBSOD()
    {
        m_bBSODActive = true;
        m_fBSODTimer = 8.0; // Display for 8 seconds

        string bsodText = "<color rgba='30,120,255,255'>████████████████████████████████████████████████</color>\n";
        bsodText = bsodText + "<color rgba='30,120,255,255'>███</color> <color rgba='255,255,255,255'>MORTAR COMPUTER ERROR</color> <color rgba='30,120,255,255'>███</color>\n";
        bsodText = bsodText + "<color rgba='30,120,255,255'>████████████████████████████████████████████████</color>\n\n";

        bsodText = bsodText + "<color rgba='255,255,255,255'>:(</color>\n\n";

        bsodText = bsodText + "<color rgba='255,255,255,255'>Your mortar ran into a problem and needs</color>\n";
        bsodText = bsodText + "<color rgba='255,255,255,255'>to restart. We're just collecting some</color>\n";
        bsodText = bsodText + "<color rgba='255,255,255,255'>error info, and then we'll restart for you.</color>\n\n";

        bsodText = bsodText + "<color rgba='255,255,255,255'>BALLISTIC_CALCULATION_FAULT</color>\n\n";

        bsodText = bsodText + "<color rgba='200,200,200,255'>Technical information:</color>\n";
        bsodText = bsodText + "<color rgba='200,200,200,255'>*** STOP: 0x00000ED (0x81MM0RT4R)</color>\n";
        bsodText = bsodText + "<color rgba='200,200,200,255'>*** MortarOS.sys - Address F73120AE</color>\n";
        bsodText = bsodText + "<color rgba='200,200,200,255'>*** Datestamp 506IRRU</color>\n\n";

        bsodText = bsodText + "<color rgba='255,255,255,255'>Memory dump complete.</color>\n\n";
        bsodText = bsodText + "<color rgba='255,255,0,255'>Press ALT+F4 to restart computer...</color>";

        SCR_HintManagerComponent.ShowCustomHint(bsodText, "SYSTEM ERROR", 10.0, true);

        // Listen for ESC key
        InputManager inputManager = GetGame().GetInputManager();
        if (inputManager)
            inputManager.AddActionListener("MapMortarExit", EActionTrigger.DOWN, OnBSODEscapeAction);

        // Enable frame updates to track timer
        SetEventMask(m_Owner, EntityEvent.POSTFRAME);
    }

    //------------------------------------------------------------------------------------------------
    protected void OnBSODEscapeAction(float value, EActionTrigger reason)
    {
        if (!m_bBSODActive)
            return;

        // Clean up BSOD
        m_bBSODActive = false;
        m_fBSODTimer = 0;
        m_bBSODCooldown = false; // Reset cooldown so it can happen again
        SCR_HintManagerComponent.ShowCustomHint("", "", 0.1, false); // Clear hint

        // Remove the listener
        InputManager inputManager = GetGame().GetInputManager();
        if (inputManager)
            inputManager.RemoveActionListener("MapMortarExit", EActionTrigger.DOWN, OnBSODEscapeAction);

        ClearEventMask(m_Owner, EntityEvent.POSTFRAME);
    }

    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    void CloseComputer()
    {
        if (!m_bMapOpen)
            return;
            
        GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
        m_bMapOpen = false;
    }
    
    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    protected void OnMapOpen(MapConfiguration config)
    {
        InputManager inputManager = GetGame().GetInputManager();
        if (inputManager)
        {
            inputManager.AddActionListener("MapMortarAim", EActionTrigger.DOWN, OnToggleAutoAimAction);
            inputManager.AddActionListener("MapMortarCharge", EActionTrigger.DOWN, OnCycleChargeAction);
            inputManager.AddActionListener("MapMortarExit", EActionTrigger.DOWN, OnExitMapAction);
        }
    }

    //------------------------------------------------------------------------------------------------
    protected void OnToggleAutoAimAction(float value, EActionTrigger reason)
    {
        ToggleAutoAim();
    }

    //------------------------------------------------------------------------------------------------
    protected void OnCycleChargeAction(float value, EActionTrigger reason)
    {
        CycleChargeSelection();
    }

    //------------------------------------------------------------------------------------------------
    protected void OnExitMapAction(float value, EActionTrigger reason)
    {
        CloseComputer();
    }

    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    protected void OnMapSelection(vector selectedPos)
    {
        if (!m_MapEntity || !m_Owner)
            return;

        float worldX, worldY;
        m_MapEntity.ScreenToWorld(selectedPos[0], selectedPos[2], worldX, worldY);

        float heightAtPos = m_MapEntity.GetWorld().GetSurfaceY(worldX, worldY);

        vector targetPos = Vector(worldX, heightAtPos, worldY);
        vector mortarPos = m_Owner.GetOrigin();
        
        vector toTarget = targetPos - mortarPos;
        float horizontalDistance = Math.Sqrt(toTarget[0] * toTarget[0] + toTarget[2] * toTarget[2]);

        float azimuth = Math.Atan2(toTarget[0], toTarget[2]) * Math.RAD2DEG;
        if (azimuth < 0)
            azimuth = azimuth + 360;

        string ammoType = GetSelectedAmmoType();
        float minRange, maxRange;
        MortarBallisticTables.GetMinMaxRange(ammoType, minRange, maxRange);
        
        if (horizontalDistance < minRange)
        {
            DisplayRangeError(true, minRange, maxRange, horizontalDistance);
        }
        else if (horizontalDistance > maxRange)
        {
            DisplayRangeError(false, minRange, maxRange, horizontalDistance);
        }
        else
        {
            CalculateAndDisplaySolution(ammoType, horizontalDistance, toTarget, targetPos, mortarPos, azimuth);
        }

    }
    
    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    protected void OnMapClose(MapConfiguration config)
    {
        if (!m_MapEntity)
            return;

        InputManager inputManager = GetGame().GetInputManager();
        if (inputManager)
        {
            inputManager.RemoveActionListener("MapMortarAim", EActionTrigger.DOWN, OnToggleAutoAimAction);
            inputManager.RemoveActionListener("MapMortarCharge", EActionTrigger.DOWN, OnCycleChargeAction);
            inputManager.RemoveActionListener("MapMortarExit", EActionTrigger.DOWN, OnExitMapAction);
        }

        m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
        m_MapEntity.GetOnSelection().Remove(OnMapSelection);
        m_MapEntity.GetOnMapClose().Remove(OnMapClose);

        m_bHintShown = false;
        m_bMapOpen = false;
    }
    
    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    protected void CalculateAndDisplaySolution(string ammoType, float horizontalDistance, vector toTarget, vector targetPos, vector mortarPos, float azimuth)
    {
        float elevationMils;
        float timeOfFlight;
        int charge;
        int dElevCorrection;

        bool solutionFound;

        if (m_iSelectedCharge == -1)
        {
            solutionFound = MortarBallisticTables.CalculateSolution(ammoType, horizontalDistance, elevationMils, timeOfFlight, charge, dElevCorrection);
        }
        else
        {
            charge = m_iSelectedCharge;
            solutionFound = MortarBallisticTables.CalculateSolutionForCharge(ammoType, charge, horizontalDistance, elevationMils, timeOfFlight, dElevCorrection);
        }
        
        if (!solutionFound)
        {
            DisplayNoSolutionHint(horizontalDistance);
            return;
        }
        
        float elevationDifference = targetPos[1] - mortarPos[1];
        elevationMils = ApplyAltitudeCorrection(elevationMils, elevationDifference, dElevCorrection);
        elevationMils = ClampElevation(elevationMils);
        
        DisplayFiringSolution(ammoType, horizontalDistance, toTarget, azimuth, elevationMils, elevationDifference, charge, timeOfFlight);

        if (m_TurretComponent && m_bAutoAimSystemEnabled)
        {
            IEntity turretEntity = m_TurretComponent.GetOwner();
            if (!turretEntity)
                return;

            vector turretMat[4];
            turretEntity.GetTransform(turretMat);

            float quat[4];
            Math3D.MatrixToQuat(turretMat, quat);
            float quatInv[4];
            Math3D.QuatInverse(quatInv, quat);

            float elevationAngleDeg = elevationMils * MILS_TO_DEGREES;

            float azimuthRad = azimuth * Math.DEG2RAD;
            float elevationRad = elevationAngleDeg * Math.DEG2RAD;

            vector worldDirection;
            worldDirection[0] = Math.Sin(azimuthRad) * Math.Cos(elevationRad);
            worldDirection[1] = Math.Sin(elevationRad);
            worldDirection[2] = Math.Cos(azimuthRad) * Math.Cos(elevationRad);

            vector dirLocal = SCR_Math3D.QuatMultiply(quatInv, worldDirection);

            vector desiredAngles = dirLocal.VectorToAngles();

            m_vTargetAngles = Vector(desiredAngles[0] * Math.DEG2RAD, desiredAngles[1] * Math.DEG2RAD, 0);
            m_bAutoAimActive = true;
        }
    }
    
    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
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
    //------------------------------------------------------------------------------------------------
    protected float ApplyAltitudeCorrection(float elevationMils, float elevationDifference, int dElevCorrection)
    {
        if (dElevCorrection <= 0 || elevationDifference == 0)
            return elevationMils;
            
        float altitudeCorrection = (elevationDifference / ALTITUDE_CORRECTION_FACTOR) * dElevCorrection;
        return elevationMils - altitudeCorrection;
    }
    
    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    protected float ClampElevation(float elevationMils)
    {
        if (elevationMils < MIN_ELEVATION_MILS)
            return MIN_ELEVATION_MILS;
        if (elevationMils > MAX_ELEVATION_MILS)
            return MAX_ELEVATION_MILS;
        return elevationMils;
    }
    
    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
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
    //------------------------------------------------------------------------------------------------
    protected void DisplayNoSolutionHint(float distance)
    {
        string hint = string.Format(
            "NO VALID SOLUTION!\n\nTarget at %1m requires elevation\noutside mortar's physical limits.\n\nMove closer/further to target or\nuse different firing position.",
            distance.ToString(0)
        );
        SCR_HintManagerComponent.ShowCustomHint(hint, "Elevation Limit", 8.0, false);
    }

    //------------------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    override void EOnPostFrame(IEntity owner, float timeSlice)
    {
        // Handle BSOD timer
        if (m_bBSODActive)
        {
            m_fBSODTimer -= timeSlice;
            if (m_fBSODTimer <= 0)
            {
                OnBSODEscapeAction(0, EActionTrigger.DOWN); // Auto-dismiss after timer
            }
            return;
        }

        // Handle auto-aim rotation
        if (!m_bAutoAimActive || !m_TurretComponent)
        {
            m_bAutoAimActive = false;
            return;
        }

        if (m_vTargetAngles == vector.Zero)
        {
            m_bAutoAimActive = false;
            return;
        }

        m_TurretComponent.SetAimingRotation(m_vTargetAngles);
        m_bAutoAimActive = false;

        if (!m_bMapOpen)
            ClearEventMask(owner, EntityEvent.POSTFRAME);
    }

    //------------------------------------------------------------------------------------------------
    override void OnDelete(IEntity owner)
    {
        ClearEventMask(owner, EntityEvent.POSTFRAME);
        super.OnDelete(owner);
    }
}