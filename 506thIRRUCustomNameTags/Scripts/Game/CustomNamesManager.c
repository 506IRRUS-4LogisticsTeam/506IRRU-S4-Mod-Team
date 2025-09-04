//------------------------------------------------------------------------------------------------
class CustomNameEntry
{
	string m_sCustomName;       // The custom name
	int m_iLastUpdated;         // Unix timestamp of last update
	string m_sLastPlayerName;   // Last known player name for reference
}

class CustomNamesManager
{
	protected ref map<string, ref CustomNameEntry> m_CustomNames = new map<string, ref CustomNameEntry>(); // UID -> CustomNameEntry
	protected string m_SaveFilePath = "$profile:custom_names.json";
	
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
		if (GetGame().InPlayMode() && Replication.IsServer())
		{
			LoadCustomNames();
			Print("CustomNamesManager: Server-side initialization complete");
		}
		
		Print("CustomNamesManager: Initialized with UID-based persistence");
	}
	
	//------------------------------------------------------------------------------------------------
	bool SetCustomName(string playerId, string customName)
	{
		string playerUID = GetPlayerUID(playerId.ToInt());
		if (playerUID.IsEmpty())
		{
			Print(string.Format("CustomNamesManager: Could not get UID for player %1", playerId));
			return false;
		}
		if (!ValidateCustomName(customName))
			return false;
		CustomNameEntry entry;
		if (!m_CustomNames.Find(playerUID, entry))
		{
			entry = new CustomNameEntry();
			m_CustomNames[playerUID] = entry;
		}
		
		entry.m_sCustomName = customName;
		entry.m_iLastUpdated = System.GetUnixTime();
		entry.m_sLastPlayerName = GetPlayerName(playerId.ToInt());
		
		Print(string.Format("CustomNamesManager: Set name '%1' for UID %2 (Player: %3)", 
			customName, playerUID, entry.m_sLastPlayerName));
		if (GetGame().InPlayMode() && Replication.IsServer())
			SaveCustomNames();
			
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateCustomNameLocal(string playerId, string customName)
	{
		string playerUID = GetPlayerUID(playerId.ToInt());
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
		entry.m_sLastPlayerName = GetPlayerName(playerId.ToInt());
	}
	
	//------------------------------------------------------------------------------------------------
	string GetCustomName(string playerId)
	{
		string playerUID = GetPlayerUID(playerId.ToInt());
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
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "";
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		if (!playerController)
			return "";
		string playerId_str = playerId.ToString();
		string playerName = playerManager.GetPlayerName(playerId);
		if (playerName.IsEmpty())
			playerName = "Unknown";
		string pseudoUID = string.Format("ARMA_%1_%2", playerId, playerName.Hash());
		
		return pseudoUID;
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
				Print(string.Format("CustomNamesManager: Found existing name '%1' for player %2", 
					entry.m_sCustomName, playerId));
				entry.m_sLastPlayerName = GetPlayerName(playerId);
				entry.m_iLastUpdated = System.GetUnixTime();
				if (GetGame().InPlayMode() && Replication.IsServer())
					SaveCustomNames();
				Print(string.Format("CustomNamesManager: Custom name '%1' is available for player %2", 
					entry.m_sCustomName, playerId));
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
	bool RemoveCustomName(string playerId)
	{
		string playerUID = GetPlayerUID(playerId.ToInt());
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
		FileHandle file = FileIO.OpenFile(m_SaveFilePath, FileMode.READ);
		if (!file)
		{
			Print("CustomNamesManager: No existing save file found, starting fresh");
			return;
		}
		
		string jsonContent;
		string line;
		while (file.ReadLine(line) != -1)
		{
			jsonContent += line;
		}
		file.Close();
		
		if (jsonContent.IsEmpty())
			return;
		ParseCustomNamesJson(jsonContent);
		
		Print(string.Format("CustomNamesManager: Loaded %1 custom names from file", m_CustomNames.Count()));
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SaveCustomNames()
	{
		FileHandle file = FileIO.OpenFile(m_SaveFilePath, FileMode.WRITE);
		if (!file)
		{
			Print("CustomNamesManager: Failed to open save file for writing");
			return;
		}
		string jsonContent = CreateCustomNamesJson();
		file.WriteLine(jsonContent);
		file.Close();
		
		Print(string.Format("CustomNamesManager: Saved %1 custom names to file", m_CustomNames.Count()));
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
		jsonContent.Replace("{\n", "");
		jsonContent.Replace("\n}", "");
		array<string> lines = {};
		jsonContent.Split("\n", lines, false);
		
		string currentUID = "";
		CustomNameEntry currentEntry = null;
		
		foreach (string line : lines)
		{
			line.Trim();
			if (line.IsEmpty() || line == "," || line == "{" || line == "}")
				continue;
			if (line.Contains("\": {"))
			{
				int startQuote = line.IndexOf("\"");
				if (startQuote >= 0)
				{
					string afterFirstQuote = line.Substring(startQuote + 1, line.Length() - startQuote - 1);
					int endQuote = afterFirstQuote.IndexOf("\"");
					if (endQuote >= 0)
					{
						currentUID = afterFirstQuote.Substring(0, endQuote);
						currentEntry = new CustomNameEntry();
						m_CustomNames[currentUID] = currentEntry;
					}
				}
			}
			else if (currentEntry && line.Contains("customName"))
			{
				string name = ExtractJsonStringValue(line);
				currentEntry.m_sCustomName = name;
			}
			else if (currentEntry && line.Contains("lastUpdated"))
			{
				string timeStr = ExtractJsonNumberValue(line);
				currentEntry.m_iLastUpdated = timeStr.ToInt();
			}
			else if (currentEntry && line.Contains("lastPlayerName"))
			{
				string playerName = ExtractJsonStringValue(line);
				currentEntry.m_sLastPlayerName = playerName;
			}
		}
		
		Print(string.Format("CustomNamesManager: Parsed %1 entries from JSON", m_CustomNames.Count()));
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
