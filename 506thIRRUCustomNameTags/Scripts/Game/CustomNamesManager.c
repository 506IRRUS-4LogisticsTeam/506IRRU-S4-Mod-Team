//! Custom Names Manager - handles client-server communication for custom names

//! Entry for storing custom name data with metadata
class CustomNameEntry
{
	string m_sCustomName;       // The custom name
	int m_iLastUpdated;         // Unix timestamp of last update
	string m_sLastPlayerName;   // Last known player name for reference
	
	void CustomNameEntry()
	{
		m_sCustomName = "";
		m_iLastUpdated = 0;
		m_sLastPlayerName = "";
	}
}

class CustomNamesManager
{
	protected ref map<string, ref CustomNameEntry> m_CustomNames = new map<string, ref CustomNameEntry>(); // UID -> CustomNameEntry
	protected string m_SaveFilePath = "$profile:custom_names.json";
	
	// Singleton instance
	protected static ref CustomNamesManager s_Instance;
	
	//! Get singleton instance
	static CustomNamesManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new CustomNamesManager();
		
		return s_Instance;
	}
	
	//! Initialize the custom names system
	void CustomNamesManager()
	{
		// Load existing custom names from file (server only)
		if (GetGame().InPlayMode() && Replication.IsServer())
		{
			LoadCustomNames();
			
			// Note: Player connection monitoring will be handled differently
			// We'll check for existing names when players use commands instead
			Print("CustomNamesManager: Server-side initialization complete");
		}
		
		Print("CustomNamesManager: Initialized with UID-based persistence");
	}
	
	//! Set a custom name for a player using their UID
	bool SetCustomName(string playerId, string customName)
	{
		// Get player UID from player ID
		string playerUID = GetPlayerUID(playerId.ToInt());
		if (playerUID.IsEmpty())
		{
			Print(string.Format("CustomNamesManager: Could not get UID for player %1", playerId));
			return false;
		}
		
		// Validate the name
		if (!ValidateCustomName(customName))
			return false;
		
		// Create or update entry
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
		
		// Save to file (server only)
		if (GetGame().InPlayMode() && Replication.IsServer())
			SaveCustomNames();
			
		return true;
	}
	
	//! Update custom name locally (for network sync) - converts player ID to UID
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
	
	//! Get a player's custom name using their player ID
	string GetCustomName(string playerId)
	{
		string playerUID = GetPlayerUID(playerId.ToInt());
		if (playerUID.IsEmpty())
			return "";
		
		CustomNameEntry entry;
		if (m_CustomNames.Find(playerUID, entry))
			return entry.m_sCustomName;
		
		return ""; // No custom name set
	}
	
	//! Get a player's custom name using their UID directly
	string GetCustomNameByUID(string playerUID)
	{
		CustomNameEntry entry;
		if (m_CustomNames.Find(playerUID, entry))
			return entry.m_sCustomName;
		
		return "";
	}
	
	//! Get player UID from player ID
	protected string GetPlayerUID(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "";
		
		// Try to get the player controller first
		PlayerController playerController = playerManager.GetPlayerController(playerId);
		if (!playerController)
			return "";
		
		// Get the player identity (this should give us a unique identifier)
		string playerId_str = playerId.ToString();
		
		// For now, use a combination of player ID and name as unique identifier
		// In a real implementation, you'd want to get the actual Steam/platform UID
		string playerName = playerManager.GetPlayerName(playerId);
		if (playerName.IsEmpty())
			playerName = "Unknown";
		
		// Create a pseudo-UID using player ID + name hash
		// This is a fallback - ideally you'd get the real platform UID
		string pseudoUID = string.Format("ARMA_%1_%2", playerId, playerName.Hash());
		
		return pseudoUID;
	}
	
	//! Get player name from player ID
	protected string GetPlayerName(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "Unknown";
		
		return playerManager.GetPlayerName(playerId);
	}
	
	//! Check and restore custom name for a player (called when they use commands)
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
				
				// Update metadata
				entry.m_sLastPlayerName = GetPlayerName(playerId);
				entry.m_iLastUpdated = System.GetUnixTime();
				
				// Save updated metadata (server only)
				if (GetGame().InPlayMode() && Replication.IsServer())
					SaveCustomNames();
				
				// Note: RPC broadcasting will be handled by the chat component's command processing
				Print(string.Format("CustomNamesManager: Custom name '%1' is available for player %2", 
					entry.m_sCustomName, playerId));
			}
		}
	}
	
	//! Get all custom names (server only) - returns simplified map for compatibility
	map<string, string> GetAllCustomNames()
	{
		map<string, string> result = new map<string, string>();
		foreach (string uid, CustomNameEntry entry : m_CustomNames)
		{
			result[uid] = entry.m_sCustomName;
		}
		return result;
	}
	
	//! Remove a custom name
	bool RemoveCustomName(string playerId)
	{
		string playerUID = GetPlayerUID(playerId.ToInt());
		if (playerUID.IsEmpty())
			return false;
		
		CustomNameEntry entry;
		if (m_CustomNames.Find(playerUID, entry))
		{
			m_CustomNames.Remove(playerUID);
			
			// Save to file (server only)
			if (GetGame().InPlayMode() && Replication.IsServer())
				SaveCustomNames();
				
			return true;
		}
		
		return false;
	}
	
	//! Validate a custom name - simplified for milsim servers
	bool ValidateCustomName(string name)
	{
		// Check length (3-20 characters)
		if (name.Length() < 3 || name.Length() > 20)
			return false;
		
		// Check for invalid characters (basic alphanumeric + some symbols)
		for (int i = 0; i < name.Length(); i++)
		{
			string char = name.Get(i);
			if (!IsValidCharacter(char))
				return false;
		}
		
		return true;
	}
	
	//! Check if a character is valid for custom names
	protected bool IsValidCharacter(string char)
	{
		// Allow letters, numbers, spaces, hyphens, underscores, periods
		int ascii = char.ToAscii();
		
		// A-Z, a-z
		if ((ascii >= 65 && ascii <= 90) || (ascii >= 97 && ascii <= 122))
			return true;
			
		// 0-9
		if (ascii >= 48 && ascii <= 57)
			return true;
			
		// Space, hyphen, underscore, period
		if (ascii == 32 || ascii == 45 || ascii == 95 || ascii == 46)
			return true;
			
		return false;
	}
	
	//! Load custom names from file (server only)
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
			
		// Parse JSON (basic implementation)
		ParseCustomNamesJson(jsonContent);
		
		Print(string.Format("CustomNamesManager: Loaded %1 custom names from file", m_CustomNames.Count()));
	}
	
	//! Save custom names to file (server only)
	protected void SaveCustomNames()
	{
		FileHandle file = FileIO.OpenFile(m_SaveFilePath, FileMode.WRITE);
		if (!file)
		{
			Print("CustomNamesManager: Failed to open save file for writing");
			return;
		}
		
		// Create JSON content
		string jsonContent = CreateCustomNamesJson();
		file.WriteLine(jsonContent);
		file.Close();
		
		Print(string.Format("CustomNamesManager: Saved %1 custom names to file", m_CustomNames.Count()));
	}
	
	//! Create JSON string from custom names map
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
	
	//! Parse JSON string to populate custom names map
	protected void ParseCustomNamesJson(string jsonContent)
	{
		// Enhanced JSON parser for nested objects
		// This is a simplified parser for our specific structure
		
		// Remove outer braces and split by player entries
		jsonContent.Replace("{\n", "");
		jsonContent.Replace("\n}", "");
		
		// Split entries by looking for UID patterns
		array<string> lines = {};
		jsonContent.Split("\n", lines, false);
		
		string currentUID = "";
		CustomNameEntry currentEntry = null;
		
		foreach (string line : lines)
		{
			line.Trim();
			if (line.IsEmpty() || line == "," || line == "{" || line == "}")
				continue;
			
			// Check if this is a UID line (starts with quote, ends with quote: {)
			if (line.Contains("\": {"))
			{
				// Extract UID
				int startQuote = line.IndexOf("\"");
				if (startQuote >= 0)
				{
					// Find the second quote after the first one
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
			// Parse properties
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
	
	//! Extract string value from JSON line like "key": "value"
	protected string ExtractJsonStringValue(string line)
	{
		int colonPos = line.IndexOf(":");
		if (colonPos >= 0)
		{
			// Find the first quote after the colon
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
	
	//! Extract number value from JSON line like "key": 123
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

//! Global instance of the custom names manager
static ref CustomNamesManager g_CustomNamesManager;

//! Get the global custom names manager instance
static CustomNamesManager GetCustomNamesManager()
{
	if (!g_CustomNamesManager)
		g_CustomNamesManager = new CustomNamesManager();
		
	return g_CustomNamesManager;
}
