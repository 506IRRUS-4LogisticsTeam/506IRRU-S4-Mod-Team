//------------------------------------------------------------------------------------------------
class CustomNameEntry
{
	string m_sCustomName;
	int m_iLastUpdated;
	string m_sLastPlayerName;
}

class CustomNamesManager
{
	protected ref map<string, ref CustomNameEntry> m_CustomNames = new map<string, ref CustomNameEntry>();
	protected string m_SaveFilePath = "$profile:custom_names.json";
	protected bool m_bInitialized = false;
	
	protected static ref CustomNamesManager s_Instance;
	
	//------------------------------------------------------------------------------------------------
	static CustomNamesManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new CustomNamesManager();
		
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	void CustomNamesManager()
	{
		if (m_bInitialized)
			return;
		
		m_bInitialized = true;
		Print("[CustomNames] CustomNamesManager initialized", LogLevel.NORMAL);
		
		if (GetGame().InPlayMode() && Replication.IsServer())
		{
			Print("[CustomNames] Server-side initialization: Loading persistence and setting up player events", LogLevel.NORMAL);
			LoadCustomNames();
			
			SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			if (gameMode)
			{
				gameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
				gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
			}
			else
			{
				Print("[CustomNames] GameMode not available during initialization", LogLevel.WARNING);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnPlayerConnected(int playerId)
	{
		CustomNamesNetworkEntity net = CustomNamesNetworkEntity.Get();
		if (net)
			net.OnPlayerConnectedWithRetry(playerId, 0);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnPlayerDisconnected(int playerId)
	{
		// Custom names are already persisted in the file, nothing to do here
	}
	
	//------------------------------------------------------------------------------------------------
	protected void BroadcastRestoredName(int playerId, string customName)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
		{
			SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
			if (chatComp)
			{
				CustomNamesNetworkEntity net = CustomNamesNetworkEntity.Get();
				if (net)
				{
					net.BroadcastNameDelayed(playerId, customName);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	bool SetCustomName(int playerId, string customName)
	{
		string playerUID = GetPlayerUID(playerId);
		if (playerUID.IsEmpty())
			return false;
		
		if (!customName.IsEmpty() && !ValidateCustomName(customName))
			return false;
		
		CustomNameEntry entry;
		if (!m_CustomNames.Find(playerUID, entry))
		{
			entry = new CustomNameEntry();
			m_CustomNames[playerUID] = entry;
		}
		
		entry.m_sCustomName = customName;
		entry.m_iLastUpdated = System.GetUnixTime();
		entry.m_sLastPlayerName = GetPlayerName(playerId);
		
		if (GetGame().InPlayMode() && Replication.IsServer())
			SaveCustomNames();
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateCustomNameLocal(int playerId, string customName)
	{
		string playerUID = GetPlayerUID(playerId);
		if (playerUID.IsEmpty())
			return;
		
		CustomNameEntry entry;
		if (!m_CustomNames.Find(playerUID, entry))
		{
			entry = new CustomNameEntry();
			m_CustomNames[playerUID] = entry;
		}
		
		entry.m_sCustomName = customName;
		entry.m_iLastUpdated = System.GetUnixTime();
		entry.m_sLastPlayerName = GetPlayerName(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	string GetCustomName(int playerId)
	{
		string playerUID = GetPlayerUID(playerId);
		if (playerUID.IsEmpty())
			return "";
		
		CustomNameEntry entry;
		if (m_CustomNames.Find(playerUID, entry))
			return entry.m_sCustomName;
		
		return "";
	}
	
	//------------------------------------------------------------------------------------------------
	string GetCustomNameByUID(string playerUID)
	{
		CustomNameEntry entry;
		if (m_CustomNames.Find(playerUID, entry))
			return entry.m_sCustomName;
		
		return "";
	}
	
	//------------------------------------------------------------------------------------------------
	protected string GetPlayerUID(int playerId)
	{
		string identityId = "";
		
		if (GetGame().InPlayMode() && Replication.IsServer())
		{
			BackendApi backendApi = GetGame().GetBackendApi();
			if (backendApi)
				identityId = backendApi.GetPlayerIdentityId(playerId);
		}
		
		if (!identityId.IsEmpty())
			return identityId;
		
		// Fallback to pseudo-UID
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "";
		
		string playerName = playerManager.GetPlayerName(playerId);
		if (playerName.IsEmpty())
			playerName = "Unknown";
		
		return string.Format("FALLBACK_%1_%2", playerId, playerName.Hash());
	}
	
	//------------------------------------------------------------------------------------------------
	protected string GetPlayerName(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "Unknown";
		
		return playerManager.GetPlayerName(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void CheckAndRestoreCustomName(int playerId)
	{
		string playerUID = GetPlayerUID(playerId);
		if (playerUID.IsEmpty())
			return;
		
		CustomNameEntry entry;
		if (m_CustomNames.Find(playerUID, entry))
		{
			if (!entry.m_sCustomName.IsEmpty())
			{
				entry.m_sLastPlayerName = GetPlayerName(playerId);
				entry.m_iLastUpdated = System.GetUnixTime();
				
				if (GetGame().InPlayMode() && Replication.IsServer())
					SaveCustomNames();
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	map<string, string> GetAllCustomNames()
	{
		map<string, string> result = new map<string, string>();
		foreach (string uid, CustomNameEntry entry : m_CustomNames)
		{
			result[uid] = entry.m_sCustomName;
		}
		return result;
	}
	
	//------------------------------------------------------------------------------------------------
	bool RemoveCustomName(int playerId)
	{
		string playerUID = GetPlayerUID(playerId);
		if (playerUID.IsEmpty())
			return false;
		
		CustomNameEntry entry;
		if (m_CustomNames.Find(playerUID, entry))
		{
			m_CustomNames.Remove(playerUID);
			if (GetGame().InPlayMode() && Replication.IsServer())
				SaveCustomNames();
				
			return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool ValidateCustomName(string name)
	{
		if (name.Length() < 3 || name.Length() > 20)
			return false;
		for (int i = 0; i < name.Length(); i++)
		{
			string char = name.Get(i);
			if (!IsValidCharacter(char))
				return false;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsValidCharacter(string char)
	{
		int ascii = char.ToAscii();
		if ((ascii >= 65 && ascii <= 90) || (ascii >= 97 && ascii <= 122))
			return true;
		if (ascii >= 48 && ascii <= 57)
			return true;
		if (ascii == 32 || ascii == 45 || ascii == 95 || ascii == 46)
			return true;
			
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void LoadCustomNames()
	{
		Print(string.Format("[CustomNames] Loading custom names from %1", m_SaveFilePath), LogLevel.NORMAL);
		FileHandle file = FileIO.OpenFile(m_SaveFilePath, FileMode.READ);
		if (!file)
		{
			Print("[CustomNames] No existing custom names file found - starting with empty database", LogLevel.NORMAL);
			return;
		}
		
		string jsonContent;
		string line;
		while (file.ReadLine(line) != -1)
		{
			jsonContent += line + "\n";
		}
		file.Close();
		
		if (jsonContent.IsEmpty())
		{
			Print("[CustomNames] Custom names file is empty", LogLevel.NORMAL);
			return;
		}
		
		Print(string.Format("[CustomNames] Loaded %1 bytes of JSON data, parsing...", jsonContent.Length()), LogLevel.NORMAL);
		ParseCustomNamesJson(jsonContent);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SaveCustomNames()
	{
		FileHandle file = FileIO.OpenFile(m_SaveFilePath, FileMode.WRITE);
		if (!file)
		{
			// Try APPEND mode which might create the file
			file = FileIO.OpenFile(m_SaveFilePath, FileMode.APPEND);
			if (!file)
				return;
			
			// Close and reopen in WRITE mode to overwrite
			file.Close();
			file = FileIO.OpenFile(m_SaveFilePath, FileMode.WRITE);
			if (!file)
				return;
		}
		
		string jsonContent = CreateCustomNamesJson();
		file.WriteLine(jsonContent);
		file.Close();
	}
	
	//------------------------------------------------------------------------------------------------
	protected string CreateCustomNamesJson()
	{
		string json = "{\n";
		int count = 0;
		int total = m_CustomNames.Count();
		
		foreach (string playerUID, CustomNameEntry entry : m_CustomNames)
		{
			count++;
			json += string.Format("  \"%1\": {\n", playerUID);
			json += string.Format("    \"customName\": \"%1\",\n", entry.m_sCustomName);
			json += string.Format("    \"lastUpdated\": %1,\n", entry.m_iLastUpdated);
			json += string.Format("    \"lastPlayerName\": \"%1\"\n", entry.m_sLastPlayerName);
			json += "  }";
			
			if (count < total)
				json += ",";
			json += "\n";
		}
		
		json += "}";
		return json;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ParseCustomNamesJson(string jsonContent)
	{
		int initialCount = m_CustomNames.Count();
		
		// Remove outer braces and clean up
		jsonContent.Replace("{", "");
		jsonContent.Replace("}", "");
		
		// Split by newlines
		array<string> lines = {};
		jsonContent.Split("\n", lines, false);
		
		string currentUID = "";
		CustomNameEntry currentEntry = null;
		int parsedEntries = 0;
		
		foreach (string line : lines)
		{
			line.Trim();
			
			if (line.IsEmpty() || line == ",")
				continue;
				
			// Check if this line starts a new entry
			if (line.Contains("\": ") && !line.Contains("customName") && !line.Contains("lastUpdated") && !line.Contains("lastPlayerName"))
			{
				int firstQuote = line.IndexOf("\"");
				if (firstQuote >= 0)
				{
					string afterFirstQuote = line.Substring(firstQuote + 1, line.Length() - firstQuote - 1);
					int secondQuote = afterFirstQuote.IndexOf("\"");
					if (secondQuote >= 0)
					{
						currentUID = afterFirstQuote.Substring(0, secondQuote);
						currentEntry = new CustomNameEntry();
						m_CustomNames[currentUID] = currentEntry;
						parsedEntries++;
					}
				}
			}
			else if (currentEntry)
			{
				if (line.Contains("\"customName\""))
				{
					currentEntry.m_sCustomName = ExtractJsonStringValue(line);
				}
				else if (line.Contains("\"lastUpdated\""))
				{
					string timeStr = ExtractJsonNumberValue(line);
					currentEntry.m_iLastUpdated = timeStr.ToInt();
				}
				else if (line.Contains("\"lastPlayerName\""))
				{
					currentEntry.m_sLastPlayerName = ExtractJsonStringValue(line);
				}
			}
		}
		
		int finalCount = m_CustomNames.Count();
		Print(string.Format("[CustomNames] JSON parsing complete: %1 entries parsed, %2 total custom names loaded", 
			parsedEntries, finalCount), LogLevel.NORMAL);
			
		// Log each parsed entry for debugging
		foreach (string uid, CustomNameEntry entry : m_CustomNames)
		{
			if (!entry.m_sCustomName.IsEmpty())
			{
				Print(string.Format("[CustomNames] Loaded: UID='%1' -> Name='%2' (Last: %3)", 
					uid, entry.m_sCustomName, entry.m_sLastPlayerName), LogLevel.NORMAL);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected string ExtractJsonStringValue(string line)
	{
		int colonPos = line.IndexOf(":");
		if (colonPos >= 0)
		{
			string afterColon = line.Substring(colonPos, line.Length() - colonPos);
			int firstQuote = afterColon.IndexOf("\"");
			if (firstQuote >= 0)
			{
				string afterFirstQuote = afterColon.Substring(firstQuote + 1, afterColon.Length() - firstQuote - 1);
				int secondQuote = afterFirstQuote.IndexOf("\"");
				if (secondQuote >= 0)
				{
					return afterFirstQuote.Substring(0, secondQuote);
				}
			}
		}
		return "";
	}
	
	//------------------------------------------------------------------------------------------------
	protected string ExtractJsonNumberValue(string line)
	{
		int colonPos = line.IndexOf(":");
		if (colonPos >= 0)
		{
			string value = line.Substring(colonPos + 1, line.Length() - colonPos - 1);
			value.Trim();
			value.Replace(",", "");
			return value;
		}
		return "0";
	}
}
