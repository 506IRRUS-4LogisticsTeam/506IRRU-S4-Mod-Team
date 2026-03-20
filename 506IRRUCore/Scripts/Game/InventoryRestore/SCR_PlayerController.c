modded class SCR_PlayerController : SCR_PlayerController
{
    override void OnControlledEntityChanged(IEntity from, IEntity to)
    {
        super.OnControlledEntityChanged(from, to);

        if (Replication.IsServer() && to)
        {
            // Run multiple delayed checks to catch reconnect/loadout sync timing.
            QueueInventoryChecks(to);
        }
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

        IRRU_InventoryRestore.CheckAndAddItems(player);
    }
}