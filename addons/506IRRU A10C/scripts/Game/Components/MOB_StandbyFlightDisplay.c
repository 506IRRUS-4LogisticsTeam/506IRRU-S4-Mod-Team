//This is the work of a very sleep deprived individual
//How is there no search hits for "create layout" or "layout signals" in the Arma Discord!?!?!
//This game badly needs a good web docs. See Discord.JS for example - that is good stuff.
//Given a super powerful system but do one wrong thing and you get illegal read error with no trace.
class MOB_StandbyFlightDisplayClass: ScriptComponentClass
{
}

class MOB_StandbyFlightDisplay : ScriptComponent
{
    [Attribute("", UIWidgets.ResourceNamePicker, "Layout", "layout")]
    protected ResourceName m_LayoutPath;

    protected Widget m_wRoot;
    protected RTTextureWidget m_wRTTexture0;
    protected Widget m_wFrame0;
    protected OverlayWidget m_wOverlay0;

    protected bool m_IsDisplayOn = false;
    protected SignalsManagerComponent m_VehicleSignals;

    protected const ResourceName DEFAULT_LAYOUT = "{7270849B554E6826}UI/layouts/StandbyFlightDisplay.layout";

    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);

        if (SCR_Global.IsEditMode())
            return;
    }

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (m_LayoutPath.IsEmpty())
            m_LayoutPath = DEFAULT_LAYOUT;

        m_VehicleSignals = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));

        GetGame().GetCallqueue().CallLater(InitializeLayout, 0, false, owner);
    }

    protected void InitializeLayout(IEntity owner)
    {
        if (!GetGame().GetWorkspace())
            return;

        m_wRoot = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath);
        if (!m_wRoot)
            return;

        m_wRTTexture0 = RTTextureWidget.Cast(m_wRoot.FindAnyWidget("RTTexture0"));
        m_wFrame0 = m_wRoot.FindAnyWidget("Frame0");
        m_wOverlay0 = OverlayWidget.Cast(m_wRoot.FindAnyWidget("Overlay0"));

        if (!m_wRTTexture0 || !m_wFrame0 || !m_wOverlay0)
            return;

        SetDisplayVisibility(false);
    }

    void TurnOnDisplay(IEntity owner)
    {
        Print("MOB_StandbyFlightDisplayComponent: Turning on display.", LogLevel.NORMAL);
        if (m_IsDisplayOn)
            return;

        if (!m_wRoot)
            return;

        SetDisplayVisibility(true);
        m_IsDisplayOn = true;
        UpdateDisplay();
    }

    void TurnOffDisplay(IEntity owner)
    {
        if (!m_IsDisplayOn)
            return;

        Print("MOB_StandbyFlightDisplayComponent: Turning off display.", LogLevel.NORMAL);
        SetDisplayVisibility(false);
        m_IsDisplayOn = false;
    }

    protected void SetDisplayVisibility(bool isVisible)
    {
        if (!m_wRoot)
            return;

        if (!isVisible)
        {
            m_wRoot.SetVisible(false);
            if (m_wRTTexture0)
                m_wRTTexture0.SetVisible(false);
            m_wRoot.SetEnabled(false);
        }
        else
        {
            m_wRoot.SetOpacity(1);
            m_wRoot.SetVisible(true);
            m_wRoot.SetEnabled(true);
        }
    }

    protected float METERS_TO_FEET = 3.2808399;

    void UpdateDisplay()
    {
        if (!m_IsDisplayOn || !m_VehicleSignals)
            return;

        int airspeedIndex = m_VehicleSignals.FindSignal("Airspeed");
        if (airspeedIndex != -1)
        {
            float airspeed = m_VehicleSignals.GetSignalValue(airspeedIndex);
            int airspeedFlat = Math.Floor(airspeed * 0.54);

            TextWidget airspeedText = TextWidget.Cast(m_wRoot.FindAnyWidget("AirspeedText"));
            if (airspeedText)
                airspeedText.SetText(airspeedFlat.ToString());
        }

        int headingIndex = m_VehicleSignals.FindSignal("yawangle");
        if (headingIndex != -1)
        {
            float heading = m_VehicleSignals.GetSignalValue(headingIndex);
            if (heading < 0)
                heading += 360;
            int headingFlat = Math.Floor(heading);

            TextWidget headingText = TextWidget.Cast(m_wRoot.FindAnyWidget("HeadingText"));
            if (headingText)
                headingText.SetText(headingFlat.ToString());
        }

        int altitudeIndex = m_VehicleSignals.FindSignal("Altitude");
        if (altitudeIndex != -1)
        {
            float altitude = m_VehicleSignals.GetSignalValue(altitudeIndex);
            int altitudeFlat = Math.Floor(altitude * METERS_TO_FEET);

            TextWidget altitudeText = TextWidget.Cast(m_wRoot.FindAnyWidget("AltitudeText"));
            if (altitudeText)
                altitudeText.SetText(altitudeFlat.ToString());
        }

        int pitchangleIndex = m_VehicleSignals.FindSignal("Pitch");
        int rollangleIndex = m_VehicleSignals.FindSignal("Roll");
        if (pitchangleIndex != -1 && rollangleIndex != -1)
        {
            float pitchAngle = m_VehicleSignals.GetSignalValue(pitchangleIndex);
            float rollAngle = m_VehicleSignals.GetSignalValue(rollangleIndex);

            ImageWidget attitudeIndicator = ImageWidget.Cast(m_wFrame0.FindAnyWidget("AttitudeIndicator"));
            if (attitudeIndicator)
            {
                float verticalScale = 10.8;
                vector offsets = CalculateOffsets(pitchAngle, rollAngle, verticalScale);

                FrameSlot.SetPos(attitudeIndicator, -256 + offsets[0], -768 + offsets[1]);
                attitudeIndicator.SetRotation(-rollAngle);
            }
        }

        GetGame().GetCallqueue().CallLater(UpdateDisplay, (1000 / 30), false);
    }

    vector CalculateOffsets(float pitchAngle, float rollAngle, float verticalScale)
    {
        float pitchRad = Math.PI * pitchAngle / 180.0;
        float rollRad = Math.PI * rollAngle / 180.0;

        float verticalOffset = pitchAngle * verticalScale;
        float horizontalOffset = verticalOffset * Math.Sin(rollRad);
        verticalOffset *= Math.Cos(rollRad);

        return Vector(horizontalOffset, verticalOffset, 0);
    }

    bool IsDisplayOn()
    {
        return m_IsDisplayOn;
    }

    override void OnDelete(IEntity owner)
    {
        super.OnDelete(owner);

        if (m_wRoot)
        {
            m_wRoot.RemoveFromHierarchy();
            m_wRoot = null;
        }
    }
}