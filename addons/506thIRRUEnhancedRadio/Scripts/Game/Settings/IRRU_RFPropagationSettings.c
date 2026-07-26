[BaseContainerProps(configRoot: true)]
class IRRU_RFPropagationSettings
{
    protected static ref IRRU_RFPropagationSettings s_Instance;
    protected static bool s_bInitialized = false;
    protected static const string JSON_CONFIG_PATH = "$profile:IRRU_RFPropagation.json";

    [Attribute(defvalue: "0", desc: "Enable RF propagation simulation (terrain, obstacles, distance affects signal quality)", category: "RF Propagation", uiwidget: UIWidgets.CheckBox)]
    bool m_bRFPropagationEnabled;

    [Attribute(defvalue: "0", desc: "Enable debug output to RPT log", category: "RF Propagation", uiwidget: UIWidgets.CheckBox)]
    bool m_bDebugEnabled;

    //------------------------------------------------------------------------------------------------
    static IRRU_RFPropagationSettings GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new IRRU_RFPropagationSettings();
            s_Instance.m_bRFPropagationEnabled = false;
            s_Instance.m_bDebugEnabled = false;

            if (!LoadFromJSON())
            {
                LoadFromConf();
            }

            if (!s_bInitialized)
            {
                s_bInitialized = true;
                Print(string.Format("[IRRU RFPropagation] RF Propagation: %1 | Debug: %2",
                    s_Instance.m_bRFPropagationEnabled, s_Instance.m_bDebugEnabled));
            }
        }

        return s_Instance;
    }

    //------------------------------------------------------------------------------------------------
    protected static bool LoadFromJSON()
    {
        SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
        if (!loadContext.LoadFromFile(JSON_CONFIG_PATH))
        {
            CreateDefaultJSON();
            return false;
        }

        bool rfEnabled, debugEnabled;
        if (loadContext.ReadValue("RFPropagationEnabled", rfEnabled))
            s_Instance.m_bRFPropagationEnabled = rfEnabled;

        if (loadContext.ReadValue("DebugEnabled", debugEnabled))
            s_Instance.m_bDebugEnabled = debugEnabled;

        Print("[IRRU RFPropagation] Loaded settings from JSON");
        return true;
    }

    //------------------------------------------------------------------------------------------------
    protected static void CreateDefaultJSON()
    {
        FileHandle file = FileIO.OpenFile(JSON_CONFIG_PATH, FileMode.APPEND);
        if (file)
            file.Close();

        file = FileIO.OpenFile(JSON_CONFIG_PATH, FileMode.WRITE);
        if (!file)
        {
            Print("[IRRU RFPropagation] ERROR: Failed to create JSON config", LogLevel.ERROR);
            return;
        }

        file.WriteLine("{");
        file.WriteLine("    \"RFPropagationEnabled\": false,");
        file.WriteLine("    \"DebugEnabled\": false");
        file.WriteLine("}");
        file.Close();

        Print("[IRRU RFPropagation] Created default JSON config");
    }

    //------------------------------------------------------------------------------------------------
    protected static void LoadFromConf()
    {
        Resource holder = BaseContainerTools.LoadContainer("{3DE007FE1017EE5D}Configs/506th_VONConfig.conf");

        if (holder && holder.GetResource())
        {
            BaseContainer container = holder.GetResource().ToBaseContainer();
            if (container)
            {
                IRRU_RFPropagationSettings confSettings = IRRU_RFPropagationSettings.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
                if (confSettings)
                {
                    s_Instance.m_bRFPropagationEnabled = confSettings.m_bRFPropagationEnabled;
                    s_Instance.m_bDebugEnabled = confSettings.m_bDebugEnabled;
                    Print("[IRRU RFPropagation] Loaded settings from .conf file");
                    return;
                }
            }
        }

        Print("[IRRU RFPropagation] Using default settings (disabled)", LogLevel.WARNING);
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
            Print(string.Format("[IRRU RFPropagation] RF Propagation set to %1", enabled));
        }
    }

    //------------------------------------------------------------------------------------------------
    static void SetDebugEnabled(bool enabled)
    {
        IRRU_RFPropagationSettings settings = GetInstance();
        if (settings)
        {
            settings.m_bDebugEnabled = enabled;
            Print(string.Format("[IRRU RFPropagation] Debug mode set to %1", enabled));
        }
    }
}
