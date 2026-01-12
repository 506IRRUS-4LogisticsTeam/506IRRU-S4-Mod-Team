class IRRU_ContactViewSettings
{
	protected static float s_fWarningThreshold = 300.0;
	protected static float s_fCriticalThreshold = 600.0;
	protected static bool s_bDebugEnabled = false;

	//------------------------------------------------------------------------------------------------
	static float GetWarningThreshold()
	{
		return s_fWarningThreshold;
	}

	//------------------------------------------------------------------------------------------------
	static float GetCriticalThreshold()
	{
		return s_fCriticalThreshold;
	}

	//------------------------------------------------------------------------------------------------
	static void SetWarningThreshold(float seconds)
	{
		s_fWarningThreshold = seconds;
	}

	//------------------------------------------------------------------------------------------------
	static void SetCriticalThreshold(float seconds)
	{
		s_fCriticalThreshold = seconds;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDebugEnabled()
	{
		return s_bDebugEnabled;
	}

	//------------------------------------------------------------------------------------------------
	static void SetDebugEnabled(bool enabled)
	{
		s_bDebugEnabled = enabled;
	}
}
