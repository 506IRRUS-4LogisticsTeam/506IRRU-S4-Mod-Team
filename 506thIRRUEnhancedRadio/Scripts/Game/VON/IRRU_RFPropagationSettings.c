[BaseContainerProps(configRoot: true)]
class IRRU_RFPropagationSettings
{
    protected static ref IRRU_RFPropagationSettings s_Instance;
    protected static bool s_bInitialized = false;

    [Attribute(defvalue: "0", desc: "Enable RF propagation simulation (terrain, obstacles, distance affects signal quality)", category: "RF Propagation", uiwidget: UIWidgets.CheckBox)]
    bool m_bRFPropagationEnabled;

    [Attribute(defvalue: "0", desc: "Enable debug output to RPT log", category: "RF Propagation", uiwidget: UIWidgets.CheckBox)]
    bool m_bDebugEnabled;

    //------------------------------------------------------------------------------------------------
    static IRRU_RFPropagationSettings GetInstance()
    {
        if (!s_Instance)
        {
            bool configLoaded = false;
            Resource holder = BaseContainerTools.LoadContainer("{3DE007FE1017EE5D}Configs/VON_Settings/506th_VONConfig.conf");
            if (holder && holder.GetResource())
            {
                BaseContainer container = holder.GetResource().ToBaseContainer();
                if (container)
                {
                    s_Instance = IRRU_RFPropagationSettings.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
                    if (s_Instance)
                        configLoaded = true;
                }
            }

            if (!s_Instance)
            {
                s_Instance = new IRRU_RFPropagationSettings();
                s_Instance.m_bRFPropagationEnabled = false;
                s_Instance.m_bDebugEnabled = false;
            }

            if (!s_bInitialized)
            {
                s_bInitialized = true;
                if (configLoaded)
                    Print("[RFPropagation] Config loaded successfully");
                else
                    Print("[RFPropagation] Config not found, using defaults");
                Print(string.Format("[RFPropagation] RF Propagation enabled: %1", s_Instance.m_bRFPropagationEnabled));
                Print(string.Format("[RFPropagation] Debug mode: %1", s_Instance.m_bDebugEnabled));
            }
        }

        return s_Instance;
    }

    //------------------------------------------------------------------------------------------------
    static bool IsRFPropagationEnabled()
    {
        IRRU_RFPropagationSettings settings = GetInstance();
        if (settings)
            return settings.m_bRFPropagationEnabled;
        return false;
    }

    //------------------------------------------------------------------------------------------------
    static bool IsDebugEnabled()
    {
        IRRU_RFPropagationSettings settings = GetInstance();
        if (settings)
            return settings.m_bDebugEnabled;
        return false;
    }

    //------------------------------------------------------------------------------------------------
    static void SetRFPropagationEnabled(bool enabled)
    {
        IRRU_RFPropagationSettings settings = GetInstance();
        if (settings)
        {
            settings.m_bRFPropagationEnabled = enabled;
            Print(string.Format("[RFPropagation] RF Propagation set to %1", enabled));
        }
    }

    //------------------------------------------------------------------------------------------------
    static void SetDebugEnabled(bool enabled)
    {
        IRRU_RFPropagationSettings settings = GetInstance();
        if (settings)
        {
            settings.m_bDebugEnabled = enabled;
            Print(string.Format("[RFPropagation] Debug mode set to %1", enabled));
        }
    }
}
