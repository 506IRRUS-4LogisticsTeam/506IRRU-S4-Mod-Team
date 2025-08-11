// ============================================================================
//  NoInstantDeath_Settings.c   
//  506th IRRU Medical Mod v2.0.5
//  Standalone settings configuration
// ============================================================================

[BaseContainerProps(configRoot: true)]
class NoInstantDeath_Settings
{
	// Version constant
	static const string MOD_VERSION = "2.0.5";
	
	// ─── Configurable Settings ─────────────────────────────────────────
	[Attribute(defvalue: "360", desc: "Time (in seconds) before the unconscious player dies", category: "No Instant Death")]
	float m_fBleedoutTime;
	
	[Attribute(defvalue: "1", desc: "Enable verbose debug output to the RPT log", category: "No Instant Death", uiwidget: UIWidgets.CheckBox)]
	bool m_bDebugEnabled;
	
	[Attribute(defvalue: "1.0", desc: "Bleeding rate multiplier (0.5 = half speed, 2.0 = double speed)", category: "No Instant Death", params: "0.01 5.0 0.01")]
	float m_fBleedingScale;
	
	// ─── Static singleton instance ─────────────────────────────────────
	protected static ref NoInstantDeath_Settings s_Instance;
	protected static bool s_Initialized = false;
	
	//------------------------------------------------------------------------------------------------
	//! Constructor
	void NoInstantDeath_Settings()
	{
		// Set defaults if not specified
		if (m_fBleedoutTime <= 0)
			m_fBleedoutTime = 360.0;
		
		if (m_fBleedingScale <= 0)
			m_fBleedingScale = 1.0;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the singleton instance
	static NoInstantDeath_Settings GetInstance()
	{
		if (!s_Instance)
		{
			// Try to load from config file
			Resource holder = BaseContainerTools.LoadContainer("{0FA779A2B56A1711}Configs/NoInstantDeath_Settings.conf");
			if (holder)
			{
				BaseContainer container = holder.GetResource().ToBaseContainer();
				s_Instance = NoInstantDeath_Settings.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
			}
			
			// Create default if config not found
			if (!s_Instance)
			{
				Print(string.Format("[NoInstantDeath] Config settings not found! :O"));
				s_Instance = new NoInstantDeath_Settings();
				s_Instance.m_fBleedoutTime = 360.0;  // Default 6 minutes
				s_Instance.m_bDebugEnabled = true;   // Default debug on
				s_Instance.m_fBleedingScale = 1.0;  // Default normal bleeding rate
			}
			
			// Log initialization once
			if (!s_Initialized)
			{
				s_Initialized = true;
				Print(string.Format("[NoInstantDeath] Medical Mod v%1 initialized", MOD_VERSION));
				Print(string.Format("[NoInstantDeath] Bleedout timer: %1 seconds", s_Instance.m_fBleedoutTime));
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
		NoInstantDeath_Settings settings = GetInstance();
		if (settings)
			return settings.m_bDebugEnabled;
		return true; // Default to debug on
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the configured bleedout time in seconds
	static float GetBleedoutTime()
	{
		NoInstantDeath_Settings settings = GetInstance();
		if (settings)
		{
			float time = settings.m_fBleedoutTime;
			
			// Validate and clamp
			if (time < 60.0)
			{
				static bool s_warnedLow = false;
				if (!s_warnedLow && IsDebugEnabled())
				{
					Print(string.Format("[NoInstantDeath] Bleedout time %1s too low, clamping to 60s", time));
					s_warnedLow = true;
				}
				return 60.0;  // Minimum 1 minute
			}
			
			if (time > 3600.0)
			{
				static bool s_warnedHigh = false;
				if (!s_warnedHigh && IsDebugEnabled())
				{
					Print(string.Format("[NoInstantDeath] Bleedout time %1s too high, clamping to 3600s", time));
					s_warnedHigh = true;
				}
				return 3600.0;  // Maximum 1 hour
			}
			
			return time;
		}
		
		// Fallback default
		return 360.0;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Runtime configuration (for testing)
	static void SetBleedoutTime(float seconds)
	{
		NoInstantDeath_Settings settings = GetInstance();
		if (settings)
		{
			settings.m_fBleedoutTime = seconds;
			if (IsDebugEnabled())
				Print(string.Format("[NoInstantDeath] Bleedout timer changed to %1 seconds", seconds));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Runtime debug toggle
	static void SetDebugEnabled(bool enabled)
	{
		NoInstantDeath_Settings settings = GetInstance();
		if (settings)
		{
			settings.m_bDebugEnabled = enabled;
			Print(string.Format("[NoInstantDeath] Debug mode set to %1", enabled));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the configured bleeding scale
	static float GetBleedingScale()
	{
		NoInstantDeath_Settings settings = GetInstance();
		if (settings)
		{
			float scale = settings.m_fBleedingScale;
			
			// Validate and clamp
			if (scale < 0.01)
			{
				static bool s_warnedLow = false;
				if (!s_warnedLow && IsDebugEnabled())
				{
					Print(string.Format("[NoInstantDeath] Bleeding scale %1 too low, clamping to 0.01", scale));
					s_warnedLow = true;
				}
				return 0.01;  // Minimum 1% bleeding
			}
			
			if (scale > 5.0)
			{
				static bool s_warnedHigh = false;
				if (!s_warnedHigh && IsDebugEnabled())
				{
					Print(string.Format("[NoInstantDeath] Bleeding scale %1 too high, clamping to 5.0", scale));
					s_warnedHigh = true;
				}
				return 5.0;  // Maximum 5x bleeding
			}
			
			return scale;
		}
		
		// Fallback default
		return 1.0;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Runtime bleeding scale configuration (for testing)
	static void SetBleedingScale(float scale)
	{
		NoInstantDeath_Settings settings = GetInstance();
		if (settings)
		{
			settings.m_fBleedingScale = scale;
			if (IsDebugEnabled())
				Print(string.Format("[NoInstantDeath] Bleeding scale changed to %1x", scale));
		}
	}
}