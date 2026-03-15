modded class SightsComponent : BaseSightsComponent
{
	[Attribute("1", UIWidgets.CheckBox, "Can NVG be used when aiming with this scope", category : "RHS NVG")]
	protected bool m_bIsNVGCompatible;


	//------------------------------------------------------------------------------------------------
    override bool RHS_IsNVGCompatible()
    {
        return true;
    }
	//------------------------------------------------------------------------------------------------
    override bool RHS_IsSightConfiguredForRHSNVG()
    {
        return true;
    }
}
