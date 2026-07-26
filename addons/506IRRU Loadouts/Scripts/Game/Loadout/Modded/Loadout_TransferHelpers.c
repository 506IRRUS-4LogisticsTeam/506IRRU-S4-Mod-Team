sealed class Loadout_TransferHelpers {
	static bool TransferLoadout(IEntity sourceEntity, IEntity targetEntity)
	{
		if (!Replication.IsServer())
			return false;

		if (!sourceEntity || !targetEntity)
			return false;

		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(sourceEntity, saveContext))
			return false;

		string loadoutData = saveContext.ExportToString();
		if (loadoutData.IsEmpty())
			return false;

		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if (!loadContext.ImportFromString(loadoutData))
			return false;

		Loadout_ApplyLoadout.ClearCharacterEquipment(targetEntity);
		return SCR_PlayerArsenalLoadout.ApplyLoadoutString(targetEntity, loadContext);
	}
}
