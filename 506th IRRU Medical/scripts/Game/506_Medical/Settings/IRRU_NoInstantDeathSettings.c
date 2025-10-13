//! Settings configuration for medical system

[BaseContainerProps(configRoot: true)]
class IRRU_NoInstantDeathSettings
{
	static const string MOD_VERSION = "2.1.7";

	protected const float MIN_BLEEDOUT_TIME = 60.0;
	protected const float MAX_BLEEDOUT_TIME = 3600.0;
	protected const float MIN_BLEEDING_SCALE = 0.01;
	protected const float MAX_BLEEDING_SCALE = 5.0;
	protected const float DEFAULT_BLEEDOUT_TIME = 360.0;
	protected const float DEFAULT_BLEEDING_SCALE = 1.0;

	[Attribute(defvalue: "360", desc: "Time (in seconds) before the unconscious player dies", category: "No Instant Death")]
	float m_fBleedoutTime;

	[Attribute(defvalue: "1", desc: "Enable verbose debug output to the RPT log", category: "No Instant Death", uiwidget: UIWidgets.CheckBox)]
	bool m_bDebugEnabled;

	[Attribute(defvalue: "1.0", desc: "Bleeding rate multiplier (0.5 = half speed, 2.0 = double speed)", category: "No Instant Death", params: "0.01 5.0 0.01")]
	float m_fBleedingScale;

	[Attribute(defvalue: "1", desc: "Use descriptive text instead of exact timer (e.g. 'critical condition' instead of '1:23')", category: "No Instant Death", uiwidget: UIWidgets.CheckBox)]
	bool m_bUseDescriptiveTimer;

	protected static ref IRRU_NoInstantDeathSettings s_Instance;
	protected static bool s_Initialized = false;
	
	//------------------------------------------------------------------------------------------------
	void IRRU_NoInstantDeathSettings()
	{
		if (m_fBleedoutTime <= 0)
			m_fBleedoutTime = DEFAULT_BLEEDOUT_TIME;

		if (m_fBleedingScale <= 0)
			m_fBleedingScale = DEFAULT_BLEEDING_SCALE;
	}
	
	//------------------------------------------------------------------------------------------------
	static IRRU_NoInstantDeathSettings GetInstance()
	{
		if (!s_Instance)
		{
			Resource holder = BaseContainerTools.LoadContainer("{7E9D8A65E020E49C}Configs/IRRU_NoInstantDeathSettings.conf");
			if (!holder)
			{
				Print("[NoInstantDeath] Config file not found", LogLevel.WARNING);
			}
			else if (!holder.GetResource())
			{
				Print("[NoInstantDeath] Config file found but resource is null", LogLevel.WARNING);
			}
			else
			{
				BaseContainer container = holder.GetResource().ToBaseContainer();
				if (!container)
				{
					Print("[NoInstantDeath] Failed to convert resource to BaseContainer", LogLevel.WARNING);
				}
				else
				{
					s_Instance = IRRU_NoInstantDeathSettings.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
					if (!s_Instance)
						Print("[NoInstantDeath] Failed to create instance from container", LogLevel.ERROR);
				}
			}

			if (!s_Instance)
			{
				Print("[NoInstantDeath] Creating default configuration");
				s_Instance = new IRRU_NoInstantDeathSettings();
				s_Instance.m_fBleedoutTime = DEFAULT_BLEEDOUT_TIME;
				s_Instance.m_bDebugEnabled = true;
				s_Instance.m_fBleedingScale = DEFAULT_BLEEDING_SCALE;
				s_Instance.m_bUseDescriptiveTimer = true;
			}

			if (!s_Initialized)
			{
				s_Initialized = true;
				Print(string.Format("[NoInstantDeath] Medical Mod v%1 initialized", MOD_VERSION));
				Print(string.Format("[NoInstantDeath] Bleedout timer: %1s", s_Instance.m_fBleedoutTime));
				Print(string.Format("[NoInstantDeath] Debug mode: %1", s_Instance.m_bDebugEnabled));
				Print(string.Format("[NoInstantDeath] Bleeding scale: %1x", s_Instance.m_fBleedingScale));
			}
		}

		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if debug mode is enabled
	static bool IsDebugEnabled()
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
			return settings.m_bDebugEnabled;
		return true; // Default to debug on
	}
	
	//------------------------------------------------------------------------------------------------
	static float GetBleedoutTime()
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
		{
			float time = settings.m_fBleedoutTime;

			if (time < MIN_BLEEDOUT_TIME)
			{
				static bool s_warnedLow = false;
				if (!s_warnedLow && IsDebugEnabled())
				{
					Print(string.Format("[NoInstantDeath] Bleedout time %1s too low, clamping to %2s",
					                   time, MIN_BLEEDOUT_TIME), LogLevel.WARNING);
					s_warnedLow = true;
				}
				return MIN_BLEEDOUT_TIME;
			}

			if (time > MAX_BLEEDOUT_TIME)
			{
				static bool s_warnedHigh = false;
				if (!s_warnedHigh && IsDebugEnabled())
				{
					Print(string.Format("[NoInstantDeath] Bleedout time %1s too high, clamping to %2s",
					                   time, MAX_BLEEDOUT_TIME), LogLevel.WARNING);
					s_warnedHigh = true;
				}
				return MAX_BLEEDOUT_TIME;
			}

			return time;
		}

		return DEFAULT_BLEEDOUT_TIME;
	}
	
	//------------------------------------------------------------------------------------------------
	static void SetBleedoutTime(float seconds)
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
		{
			settings.m_fBleedoutTime = seconds;
			if (IsDebugEnabled())
				Print(string.Format("[NoInstantDeath] Bleedout timer changed to %1s", seconds));
		}
	}

	//------------------------------------------------------------------------------------------------
	static void SetDebugEnabled(bool enabled)
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
		{
			settings.m_bDebugEnabled = enabled;
			Print(string.Format("[NoInstantDeath] Debug mode set to %1", enabled));
		}
	}

	//------------------------------------------------------------------------------------------------
	static float GetBleedingScale()
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
		{
			float scale = settings.m_fBleedingScale;

			if (scale < MIN_BLEEDING_SCALE)
			{
				static bool s_warnedLow = false;
				if (!s_warnedLow && IsDebugEnabled())
				{
					Print(string.Format("[NoInstantDeath] Bleeding scale %1 too low, clamping to %2",
					                   scale, MIN_BLEEDING_SCALE), LogLevel.WARNING);
					s_warnedLow = true;
				}
				return MIN_BLEEDING_SCALE;
			}

			if (scale > MAX_BLEEDING_SCALE)
			{
				static bool s_warnedHigh = false;
				if (!s_warnedHigh && IsDebugEnabled())
				{
					Print(string.Format("[NoInstantDeath] Bleeding scale %1 too high, clamping to %2",
					                   scale, MAX_BLEEDING_SCALE), LogLevel.WARNING);
					s_warnedHigh = true;
				}
				return MAX_BLEEDING_SCALE;
			}

			return scale;
		}

		return DEFAULT_BLEEDING_SCALE;
	}

	//------------------------------------------------------------------------------------------------
	static void SetBleedingScale(float scale)
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
		{
			settings.m_fBleedingScale = scale;
			if (IsDebugEnabled())
				Print(string.Format("[NoInstantDeath] Bleeding scale changed to %1x", scale));
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDescriptiveTimerEnabled()
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
			return settings.m_bUseDescriptiveTimer;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void SetDescriptiveTimerEnabled(bool enabled)
	{
		IRRU_NoInstantDeathSettings settings = GetInstance();
		if (settings)
		{
			settings.m_bUseDescriptiveTimer = enabled;
			if (IsDebugEnabled())
				Print(string.Format("[NoInstantDeath] Descriptive timer mode set to %1", enabled));
		}
	}
}