class PR_TempScreenDisplayClass : ScriptComponentClass { }

class PR_TempScreenDisplay : ScriptComponent
{
    [Attribute("", UIWidgets.ResourceNamePicker, "Layout", "layout")]
    protected ResourceName m_LayoutPath;

    [Attribute("", UIWidgets.ResourceNamePicker, "On Material", "emat")]
    protected ResourceName m_OnMaterialPath;

    [Attribute("", UIWidgets.ResourceNamePicker, "Off Material", "emat")]
    protected ResourceName m_OffMaterialPath;

    protected Widget m_wRoot;
    protected RTTextureWidget m_wRTTexture0;
    protected Widget m_wFrame0;

    protected bool m_IsDisplayOn = false;
    protected SignalsManagerComponent m_VehicleSignals;
    protected WCS_Armament_WeaponComponent m_ArmamentWeaponComponent;
    protected WeaponComponent m_WeaponComponent;

    protected float m_fTargetCheckInterval = (1000 / 15) * 0.001;
    protected float m_fTargetCheckIntervalTimer = float.MAX;

    protected const ResourceName DEFAULT_LAYOUT = "";

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

        m_VehicleSignals = SignalsManagerComponent.Cast(
            owner.FindComponent(SignalsManagerComponent)
        );

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

        if (!m_wRTTexture0 || !m_wFrame0)
            return;

        SetDisplayVisibility(false);
    }

    void TurnOnDisplay(IEntity owner)
    {
        Print("PR_TempScreenDisplayComponent: Turning on display.", LogLevel.NORMAL);
        if (m_IsDisplayOn)
            return;

        if (!m_wRoot)
            return;

        SetDisplayVisibility(true);
        m_IsDisplayOn = true;
        SetEventMask(owner, EntityEvent.FRAME);
    }

    void TurnOffDisplay(IEntity owner)
    {
        if (!m_IsDisplayOn)
            return;

        Print("PR_TempScreenDisplayComponent: Turning off display.", LogLevel.NORMAL);
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

    protected float m_SmoothedTQ = 0.0;
    protected float m_SmoothingFactor = 0.05;

    protected float m_CurrentPTIT = 500.0;
    protected float m_MaxPTIT = 943.0;
    protected float m_MinPTIT = 0.0;
    protected float m_PTITChangeRate = 5.0;

    protected ref array<Managed> weaponSlotComponents = new array<Managed>;

    override protected void EOnFrame(IEntity owner, float timeSlice)
    {
        UpdateDisplay(timeSlice);
    }

    void UpdateDisplay(float timeSlice)
    {
        if (!m_IsDisplayOn || !m_VehicleSignals)
            return;

        m_fTargetCheckIntervalTimer += timeSlice;
        if (m_fTargetCheckIntervalTimer < m_fTargetCheckInterval)
            return;
        m_fTargetCheckIntervalTimer = 0;

        int nrIndex = m_VehicleSignals.FindSignal("MainRotorRPM");
        int tqIndex = m_VehicleSignals.FindSignal("MainRotorCollective");
        int engineRPMIndex = m_VehicleSignals.FindSignal("EngineRPM");

        if (tqIndex != -1 && engineRPMIndex != -1 && nrIndex != -1)
        {
            float rpmValue = m_VehicleSignals.GetSignalValue(engineRPMIndex);
            float nrValue = m_VehicleSignals.GetSignalValue(nrIndex);
            float nrValuePerc = (nrValue / 225) * 100;
            Color nrColor = Color.Green;

            if (nrValuePerc < 97)
                nrColor = Color.Yellow;

            if (nrValuePerc > 101)
                nrColor = Color.Yellow;

            if (nrValuePerc > 106)
                nrColor = Color.Red;

            if (nrValuePerc < 1)
            {
                nrColor = Color.Gray;
                nrValuePerc = 0;
            }
        }
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