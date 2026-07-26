sealed class Loadout_FilePaths {
	static const string LOADOUT_ROOT = "$profile:/Loadouts";
	static const string LOADOUT_INDEX = "$profile:/Loadouts/LoadoutIndex.json";

	static void EnsureLoadoutRoot()
	{
		FileIO.MakeDirectory(LOADOUT_ROOT);
	}

	static string BuildLoadoutPath(string loadoutName)
	{
		return string.Format("%1/%2.json", LOADOUT_ROOT, loadoutName);
	}

	static void AddLoadoutName(string loadoutName)
	{
		if (loadoutName == "CurrentLoadout")
			return;

		array<string> loadoutNames = {};
		GetSavedLoadoutNames(loadoutNames);

		if (loadoutNames.Contains(loadoutName))
			return;

		loadoutNames.Insert(loadoutName);
		SaveLoadoutIndex(loadoutNames);
	}

	static void GetSavedLoadoutNames(out array<string> loadoutNames)
	{
		loadoutNames = {};
		EnsureLoadoutRoot();

		if (!FileIO.FileExists(LOADOUT_INDEX))
			return;

		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if (!loadContext.LoadFromFile(LOADOUT_INDEX))
			return;

		loadContext.ReadValue("", loadoutNames);
	}

	protected static void SaveLoadoutIndex(array<string> loadoutNames)
	{
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		if (!saveContext.WriteValue("", loadoutNames))
			return;

		saveContext.SaveToFile(LOADOUT_INDEX);
	}
}
