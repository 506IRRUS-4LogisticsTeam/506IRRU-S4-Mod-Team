class IRRU_ContactViewSettings
{
	protected static float s_fGreenThreshold = 30.0;

	//------------------------------------------------------------------------------------------------
	static float GetGreenThreshold()
	{
		return s_fGreenThreshold;
	}

	//------------------------------------------------------------------------------------------------
	static void SetGreenThreshold(float seconds)
	{
		s_fGreenThreshold = seconds;
	}
}
