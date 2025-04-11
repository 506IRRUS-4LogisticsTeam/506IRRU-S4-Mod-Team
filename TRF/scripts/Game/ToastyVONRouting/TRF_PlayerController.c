modded class SCR_PlayerController : SCR_PlayerController
{
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		GetGame().GetCallqueue().CallLater(InitializeRouting, 100, false, to);
	}

	void InitializeRouting(IEntity playerEntity)
	{
		if (!playerEntity) return;

		BaseGameEntity baseEntity = BaseGameEntity.Cast(playerEntity);
		if (!baseEntity) return;

		// Try to find the VONRoutingComponent
		VONRoutingComponent routing = VONRoutingComponent.Cast(baseEntity.FindComponent(VONRoutingComponent));
		if (routing)
			routing.ApplyRouting(EVONAudioRouting.CENTER);
	}

	override void UpdateLocalPlayerController()
	{
		super.UpdateLocalPlayerController();

		if (!m_bIsLocalPlayerController)
			return;

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.AddActionListener("VONRoutingLeft", EActionTrigger.DOWN, OnVONLeft);
		inputManager.AddActionListener("VONRoutingCenter", EActionTrigger.DOWN, OnVONCenter);
		inputManager.AddActionListener("VONRoutingRight", EActionTrigger.DOWN, OnVONRight);
	}

	protected void OnVONLeft(float value, EActionTrigger reason)
	{
		ApplyVONRouting(EVONAudioRouting.LEFT);
	}

	protected void OnVONCenter(float value, EActionTrigger reason)
	{
		ApplyVONRouting(EVONAudioRouting.CENTER);
	}

	protected void OnVONRight(float value, EActionTrigger reason)
	{
		ApplyVONRouting(EVONAudioRouting.RIGHT);
	}

	protected void ApplyVONRouting(EVONAudioRouting direction)
	{
		IEntity entity = GetControlledEntity();
		if (!entity) return;

		VONRoutingComponent routing = VONRoutingComponent.Cast(entity.FindComponent(VONRoutingComponent));
		if (routing)
			routing.ApplyRouting(direction);
	}

	
}
