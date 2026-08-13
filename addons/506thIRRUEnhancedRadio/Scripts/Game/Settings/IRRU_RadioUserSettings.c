//! Per-user client-side toggles for Enhanced Radio extras, persisted to the
//! local profile so they apply from the moment of connection (the radio check
//! fires before any in-session toggle could take effect).
class IRRU_RadioUserSettings
{
    protected static const string SETTINGS_FILE = "$profile:IRRU_EnhancedRadio_settings.json";

    private static ref IRRU_RadioUserSettings s_Instance;

    protected bool m_bRadioCheckEnabled = true;
    protected bool m_bRxBeepsEnabled = true;

    //------------------------------------------------------------------------------------------------
    static IRRU_RadioUserSettings GetInstance()
    {
        if (!s_Instance)
        {
            s_Instance = new IRRU_RadioUserSettings();
            s_Instance.Load();
        }

        return s_Instance;
    }

    //------------------------------------------------------------------------------------------------
    bool IsRadioCheckEnabled()
    {
        return m_bRadioCheckEnabled;
    }

    //------------------------------------------------------------------------------------------------
    bool AreRxBeepsEnabled()
    {
        return m_bRxBeepsEnabled;
    }

    //------------------------------------------------------------------------------------------------
    void SetRadioCheckEnabled(bool enabled)
    {
        m_bRadioCheckEnabled = enabled;
        Save();
    }

    //------------------------------------------------------------------------------------------------
    void SetRxBeepsEnabled(bool enabled)
    {
        m_bRxBeepsEnabled = enabled;
        Save();
    }

    //------------------------------------------------------------------------------------------------
    protected void Load()
    {
        FileHandle file = FileIO.OpenFile(SETTINGS_FILE, FileMode.READ);
        if (!file)
            return;

        string line;
        while (file.ReadLine(line) != -1)
        {
            line.Trim();
            if (line.Contains("\"radioCheck\""))
                m_bRadioCheckEnabled = !line.Contains("false");
            else if (line.Contains("\"rxBeeps\""))
                m_bRxBeepsEnabled = !line.Contains("false");
        }

        file.Close();
    }

    //------------------------------------------------------------------------------------------------
    protected void Save()
    {
        FileHandle file = FileIO.OpenFile(SETTINGS_FILE, FileMode.WRITE);
        if (!file)
        {
            file = FileIO.OpenFile(SETTINGS_FILE, FileMode.APPEND);
            if (!file)
                return;

            file.Close();
            file = FileIO.OpenFile(SETTINGS_FILE, FileMode.WRITE);
            if (!file)
                return;
        }

        file.WriteLine("{");
        file.WriteLine(string.Format("  \"radioCheck\": %1,", BoolToJson(m_bRadioCheckEnabled)));
        file.WriteLine(string.Format("  \"rxBeeps\": %1", BoolToJson(m_bRxBeepsEnabled)));
        file.WriteLine("}");
        file.Close();
    }

    //------------------------------------------------------------------------------------------------
    protected string BoolToJson(bool value)
    {
        if (value)
            return "true";

        return "false";
    }
}
