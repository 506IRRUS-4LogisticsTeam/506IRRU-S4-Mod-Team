modded class SightsComponent : BaseSightsComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Can NVG be used when aiming with this scope", category : "RHS NVG")]
	protected bool m_bIsNVGCompatible;

	[Attribute("0", UIWidgets.CheckBox, "Ignored if IsNVGCompatible is true but in case that it is false then this determines if NVG should use IsNVGCompatible or calculate zoom level based on sight FOV", category : "RHS NVG")]
	protected bool m_bIsConfiguratedForRHSNVG;

	[Attribute("0", UIWidgets.CheckBox, "When enabled, Aiming Down Sight DOF is disabled", category : "RHS NVG")]
	protected bool m_bIsDOFDisabled;

	
	//------------------------------------------------------------------------------------------------
	bool RHS_IsNVGCompatible()
	{
		return m_bIsNVGCompatible;
	}

	//------------------------------------------------------------------------------------------------
	bool RHS_IsSightConfiguredForRHSNVG()
	{
		return m_bIsNVGCompatible || m_bIsConfiguratedForRHSNVG;
	}
	bool RHS_IsDOFDisabled()
	{
		return m_bIsDOFDisabled;
	}
}
