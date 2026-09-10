//! Per-user client-side toggles for Enhanced Radio extras, persisted to the
//! local profile so they apply from the moment of connection (the radio check
//! fires before any in-session toggle could take effect).
//! Also the persistence owner for crypto fills (frequency kHz -> fill string):
//! fills must survive crash-relogs or a rejoining RTO silently transmits
//! plaintext believing they are secure. SCR_IRRURadioEarSettings proxies its
//! fill accessors here so there is exactly one live copy.
class IRRU_RadioUserSettings
{
    protected static const string SETTINGS_FILE = "$profile:IRRU_EnhancedRadio_settings.json";

    private static ref IRRU_RadioUserSettings s_Instance;

    protected bool m_bRadioCheckEnabled = true;
    protected bool m_bRxBeepsEnabled = true;
    protected ref map<int, string> m_mFillByFrequency = new map<int, string>();

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
    //! Fill for a frequency; empty string = plaintext (no entry)
    string GetFill(int frequencyKHz)
    {
        string fill;
        if (m_mFillByFrequency.Find(frequencyKHz, fill))
            return fill;

        return string.Empty;
    }

    //------------------------------------------------------------------------------------------------
    void SetFill(int frequencyKHz, string fill)
    {
        if (fill.IsEmpty())
        {
            ClearFill(frequencyKHz);
            return;
        }

        m_mFillByFrequency.Set(frequencyKHz, fill);
        Save();
    }

    //------------------------------------------------------------------------------------------------
    void ClearFill(int frequencyKHz)
    {
        if (!m_mFillByFrequency.Contains(frequencyKHz))
            return;

        m_mFillByFrequency.Remove(frequencyKHz);
        Save();
    }

    //------------------------------------------------------------------------------------------------
    void GetFillFrequencies(notnull array<int> outFrequencies)
    {
        outFrequencies.Clear();
        foreach (int frequency, string fill : m_mFillByFrequency)
            outFrequencies.Insert(frequency);
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
            else if (line.Contains("\"fill_"))
                ParseFillLine(line);
        }

        file.Close();
    }

    //------------------------------------------------------------------------------------------------
    //! Parses a persisted fill line of the form:  "fill_38000": "738291",
    protected void ParseFillLine(string line)
    {
        int keyStart = line.IndexOf("fill_");
        if (keyStart < 0)
            return;
        keyStart += 5;

        int keyEnd = line.IndexOfFrom(keyStart, "\"");
        if (keyEnd <= keyStart)
            return;

        string freqText = line.Substring(keyStart, keyEnd - keyStart);
        int frequency = freqText.ToInt();
        if (frequency <= 0)
            return;

        // Value is the next quoted token after the separating colon
        int valueStart = line.IndexOfFrom(keyEnd + 1, "\"");
        if (valueStart < 0)
            return;
        valueStart += 1;

        int valueEnd = line.IndexOfFrom(valueStart, "\"");
        if (valueEnd <= valueStart)
            return;

        string fill = line.Substring(valueStart, valueEnd - valueStart);
        if (!SCR_IRRURadioEarSettings.IRRU_IsDigits(fill))
            return;

        m_mFillByFrequency.Set(frequency, fill);
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

        // Every entry line takes a trailing comma; the "_end" terminator keeps
        // the file valid JSON without tracking which line is last.
        file.WriteLine("{");
        file.WriteLine(string.Format("  \"radioCheck\": %1,", BoolToJson(m_bRadioCheckEnabled)));
        file.WriteLine(string.Format("  \"rxBeeps\": %1,", BoolToJson(m_bRxBeepsEnabled)));
        foreach (int frequency, string fill : m_mFillByFrequency)
            file.WriteLine(string.Format("  \"fill_%1\": \"%2\",", frequency, fill));
        file.WriteLine("  \"_end\": true");
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
