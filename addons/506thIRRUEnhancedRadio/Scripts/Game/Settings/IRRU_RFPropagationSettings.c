//! Server-side RF propagation settings, read from $profile:IRRU_RFPropagation.json
//! (created with defaults on first run). Clients never read this directly -
//! IRRU_RFPropagationNetworkComponent replicates the server's values to them.
[BaseContainerProps(configRoot: true)]
class IRRU_RFPropagationSettings
{
    protected static ref IRRU_RFPropagationSettings s_Instance;
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
            s_Instance.LoadFromJSON();
            Print(string.Format("[IRRU RFPropagation] RF Propagation: %1 | Debug: %2",
                s_Instance.m_bRFPropagationEnabled, s_Instance.m_bDebugEnabled));
        }

        return s_Instance;
    }

    //------------------------------------------------------------------------------------------------
    bool IsRFPropagationEnabled()
    {
        return m_bRFPropagationEnabled;
    }

    //------------------------------------------------------------------------------------------------
    bool IsDebugEnabled()
    {
        return m_bDebugEnabled;
    }

    //------------------------------------------------------------------------------------------------
    protected void LoadFromJSON()
    {
        SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
        if (!loadContext.LoadFromFile(JSON_CONFIG_PATH))
        {
            CreateDefaultJSON();
            return;
        }

        bool rfEnabled, debugEnabled;
        if (loadContext.ReadValue("RFPropagationEnabled", rfEnabled))
            m_bRFPropagationEnabled = rfEnabled;

        if (loadContext.ReadValue("DebugEnabled", debugEnabled))
            m_bDebugEnabled = debugEnabled;
    }

    //------------------------------------------------------------------------------------------------
    protected void CreateDefaultJSON()
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
    }
}
