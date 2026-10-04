class IRRU_LoadoutManagerSystem
{
	protected static const string PERSONAL_ROOT = "$profile:/506IRRU/ArsenalKits/Personal/";
	protected static const string SHARED_ROOT = "$profile:/506IRRU/ArsenalKits/Shared/";
	protected static const string INDEX_FILE = "Kits.json";
	protected static const string GRS_PREFIX = "GRS: ";
	protected static const string GRS_ACTIVE_NAME = "GRS: Active cross-server";
	protected static const string GRS_ACTIVE_PATH = "$profile:GRS_LockerCrossServer/active.json";
	protected static const string GRS_SHARED_ROOT = "$profile:GRS_Locker/players/__grs_shared__/";
	protected static const int MAX_NAME_LENGTH = 48;
	static const int MAX_LOADOUT_SIZE = 524288;

	static bool Capture(IEntity character, out string loadout)
	{
		loadout = "";
		if (!character)
			return false;

		JsonSaveContext saveContext = new JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(character, saveContext))
			return false;

		loadout = saveContext.SaveToString();
		return !loadout.IsEmpty() && loadout.Length() <= MAX_LOADOUT_SIZE;
	}

	static bool Apply(IEntity character, string loadout, out string error)
	{
		error = "";
		if (!character || loadout.IsEmpty() || loadout.Length() > MAX_LOADOUT_SIZE)
		{
			error = "invalid-loadout";
			return false;
		}
		return IRRU_LoadoutManagerStorage.ApplyComplete(character, loadout, error);
	}

	static bool ListKits(bool shared, out array<string> names, out string error, bool includeGRS = true)
	{
		names = {};
		error = "";
		string root = GetRoot(shared);

		string indexPath = root + INDEX_FILE;
		if (FileIO.FileExists(indexPath))
		{
			JsonLoadContext loadContext = new JsonLoadContext();
			if (!loadContext.LoadFromFile(indexPath) || !loadContext.ReadValue("names", names))
			{
				error = "index-read";
				return false;
			}
		}

		if (!names)
			names = {};
		if (includeGRS)
		{
			array<string> paths;
			array<string> grsNames;
			ListGRSKits(shared, grsNames, paths);
			foreach (string grsName : grsNames)
				names.Insert(grsName);
		}
		return true;
	}

	static bool IsGRSKit(string name)
	{
		return name.StartsWith(GRS_PREFIX);
	}

	protected static void ListGRSKits(bool shared, out array<string> names, out array<string> paths)
	{
		names = {};
		paths = {};
		if (!shared && FileIO.FileExists(GRS_ACTIVE_PATH))
		{
			names.Insert(GRS_ACTIVE_NAME);
			paths.Insert(GRS_ACTIVE_PATH);
		}

		array<string> files = {};
		if (shared)
			FileIO.FindFiles(files.Insert, GRS_SHARED_ROOT, ".json");
		else
		{
			FileIO.FindFiles(files.Insert, "$profile:.save/", ".json");
			FileIO.FindFiles(files.Insert, "$profile:GRS_Locker/saves/", ".json");
		}
		files.Sort();
		foreach (string path : files)
		{
			// The .save directory also contains unrelated game and addon data.
			if (shared && path.IndexOf("GRSKit_sharedkits_") < 0)
				continue;
			if (!shared && path.IndexOf("GRSLockerKit_") < 0 && path.IndexOf("GRS_Locker/saves/") < 0)
				continue;
			JsonLoadContext context = new JsonLoadContext();
			string name;
			string bank;
			if (!context.LoadFromFile(path) || !context.ReadValue("name", name) || !context.ReadValue("kit", bank) || name.IsEmpty() || bank.IsEmpty())
			{
				Print("[IRRU Kits] Could not read GRS kit metadata: " + path, LogLevel.WARNING);
				continue;
			}
			if ((shared && bank != "sharedkits") || name.IndexOf("\n") >= 0 || name.IndexOf("\r") >= 0 || bank.IndexOf("\n") >= 0 || bank.IndexOf("\r") >= 0)
			{
				Print("[IRRU Kits] Invalid GRS kit metadata: " + path, LogLevel.WARNING);
				continue;
			}
			string displayName = GRS_PREFIX + bank + " / " + name;
			if (FindName(names, displayName) >= 0)
				continue;
			names.Insert(displayName);
			paths.Insert(path);
		}
	}

	protected static bool LoadGRSKit(bool shared, string name, out string loadout, out string error)
	{
		array<string> names;
		array<string> paths;
		ListGRSKits(shared, names, paths);
		int index = FindName(names, name);
		if (index < 0)
		{
			error = "grs-kit-not-found";
			return false;
		}
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(paths[index]))
		{
			error = "grs-kit-read";
			return false;
		}
		if (names[index] == GRS_ACTIVE_NAME)
		{
			int magic;
			int version;
			if (!context.ReadValue("shapeMagic", magic) || magic != 1196180299 || !context.ReadValue("blobVersion", version) || version != 2 || !context.ReadValue("loadout", loadout))
			{
				error = "invalid-grs-active-kit";
				return false;
			}
		}
		else
		{
			FileHandle file = FileIO.OpenFile(paths[index], FileMode.READ);
			if (!file)
			{
				error = "grs-kit-read";
				return false;
			}
			string line;
			while (file.ReadLine(line) >= 0)
			{
				loadout += line + "\n";
				if (loadout.Length() > MAX_LOADOUT_SIZE)
					break;
			}
			file.Close();
		}
		if (loadout.IsEmpty() || loadout.Length() > MAX_LOADOUT_SIZE)
		{
			error = "invalid-grs-kit-size";
			return false;
		}
		return true;
	}

	static bool SaveKit(bool shared, string name, string loadout, bool overwrite, out string error)
	{
		error = "";
		if (!IsValidName(name) || loadout.IsEmpty() || loadout.Length() > MAX_LOADOUT_SIZE)
		{
			error = "invalid-data";
			return false;
		}
		FileIO.MakeDirectory(GetRoot(shared));

		array<string> names;
		if (!ListKits(shared, names, error, false))
			return false;
		int foundIndex = FindName(names, name);
		if (foundIndex >= 0 && !overwrite)
		{
			error = "already-exists";
			return false;
		}

		string storedName = name;
		if (foundIndex >= 0)
			storedName = names[foundIndex];
		string path = GetRoot(shared) + storedName + ".json";
		JsonSaveContext saveContext = new JsonSaveContext();
		if (!saveContext.WriteValue("name", storedName) || !saveContext.WriteValue("loadout", loadout) || !saveContext.SaveToFile(path))
		{
			error = "kit-write";
			return false;
		}

		if (foundIndex >= 0)
			return true;

		names.Insert(name);
		if (!SaveIndex(shared, names))
		{
			FileIO.DeleteFile(path);
			error = "index-write";
			return false;
		}
		return true;
	}

	static bool LoadKit(bool shared, string name, out string loadout, out string error)
	{
		loadout = "";
		error = "";
		if (IsGRSKit(name))
			return LoadGRSKit(shared, name, loadout, error);
		if (!IsValidName(name))
		{
			error = "invalid-name";
			return false;
		}

		JsonLoadContext loadContext = new JsonLoadContext();
		if (!loadContext.LoadFromFile(GetRoot(shared) + name + ".json") || !loadContext.ReadValue("loadout", loadout) || loadout.IsEmpty())
		{
			error = "kit-read";
			return false;
		}
		return true;
	}

	static bool DeleteKit(bool shared, string name, out string error)
	{
		error = "";
		if (IsGRSKit(name))
		{
			error = "grs-kit-read-only";
			return false;
		}
		array<string> names;
		if (!ListKits(shared, names, error, false))
			return false;
		int index = FindName(names, name);
		if (index < 0)
		{
			error = "not-found";
			return false;
		}

		if (!FileIO.DeleteFile(GetRoot(shared) + names[index] + ".json"))
		{
			error = "kit-delete";
			return false;
		}
		names.Remove(index);
		if (!SaveIndex(shared, names))
		{
			error = "index-write";
			return false;
		}
		return true;
	}

	static bool RenameKit(bool shared, string oldName, string newName, out string error)
	{
		error = "";
		if (IsGRSKit(oldName))
		{
			error = "grs-kit-read-only";
			return false;
		}
		if (!IsValidName(newName))
		{
			error = "invalid-name";
			return false;
		}
		array<string> names;
		if (!ListKits(shared, names, error, false))
			return false;
		int oldIndex = FindName(names, oldName);
		if (oldIndex < 0)
		{
			error = "not-found";
			return false;
		}
		if (FindName(names, newName) >= 0)
		{
			error = "already-exists";
			return false;
		}

		string loadout;
		if (!LoadKit(shared, names[oldIndex], loadout, error))
			return false;

		string oldPath = GetRoot(shared) + names[oldIndex] + ".json";
		if (!SaveKit(shared, newName, loadout, false, error))
			return false;
		if (!FileIO.DeleteFile(oldPath))
		{
			error = "kit-delete-old";
			return false;
		}

		names.Remove(oldIndex);
		if (!SaveIndex(shared, names))
		{
			error = "index-write";
			return false;
		}
		return true;
	}

	static bool IsValidName(string name)
	{
		if (name.IsEmpty() || name.Length() > MAX_NAME_LENGTH || name.Trim() != name || name == "." || name == "..")
			return false;
		if (name.EndsWith("."))
			return false;

		for (int i = 0; i < name.Length(); i++)
		{
			string character = name.Substring(i, 1);
			if (character == "/" || character == "\\" || character == ":" || character == "*" || character == "?" || character == "\"" || character == "<" || character == ">" || character == "|" || character == "\r" || character == "\n")
				return false;
		}
		return true;
	}

	protected static int FindName(array<string> names, string name)
	{
		string candidate = name;
		candidate.ToLower();
		for (int i = 0; i < names.Count(); i++)
		{
			string existing = names[i];
			existing.ToLower();
			if (existing == candidate)
				return i;
		}
		return -1;
	}

	protected static string GetRoot(bool shared)
	{
		if (shared)
			return SHARED_ROOT;
		return PERSONAL_ROOT;
	}

	protected static bool SaveIndex(bool shared, array<string> names)
	{
		JsonSaveContext saveContext = new JsonSaveContext();
		return saveContext.WriteValue("names", names) && saveContext.SaveToFile(GetRoot(shared) + INDEX_FILE);
	}
}
