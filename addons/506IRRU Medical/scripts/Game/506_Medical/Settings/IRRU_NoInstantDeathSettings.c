[BaseContainerProps(configRoot: true)]
class IRRU_NoInstantDeathSettings
{
	static const string MOD_VERSION = "2.3.9";
	protected static const ResourceName CONFIG_PATH = "{7E9D8A65E020E49C}Configs/IRRU_NoInstantDeathSettings.conf";
	protected static const float MIN_BLEEDOUT_TIME = 60.0;
	protected static const float MAX_BLEEDOUT_TIME = 3600.0;
	protected static const float MAX_BLEEDING_RATE_SCALE = 5.0;

	[Attribute(defvalue: "360", desc: "Time (in seconds) before the unconscious player dies", category: "No Instant Death")]
	float m_fBleedoutTime;

	[Attribute(defvalue: "1", desc: "Enable verbose debug output to the RPT log", category: "No Instant Death", uiwidget: UIWidgets.CheckBox)]
	bool m_bDebugEnabled;

	[Attribute(defvalue: "1", desc: "Use descriptive text instead of exact timer", category: "No Instant Death", uiwidget: UIWidgets.CheckBox)]
	bool m_bUseDescriptiveTimer;

	[Attribute(defvalue: "1", desc: "Character bleeding rate multiplier", category: "Bleeding", uiwidget: UIWidgets.Slider, params: "0 5 0.001", precision: 3)]
	float m_fBleedingRateScale;

	[Attribute(defvalue: "-1", desc: "Maximum combined bleeding rate in ml/s across all wounds (negative means uncapped)", category: "Bleeding")]
	float m_fMaxTotalBleedingRate;

	protected static ref IRRU_NoInstantDeathSettings s_Instance;

	//------------------------------------------------------------------------------------------------
	static IRRU_NoInstantDeathSettings GetInstance()
	{
		if (s_Instance)
			return s_Instance;

		s_Instance = SCR_ConfigHelperT<IRRU_NoInstantDeathSettings>.GetConfigObject(CONFIG_PATH);
		if (!s_Instance)
		{
			Print("[NoInstantDeath] Config not found, using defaults", LogLevel.WARNING);
			s_Instance = new IRRU_NoInstantDeathSettings();
			s_Instance.m_fBleedoutTime = 360;
			s_Instance.m_bDebugEnabled = true;
			s_Instance.m_bUseDescriptiveTimer = true;
			s_Instance.m_fBleedingRateScale = 1;
			s_Instance.m_fMaxTotalBleedingRate = -1;
		}

		Print(string.Format("[NoInstantDeath] Medical Mod v%1 - Bleedout: %2s, Debug: %3", MOD_VERSION, s_Instance.m_fBleedoutTime, s_Instance.m_bDebugEnabled));
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDebugEnabled()
	{
		return GetInstance().m_bDebugEnabled;
	}

	static float GetBleedoutTime()
	{
		return Math.Clamp(GetInstance().m_fBleedoutTime, MIN_BLEEDOUT_TIME, MAX_BLEEDOUT_TIME);
	}

	static float GetBleedingRateScale()
	{
		return Math.Clamp(GetInstance().m_fBleedingRateScale, 0, MAX_BLEEDING_RATE_SCALE);
	}

	static float GetMaxTotalBleedingRate()
	{
		return GetInstance().m_fMaxTotalBleedingRate;
	}

	static bool IsDescriptiveTimerEnabled()
	{
		return GetInstance().m_bUseDescriptiveTimer;
	}
}
