//
// Patches RVX_SoflamComponent without editing the RVX file:
//  - lets the sights component register itself (RR_SetSights) so the link can't be missed
//  - falls back to searching all 2D sights components on the entity
//  - null-guards every place that uses the sights component or the local player

modded class RVX_SoflamComponent
{
	//------------------------------------------------------------------------------------------------
	// Called by RR_VisionBinocularsComponent when it is used
	void RR_SetSights(RVX_SoflamSightsComponent sights)
	{
		sightComponent = sights;
	}

	//------------------------------------------------------------------------------------------------
	protected RVX_SoflamSightsComponent RR_GetSights()
	{
		if (sightComponent)
			return sightComponent;

		IEntity owner = GetOwner();
		if (!owner)
			return null;

		sightComponent = RVX_SoflamSightsComponent.Cast(owner.FindComponent(RVX_SoflamSightsComponent));

		// Fallback: check every 2D sights component on the entity
		if (!sightComponent)
		{
			array<Managed> comps = {};
			owner.FindComponents(SCR_2DSightsComponent, comps);
			foreach (Managed comp : comps)
			{
				sightComponent = RVX_SoflamSightsComponent.Cast(comp);
				if (sightComponent)
					break;
			}
		}

		if (sightComponent)
			sightComponent.Init(owner);
		else
			Print("[RR] RVX_SoflamComponent: no RVX/RR sights component found on " + owner, LogLevel.WARNING);

		return sightComponent;
	}

	//------------------------------------------------------------------------------------------------
	protected bool RR_HasLocalPlayer()
	{
		PlayerController pc = GetGame().GetPlayerController();
		return pc && pc.GetControlledEntity();
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		RR_GetSights();
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateUI()
	{
		if (!RR_GetSights())
			return;

		super.UpdateUI();
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateLaserTarget()
	{
		if (!RR_GetSights() || !RR_HasLocalPlayer())
			return;

		super.UpdateLaserTarget();
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnToggleLaserDesignator(float value, EActionTrigger reason)
	{
		if (!RR_GetSights() || !RR_HasLocalPlayer())
			return;

		super.OnToggleLaserDesignator(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	override protected void DeleteLaserTarget()
	{
		if (!RR_HasLocalPlayer())
			return;

		super.DeleteLaserTarget();
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreateLaserTarget()
	{
		if (!RR_HasLocalPlayer())
			return;

		super.CreateLaserTarget();
	}
}