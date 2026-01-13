sealed class Loadout_SaveCurrentLoadout {
	static bool SaveCurrentCharacterLoadout(IEntity characterEntity, string loadoutName)
	{
		if (!Replication.IsServer())
			return false;

		if (!characterEntity)
			return false;

		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(characterEntity, saveContext))
			return false;

		Loadout_FilePaths.EnsureLoadoutRoot();
		if (!saveContext.SaveToFile(Loadout_FilePaths.BuildLoadoutPath(loadoutName)))
			return false;

		Loadout_FilePaths.AddLoadoutName(loadoutName);
		return true;
	}
}
