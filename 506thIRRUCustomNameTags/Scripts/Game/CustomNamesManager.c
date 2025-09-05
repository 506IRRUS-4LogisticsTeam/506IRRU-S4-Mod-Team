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
			
			SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			if (gameMode)
			{
				gameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
				gameMode.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);
				Print("CustomNamesManager: Hooked into player connection events");
			}
			
			Print("CustomNamesManager: Server-side initialization complete");
		}
		
		Print("CustomNamesManager: Initialized with Game Identity persistence");
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnPlayerConnected(int playerId)
	{
		Print(string.Format("[CustomNames][CONNECT] ============ PLAYER CONNECTED ============"), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][CONNECT] Player ID: %1", playerId), LogLevel.NORMAL);
		
		string identityId = GetPlayerUID(playerId);
		PlayerManager playerManager = GetGame().GetPlayerManager();
		string playerName = "";
		if (playerManager)
			playerName = playerManager.GetPlayerName(playerId);
		
		Print(string.Format("[CustomNames][CONNECT] Player Name: %1", playerName), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][CONNECT] Identity ID retrieved: %1", identityId), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][CONNECT] Is Identity Empty: %1", identityId.IsEmpty()), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][CONNECT] Identity Length: %1", identityId.Length()), LogLevel.NORMAL);
		
		if (!identityId.IsEmpty())
		{
			CustomNameEntry entry;
			if (m_CustomNames.Find(identityId, entry))
			{
				if (!entry.m_sCustomName.IsEmpty())
				{
					Print(string.Format("[CustomNames][CONNECT] ***** AUTO-RESTORE TRIGGERED *****"), LogLevel.NORMAL);
					Print(string.Format("[CustomNames][CONNECT] Restoring custom name: '%1'", entry.m_sCustomName), LogLevel.NORMAL);
					Print(string.Format("[CustomNames][CONNECT] For player ID: %1", playerId), LogLevel.NORMAL);
					Print(string.Format("[CustomNames][CONNECT] Previous player name was: '%1'", entry.m_sLastPlayerName), LogLevel.NORMAL);
					
					entry.m_sLastPlayerName = playerName;
					entry.m_iLastUpdated = System.GetUnixTime();
					
					Print(string.Format("[CustomNames][CONNECT] Scheduling broadcast in 1000ms"), LogLevel.NORMAL);
					GetGame().GetCallqueue().CallLater(BroadcastRestoredName, 1000, false, playerId.ToString(), entry.m_sCustomName);
					Print(string.Format("[CustomNames][CONNECT] **********************************"), LogLevel.NORMAL);
				}
			}
			else
			{
				Print(string.Format("[CustomNames][CONNECT] No saved custom name found for identity %1", identityId), LogLevel.NORMAL);
			}
		}
		else
		{
			Print(string.Format("[CustomNames][CONNECT] WARNING: Identity ID is empty for player %1", playerId), LogLevel.WARNING);
		}
		
		Print(string.Format("[CustomNames][CONNECT] =========================================="), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnPlayerDisconnected(int playerId)
	{
		Print(string.Format("[CustomNames][DISCONNECT] ========= PLAYER DISCONNECTED ========="), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][DISCONNECT] Player ID: %1", playerId), LogLevel.NORMAL);
		
		string identityId = GetPlayerUID(playerId);
		Print(string.Format("[CustomNames][DISCONNECT] Identity ID: %1", identityId), LogLevel.NORMAL);
		
		if (!identityId.IsEmpty())
		{
			CustomNameEntry entry;
			if (m_CustomNames.Find(identityId, entry) && !entry.m_sCustomName.IsEmpty())
			{
				Print(string.Format("[CustomNames][DISCONNECT] Player had custom name: '%1'", entry.m_sCustomName), LogLevel.NORMAL);
				Print(string.Format("[CustomNames][DISCONNECT] Custom name preserved for next connection"), LogLevel.NORMAL);
			}
		}
		
		Print(string.Format("[CustomNames][DISCONNECT] ======================================="), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void BroadcastRestoredName(string playerId, string customName)
	{
		Print(string.Format("[CustomNames][RESTORE] Broadcasting restored name..."), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][RESTORE] Player ID: %1, Custom Name: '%2'", playerId, customName), LogLevel.NORMAL);
		
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
		{
			Print(string.Format("[CustomNames][RESTORE] PlayerController found"), LogLevel.NORMAL);
			
			SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
			if (chatComp)
			{
				Print(string.Format("[CustomNames][RESTORE] ChatComponent found, calling RpcAll_UpdateCustomName"), LogLevel.NORMAL);
				chatComp.RpcAll_UpdateCustomName(playerId, customName);
				Print(string.Format("[CustomNames][RESTORE] Broadcast complete for restored name '%1'", customName), LogLevel.NORMAL);
			}
			else
			{
				Print(string.Format("[CustomNames][RESTORE] ERROR: No ChatComponent found"), LogLevel.ERROR);
			}
		}
		else
		{
			Print(string.Format("[CustomNames][RESTORE] ERROR: No PlayerController found"), LogLevel.ERROR);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	bool SetCustomName(string playerId, string customName)
	{
		Print(string.Format("[CustomNames][SET] Setting custom name for player %1", playerId), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][SET] New name: '%1'", customName), LogLevel.NORMAL);
		
		string playerUID = GetPlayerUID(playerId.ToInt());
		if (playerUID.IsEmpty())
		{
			Print(string.Format("[CustomNames][SET] ERROR: Could not get UID for player %1", playerId), LogLevel.ERROR);
			return false;
		}
		
		Print(string.Format("[CustomNames][SET] Got UID: %1", playerUID), LogLevel.NORMAL);
		
		if (!ValidateCustomName(customName))
		{
			Print(string.Format("[CustomNames][SET] Validation failed for name: '%1'", customName), LogLevel.WARNING);
			return false;
		}
		
		CustomNameEntry entry;
		if (!m_CustomNames.Find(playerUID, entry))
		{
			Print(string.Format("[CustomNames][SET] Creating new entry for UID %1", playerUID), LogLevel.NORMAL);
			entry = new CustomNameEntry();
			m_CustomNames[playerUID] = entry;
		}
		else
		{
			Print(string.Format("[CustomNames][SET] Updating existing entry for UID %1", playerUID), LogLevel.NORMAL);
			Print(string.Format("[CustomNames][SET] Previous name was: '%1'", entry.m_sCustomName), LogLevel.NORMAL);
		}
		
		entry.m_sCustomName = customName;
		entry.m_iLastUpdated = System.GetUnixTime();
		entry.m_sLastPlayerName = GetPlayerName(playerId.ToInt());
		
		Print(string.Format("[CustomNames][SET] SUCCESS: Name set to '%1' for UID %2", customName, playerUID), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][SET] Player name: %1, Timestamp: %2", 
			entry.m_sLastPlayerName, entry.m_iLastUpdated), LogLevel.NORMAL);
		
		if (GetGame().InPlayMode() && Replication.IsServer())
		{
			Print(string.Format("[CustomNames][SET] Triggering save to persistence file"), LogLevel.NORMAL);
			SaveCustomNames();
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateCustomNameLocal(string playerId, string customName)
	{
		Print(string.Format("[CustomNames][LOCAL] Updating local cache for player %1", playerId), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][LOCAL] New name: '%1'", customName), LogLevel.NORMAL);
		
		string playerUID = GetPlayerUID(playerId.ToInt());
		if (playerUID.IsEmpty())
		{
			Print(string.Format("[CustomNames][LOCAL] ERROR: Could not get UID, aborting update"), LogLevel.ERROR);
			return;
		}
		
		Print(string.Format("[CustomNames][LOCAL] Got UID: %1", playerUID), LogLevel.NORMAL);
		
		CustomNameEntry entry;
		if (!m_CustomNames.Find(playerUID, entry))
		{
			Print(string.Format("[CustomNames][LOCAL] Creating new cache entry for UID %1", playerUID), LogLevel.NORMAL);
			entry = new CustomNameEntry();
			m_CustomNames[playerUID] = entry;
		}
		else
		{
			Print(string.Format("[CustomNames][LOCAL] Updating existing cache entry"), LogLevel.NORMAL);
			Print(string.Format("[CustomNames][LOCAL] Previous cached name: '%1'", entry.m_sCustomName), LogLevel.NORMAL);
		}
		
		entry.m_sCustomName = customName;
		entry.m_iLastUpdated = System.GetUnixTime();
		entry.m_sLastPlayerName = GetPlayerName(playerId.ToInt());
		
		Print(string.Format("[CustomNames][LOCAL] Local cache updated successfully"), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][LOCAL] Cached: UID %1 => '%2'", playerUID, customName), LogLevel.NORMAL);
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
		Print(string.Format("[CustomNames][UID] +++++ GETTING PLAYER UID +++++"), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][UID] Player ID: %1", playerId), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][UID] InPlayMode: %1", GetGame().InPlayMode()), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][UID] IsServer: %1", Replication.IsServer()), LogLevel.NORMAL);
		
		string identityId = "";
		
		// Try to get Game Identity if BackendApi is available
		if (GetGame().InPlayMode() && Replication.IsServer())
		{
			Print(string.Format("[CustomNames][UID] Attempting to get BackendApi"), LogLevel.NORMAL);
			
			BackendApi backendApi = GetGame().GetBackendApi();
			if (backendApi)
			{
				Print(string.Format("[CustomNames][UID] BackendApi available, calling GetPlayerIdentityId"), LogLevel.NORMAL);
				identityId = backendApi.GetPlayerIdentityId(playerId);
				Print(string.Format("[CustomNames][UID] GetPlayerIdentityId returned: '%1'", identityId), LogLevel.NORMAL);
			}
			else
			{
				Print(string.Format("[CustomNames][UID] WARNING: BackendApi is NULL"), LogLevel.WARNING);
			}
		}
		else
		{
			Print(string.Format("[CustomNames][UID] Not in server play mode, cannot get identity"), LogLevel.NORMAL);
		}
		
		Print(string.Format("[CustomNames][UID] Identity ID result:"), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][UID]   - Raw value: '%1'", identityId), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][UID]   - IsEmpty: %1", identityId.IsEmpty()), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][UID]   - Length: %1", identityId.Length()), LogLevel.NORMAL);
		
		if (!identityId.IsEmpty())
		{
			Print(string.Format("[CustomNames][UID] SUCCESS: Using Game Identity ID"), LogLevel.NORMAL);
			Print(string.Format("[CustomNames][UID] Identity: %1", identityId), LogLevel.NORMAL);
			Print(string.Format("[CustomNames][UID] ++++++++++++++++++++++++++++++"), LogLevel.NORMAL);
			return identityId;
		}
		
		Print(string.Format("[CustomNames][UID] WARNING: Identity ID not available, using fallback"), LogLevel.WARNING);
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			Print(string.Format("[CustomNames][UID] ERROR: No PlayerManager available for fallback"), LogLevel.ERROR);
			Print(string.Format("[CustomNames][UID] ++++++++++++++++++++++++++++++"), LogLevel.NORMAL);
			return "";
		}
		
		string playerName = playerManager.GetPlayerName(playerId);
		Print(string.Format("[CustomNames][UID] Player name for fallback: '%1'", playerName), LogLevel.NORMAL);
		
		if (playerName.IsEmpty())
		{
			playerName = "Unknown";
			Print(string.Format("[CustomNames][UID] Player name was empty, using 'Unknown'"), LogLevel.WARNING);
		}
		
		string pseudoUID = string.Format("FALLBACK_%1_%2", playerId, playerName.Hash());
		Print(string.Format("[CustomNames][UID] FALLBACK: Using pseudo-UID: %1", pseudoUID), LogLevel.WARNING);
		Print(string.Format("[CustomNames][UID] ++++++++++++++++++++++++++++++"), LogLevel.NORMAL);
		
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
		Print(string.Format("[CustomNames][CHECK] Checking for existing custom name for player %1", playerId), LogLevel.NORMAL);
		
		string playerUID = GetPlayerUID(playerId);
		if (playerUID.IsEmpty())
		{
			Print(string.Format("[CustomNames][CHECK] No UID available, cannot check"), LogLevel.WARNING);
			return;
		}
		
		Print(string.Format("[CustomNames][CHECK] Checking UID: %1", playerUID), LogLevel.NORMAL);
		
		CustomNameEntry entry;
		if (m_CustomNames.Find(playerUID, entry))
		{
			Print(string.Format("[CustomNames][CHECK] Found entry for UID %1", playerUID), LogLevel.NORMAL);
			
			if (!entry.m_sCustomName.IsEmpty())
			{
				Print(string.Format("[CustomNames][CHECK] Existing custom name found: '%1'", entry.m_sCustomName), LogLevel.NORMAL);
				Print(string.Format("[CustomNames][CHECK] Updating metadata"), LogLevel.NORMAL);
				
				entry.m_sLastPlayerName = GetPlayerName(playerId);
				entry.m_iLastUpdated = System.GetUnixTime();
				
				if (GetGame().InPlayMode() && Replication.IsServer())
				{
					Print(string.Format("[CustomNames][CHECK] Saving updated metadata to file"), LogLevel.NORMAL);
					SaveCustomNames();
				}
				
				Print(string.Format("[CustomNames][CHECK] Custom name '%1' ready for player %2", 
					entry.m_sCustomName, playerId), LogLevel.NORMAL);
			}
			else
			{
				Print(string.Format("[CustomNames][CHECK] Entry exists but custom name is empty"), LogLevel.NORMAL);
			}
		}
		else
		{
			Print(string.Format("[CustomNames][CHECK] No entry found for UID %1", playerUID), LogLevel.NORMAL);
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
		Print(string.Format("[CustomNames][JSON] ========== LOADING PERSISTENCE FILE =========="), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][JSON] File path: %1", m_SaveFilePath), LogLevel.NORMAL);
		
		FileHandle file = FileIO.OpenFile(m_SaveFilePath, FileMode.READ);
		if (!file)
		{
			Print(string.Format("[CustomNames][JSON] No existing save file found at %1, starting fresh", m_SaveFilePath), LogLevel.WARNING);
			Print(string.Format("[CustomNames][JSON] ================================================="), LogLevel.NORMAL);
			return;
		}
		
		Print(string.Format("[CustomNames][JSON] File opened successfully"), LogLevel.NORMAL);
		
		string jsonContent;
		string line;
		int lineCount = 0;
		while (file.ReadLine(line) != -1)
		{
			jsonContent += line;
			lineCount++;
		}
		file.Close();
		
		Print(string.Format("[CustomNames][JSON] Read %1 lines from file", lineCount), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][JSON] Total content length: %1 characters", jsonContent.Length()), LogLevel.NORMAL);
		
		if (jsonContent.IsEmpty())
		{
			Print(string.Format("[CustomNames][JSON] File was empty, no data to load"), LogLevel.WARNING);
			return;
		}
		
		Print(string.Format("[CustomNames][JSON] Parsing JSON content..."), LogLevel.NORMAL);
		ParseCustomNamesJson(jsonContent);
		
		Print(string.Format("[CustomNames][JSON] Successfully loaded %1 custom names from persistence file", m_CustomNames.Count()), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][JSON] ================================================="), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SaveCustomNames()
	{
		Print(string.Format("[CustomNames][JSON] ========== SAVING PERSISTENCE FILE =========="), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][JSON] File path: %1", m_SaveFilePath), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][JSON] Number of entries to save: %1", m_CustomNames.Count()), LogLevel.NORMAL);
		
		FileHandle file = FileIO.OpenFile(m_SaveFilePath, FileMode.WRITE);
		if (!file)
		{
			Print(string.Format("[CustomNames][JSON] ERROR: Failed to open save file for writing at %1", m_SaveFilePath), LogLevel.ERROR);
			Print(string.Format("[CustomNames][JSON] ================================================="), LogLevel.NORMAL);
			return;
		}
		
		Print(string.Format("[CustomNames][JSON] File opened for writing"), LogLevel.NORMAL);
		
		string jsonContent = CreateCustomNamesJson();
		
		Print(string.Format("[CustomNames][JSON] JSON content created, length: %1 characters", jsonContent.Length()), LogLevel.NORMAL);
		
		file.WriteLine(jsonContent);
		file.Close();
		
		Print(string.Format("[CustomNames][JSON] Successfully saved %1 custom names to file", m_CustomNames.Count()), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][JSON] File write complete and closed"), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][JSON] ================================================="), LogLevel.NORMAL);
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
		Print(string.Format("[CustomNames][JSON] Starting JSON parse"), LogLevel.NORMAL);
		
		jsonContent.Replace("{\n", "");
		jsonContent.Replace("\n}", "");
		array<string> lines = {};
		jsonContent.Split("\n", lines, false);
		
		Print(string.Format("[CustomNames][JSON] Split into %1 lines for parsing", lines.Count()), LogLevel.NORMAL);
		
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
						Print(string.Format("[CustomNames][JSON] Found entry for UID: %1", currentUID), LogLevel.NORMAL);
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
		
		Print(string.Format("[CustomNames][JSON] JSON parsing complete - parsed %1 entries", m_CustomNames.Count()), LogLevel.NORMAL);
		
		foreach (string uid, CustomNameEntry entry : m_CustomNames)
		{
			Print(string.Format("[CustomNames][JSON]   - UID: %1 => Name: '%2', LastPlayer: '%3'", 
				uid, entry.m_sCustomName, entry.m_sLastPlayerName), LogLevel.NORMAL);
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
