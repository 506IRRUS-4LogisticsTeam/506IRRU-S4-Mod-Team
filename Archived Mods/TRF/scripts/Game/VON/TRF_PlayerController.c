modded class SCR_PlayerController : SCR_PlayerController
{
    // Called when the controlled entity changes
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);
        GetGame().GetCallqueue().CallLater(InitializeRouting, 100, false, to);
    }

    // Initializes routing to default (CENTER) when the player entity changes
    void InitializeRouting(IEntity playerEntity)
    {
        if (!playerEntity) return;

        BaseGameEntity baseEntity = BaseGameEntity.Cast(playerEntity);
        if (!baseEntity) return;

        // Try to find the VONRoutingComponent
        VONRoutingComponent routing = VONRoutingComponent.Cast(baseEntity.FindComponent(VONRoutingComponent));
        if (routing)
            routing.ApplyRouting(EVONAudioRouting.CENTER);
        else
            PrintFormat("TRF_PlayerController: InitializeRouting failed - VONRoutingComponent not found on entity '%1' (%2).", baseEntity.ToString(), baseEntity.GetPrefabData().GetPrefabName());
    }

    // Add action listeners for VON routing
    override void UpdateLocalPlayerController()
    {
        super.UpdateLocalPlayerController();

        if (!m_bIsLocalPlayerController)
            return;

        InputManager inputManager = GetGame().GetInputManager();
        if (!inputManager)
            return;

        // Add listeners for VON routing inputs
        inputManager.AddActionListener("VONRoutingLeft", EActionTrigger.DOWN, OnVONLeft);
        inputManager.AddActionListener("VONRoutingCenter", EActionTrigger.DOWN, OnVONCenter);
        inputManager.AddActionListener("VONRoutingRight", EActionTrigger.DOWN, OnVONRight);
    }

    // Handles VON routing to left
    protected void OnVONLeft(float value, EActionTrigger reason)
    {
        ApplyVONRouting(EVONAudioRouting.LEFT);
    }

    // Handles VON routing to center
    protected void OnVONCenter(float value, EActionTrigger reason)
    {
        ApplyVONRouting(EVONAudioRouting.CENTER);
    }

    // Handles VON routing to right
    protected void OnVONRight(float value, EActionTrigger reason)
    {
        ApplyVONRouting(EVONAudioRouting.RIGHT);
    }

    // Applies VON routing to the controlled entity
    protected void ApplyVONRouting(EVONAudioRouting direction)
    {
        IEntity entity = GetControlledEntity();
        if (!entity) return;

        VONRoutingComponent routing = VONRoutingComponent.Cast(entity.FindComponent(VONRoutingComponent));
        if (routing)
            routing.ApplyRouting(direction);
        else
            PrintFormat("TRF_PlayerController: ApplyVONRouting failed - VONRoutingComponent not found on entity '%1' (%2).", entity.ToString(), entity.GetPrefabData().GetPrefabName());
    }
}
