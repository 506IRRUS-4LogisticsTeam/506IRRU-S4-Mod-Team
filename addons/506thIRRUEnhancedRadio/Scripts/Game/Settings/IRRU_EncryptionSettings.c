//! Server-side encryption (COMSEC fills) settings, read from
//! $profile:IRRU_Encryption.json (created with defaults on first run).
//! Clients never read this directly - IRRU_RFPropagationNetworkComponent
//! replicates the enabled flag to them. See ENCRYPTION_DESIGN.md.
[BaseContainerProps(configRoot: true)]
class IRRU_EncryptionSettings
{
	protected static ref IRRU_EncryptionSettings s_Instance;
	protected static const string JSON_CONFIG_PATH = "$profile:IRRU_Encryption.json";

	[Attribute(defvalue: "0", desc: "Enable per-channel crypto fills (mismatched fills hear scrambled noise)", category: "Encryption", uiwidget: UIWidgets.CheckBox)]
	bool m_bEncryptionEnabled;

	[Attribute(defvalue: "0", desc: "Enable debug output to RPT log", category: "Encryption", uiwidget: UIWidgets.CheckBox)]
	bool m_bDebugEnabled;

	//------------------------------------------------------------------------------------------------
	static IRRU_EncryptionSettings GetInstance()
	{
		if (!s_Instance)
		{
			s_Instance = new IRRU_EncryptionSettings();
			s_Instance.LoadFromJSON();
			Print(string.Format("[IRRU Encryption] Encryption: %1 | Debug: %2",
				s_Instance.m_bEncryptionEnabled, s_Instance.m_bDebugEnabled));
		}

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	bool IsEncryptionEnabled()
	{
		return m_bEncryptionEnabled;
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

		bool encryptionEnabled, debugEnabled;
		if (loadContext.ReadValue("EncryptionEnabled", encryptionEnabled))
			m_bEncryptionEnabled = encryptionEnabled;

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
			Print("[IRRU Encryption] ERROR: Failed to create JSON config", LogLevel.ERROR);
			return;
		}

		file.WriteLine("{");
		file.WriteLine("    \"EncryptionEnabled\": false,");
		file.WriteLine("    \"DebugEnabled\": false");
		file.WriteLine("}");
		file.Close();
	}
}
