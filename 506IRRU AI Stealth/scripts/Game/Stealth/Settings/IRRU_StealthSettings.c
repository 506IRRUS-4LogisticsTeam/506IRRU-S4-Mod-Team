[BaseContainerProps(configRoot: true)]
class IRRU_StealthSettings
{
	static const string MOD_VERSION = "0.1.0";

	protected const float MIN_DETECTION_RANGE = 1.0;
	protected const float MAX_DETECTION_RANGE = 200.0;
	protected const float DEFAULT_DETECTION_RANGE = 15.0;

	protected const int MIN_CHECK_INTERVAL_MS = 250;
	protected const int MAX_CHECK_INTERVAL_MS = 10000;
	protected const int DEFAULT_CHECK_INTERVAL_MS = 1000;

	[Attribute(defvalue: "15.0", desc: "Max distance (m) at which AI will detect a stealthed player. Beyond this, AI ignores them.", category: "AI Stealth", uiwidget: UIWidgets.Slider, params: "1 200 0.5")]
	float m_fStealthDetectionRange;

	[Attribute(defvalue: "1000", desc: "How often (ms) to rescan a character's inventory for the stealth item.", category: "AI Stealth", uiwidget: UIWidgets.Slider, params: "250 10000 50")]
	int m_iCheckIntervalMs;

	[Attribute(defvalue: "", desc: "Prefab of the magic item that activates stealth when present in inventory.", category: "AI Stealth", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et")]
	ResourceName m_rStealthItemPrefab;

	[Attribute(defvalue: "1", desc: "Enable verbose debug output to the RPT log", category: "AI Stealth", uiwidget: UIWidgets.CheckBox)]
	bool m_bDebugEnabled;

	protected static ref IRRU_StealthSettings s_Instance;
	protected static bool s_Initialized = false;

	//------------------------------------------------------------------------------------------------
	static IRRU_StealthSettings GetInstance()
	{
		if (!s_Instance)
		{
			Resource holder = BaseContainerTools.LoadContainer("{38BD5C4E1076D380}Configs/IRRU_StealthSettings.conf");
			if (holder && holder.GetResource())
			{
				BaseContainer container = holder.GetResource().ToBaseContainer();
				if (container)
					s_Instance = IRRU_StealthSettings.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
			}

			if (!s_Instance)
			{
				Print("[AIStealth] Config not found, using defaults", LogLevel.WARNING);
				s_Instance = new IRRU_StealthSettings();
				s_Instance.m_fStealthDetectionRange = DEFAULT_DETECTION_RANGE;
				s_Instance.m_iCheckIntervalMs = DEFAULT_CHECK_INTERVAL_MS;
				s_Instance.m_bDebugEnabled = true;
			}

			if (!s_Initialized)
			{
				s_Initialized = true;
				Print(string.Format("[AIStealth] AI Stealth Mod v%1 initialized - Range: %2m, Interval: %3ms, Debug: %4",
					MOD_VERSION, s_Instance.m_fStealthDetectionRange, s_Instance.m_iCheckIntervalMs, s_Instance.m_bDebugEnabled));
			}
		}
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDebugEnabled()
	{
		IRRU_StealthSettings settings = GetInstance();
		if (settings)
			return settings.m_bDebugEnabled;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static float GetDetectionRange()
	{
		IRRU_StealthSettings settings = GetInstance();
		if (settings)
			return Math.Clamp(settings.m_fStealthDetectionRange, MIN_DETECTION_RANGE, MAX_DETECTION_RANGE);
		return DEFAULT_DETECTION_RANGE;
	}

	//------------------------------------------------------------------------------------------------
	static int GetCheckIntervalMs()
	{
		IRRU_StealthSettings settings = GetInstance();
		if (settings)
			return Math.ClampInt(settings.m_iCheckIntervalMs, MIN_CHECK_INTERVAL_MS, MAX_CHECK_INTERVAL_MS);
		return DEFAULT_CHECK_INTERVAL_MS;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName GetStealthItemPrefab()
	{
		IRRU_StealthSettings settings = GetInstance();
		if (settings)
			return settings.m_rStealthItemPrefab;
		return ResourceName.Empty;
	}
}
