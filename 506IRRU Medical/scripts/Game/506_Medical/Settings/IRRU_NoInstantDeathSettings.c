[BaseContainerProps(configRoot: true)]
class IRRU_NoInstantDeathSettings
{
	static const string MOD_VERSION = "2.2.3";
	protected const float MIN_BLEEDOUT_TIME = 60.0;
	protected const float MAX_BLEEDOUT_TIME = 3600.0;
	protected const float DEFAULT_BLEEDOUT_TIME = 360.0;

	[Attribute(defvalue: "360", desc: "Time (in seconds) before the unconscious player dies", category: "No Instant Death")]
	float m_fBleedoutTime;

	[Attribute(defvalue: "1", desc: "Enable verbose debug output to the RPT log", category: "No Instant Death", uiwidget: UIWidgets.CheckBox)]
	bool m_bDebugEnabled;

	[Attribute(defvalue: "1", desc: "Use descriptive text instead of exact timer", category: "No Instant Death", uiwidget: UIWidgets.CheckBox)]
	bool m_bUseDescriptiveTimer;

	protected static ref IRRU_NoInstantDeathSettings s_Instance;
	protected static bool s_Initialized = false;

	void IRRU_NoInstantDeathSettings()
	{
		if (m_fBleedoutTime <= 0)
			m_fBleedoutTime = DEFAULT_BLEEDOUT_TIME;
	}

	static IRRU_NoInstantDeathSettings GetInstance()
	{
		if (!s_Instance)
		{
			Resource holder = BaseContainerTools.LoadContainer("{7E9D8A65E020E49C}Configs/IRRU_NoInstantDeathSettings.conf");
			if (holder && holder.GetResource())
			{
				BaseContainer container = holder.GetResource().ToBaseContainer();
				if (container)
					s_Instance = IRRU_NoInstantDeathSettings.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
			}

			if (!s_Instance)
			{
				Print("[NoInstantDeath] Config not found, using defaults", LogLevel.WARNING);
				s_Instance = new IRRU_NoInstantDeathSettings();
				s_Instance.m_fBleedoutTime = DEFAULT_BLEEDOUT_TIME;
				s_Instance.m_bDebugEnabled = true;
				s_Instance.m_bUseDescriptiveTimer = true;
			}

			if (!s_Initialized)
			{
				s_Initialized = true;
				Print(string.Format("[NoInstantDeath] Medical Mod v%1 initialized - Bleedout: %2s, Debug: %3",
					MOD_VERSION, s_Instance.m_fBleedoutTime, s_Instance.m_bDebugEnabled));
			}
		}
		return s_Instance;
	}

	static bool IsDebugEnabled()
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
			return settings.m_bDebugEnabled;
		return true;
	}

	static float GetBleedoutTime()
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
			return Math.Clamp(settings.m_fBleedoutTime, MIN_BLEEDOUT_TIME, MAX_BLEEDOUT_TIME);
		return DEFAULT_BLEEDOUT_TIME;
	}

	static void SetBleedoutTime(float seconds)
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
			settings.m_fBleedoutTime = seconds;
	}

	static void SetDebugEnabled(bool enabled)
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
			settings.m_bDebugEnabled = enabled;
	}

	static bool IsDescriptiveTimerEnabled()
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
			return settings.m_bUseDescriptiveTimer;
		return true;
	}

	static void SetDescriptiveTimerEnabled(bool enabled)
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
			settings.m_bUseDescriptiveTimer = enabled;
	}
}
