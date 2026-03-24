modded class SCR_PlayerController : SCR_PlayerController
{
    protected bool IsPossessedAIEntity(IEntity entity)
    {
        if (!entity)
            return false;

        SCR_ECharacterControlType controlType = SCR_CharacterHelper.GetCharacterControlType(entity);
        return controlType == SCR_ECharacterControlType.POSSESSED_AI;
    }

    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        if (!Replication.IsServer())
            return;

        if (!to || IsPossessedAIEntity(to))
            return;

        // Run multiple delayed checks to catch reconnect/loadout sync timing.
        QueueInventoryChecks(to);
    }

    void QueueInventoryChecks(IEntity player)
    {
        if (!player)
            return;

        // CallLater is in milliseconds, not seconds.
        GetGame().GetCallqueue().CallLater(DelayedInventoryCheck, 1000, false, player);
        GetGame().GetCallqueue().CallLater(DelayedInventoryCheck, 5000, false, player);
        GetGame().GetCallqueue().CallLater(DelayedInventoryCheck, 15000, false, player);
        GetGame().GetCallqueue().CallLater(DelayedInventoryCheck, 30000, false, player);
        GetGame().GetCallqueue().CallLater(DelayedInventoryCheck, 60000, false, player);
    }

    void DelayedInventoryCheck(IEntity player)
    {
        if (!Replication.IsServer() || !player)
            return;

        // Skip delayed checks for remote-controlled AI entities.
        if (IsPossessedAIEntity(player))
            return;

        IRRU_InventoryRestore.CheckAndAddItems(player);
    }
}