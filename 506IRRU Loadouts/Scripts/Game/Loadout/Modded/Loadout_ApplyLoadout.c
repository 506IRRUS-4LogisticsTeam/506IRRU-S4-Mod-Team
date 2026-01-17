sealed class Loadout_ApplyLoadout {
	static bool ApplyLoadoutByName(IEntity characterEntity, string loadoutName)
	{
		if (!Replication.IsServer())
			return false;

		if (!characterEntity)
			return false;

		string loadoutPath = Loadout_FilePaths.BuildLoadoutPath(loadoutName);
		if (!FileIO.FileExists(loadoutPath))
			return false;

		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if (!loadContext.LoadFromFile(loadoutPath))
			return false;

		ClearCharacterEquipment(characterEntity);
		return SCR_PlayerArsenalLoadout.ApplyLoadoutString(characterEntity, loadContext);
	}

	static void ClearCharacterEquipment(IEntity characterEntity)
	{
		array<BaseInventoryStorageComponent> storages = {};
		int storagesCount = Bacon_GunBuilderUI_Helpers.GetAllEntityStorages(characterEntity, storages);
		if (storagesCount < 1)
			return;

		array<IEntity> items = {};
		foreach (BaseInventoryStorageComponent storage : storages) {
			items.Clear();
			storage.GetAll(items);

			foreach (IEntity item : items) {
				SCR_EntityHelper.DeleteEntityAndChildren(item);
			}
		}
	}
}
