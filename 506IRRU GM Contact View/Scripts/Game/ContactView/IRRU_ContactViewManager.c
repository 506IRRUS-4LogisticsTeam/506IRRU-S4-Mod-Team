//! Manager singleton that tracks player contact times for GM Contact View
//! Runs on server, replicates data to clients

class IRRU_ContactViewManager
{
	private static ref IRRU_ContactViewManager s_Instance;

	// Map of player ID to their last contact time (world time when they last had contact)
	protected ref map<int, float> m_mPlayerLastContactTime;

	// Map of player ID to contact type (for display purposes)
	protected ref map<int, IRRU_EContactType> m_mPlayerLastContactType;

	// TODO: Implement ignored groups functionality
	// Some groups (like "Ares" command group) should be excluded from tracking
	// Add: protected ref set<int> m_aIgnoredGroupIds;
	// Add: void IgnoreGroup(int groupId), void UnignoreGroup(int groupId), bool IsGroupIgnored(int groupId)

	//------------------------------------------------------------------------------------------------
	static IRRU_ContactViewManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new IRRU_ContactViewManager();
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	void IRRU_ContactViewManager()
	{
		m_mPlayerLastContactTime = new map<int, float>();
		m_mPlayerLastContactType = new map<int, IRRU_EContactType>();
	}

	//------------------------------------------------------------------------------------------------
	//! Called when a player fires their weapon
	void OnPlayerFired(int playerId)
	{
		if (playerId <= 0)
			return;

		float currentTime = GetCurrentWorldTime();
		m_mPlayerLastContactTime.Set(playerId, currentTime);
		m_mPlayerLastContactType.Set(playerId, IRRU_EContactType.FIRED);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print(string.Format("[ContactView] Player %1 fired weapon at time %2", playerId, currentTime));
	}

	//------------------------------------------------------------------------------------------------
	//! Called when a player takes damage from enemy
	void OnPlayerDamaged(int playerId)
	{
		if (playerId <= 0)
			return;

		float currentTime = GetCurrentWorldTime();
		m_mPlayerLastContactTime.Set(playerId, currentTime);
		m_mPlayerLastContactType.Set(playerId, IRRU_EContactType.DAMAGED);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print(string.Format("[ContactView] Player %1 took damage at time %2", playerId, currentTime));
	}

	//------------------------------------------------------------------------------------------------
	//! Called when player joins - initialize their contact time to current time
	void OnPlayerJoined(int playerId)
	{
		if (playerId <= 0)
			return;

		float currentTime = GetCurrentWorldTime();
		m_mPlayerLastContactTime.Set(playerId, currentTime);
		m_mPlayerLastContactType.Set(playerId, IRRU_EContactType.NONE);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print(string.Format("[ContactView] Player %1 joined, initialized contact time", playerId));
	}

	//------------------------------------------------------------------------------------------------
	//! Called when player leaves - remove from tracking
	void OnPlayerLeft(int playerId)
	{
		m_mPlayerLastContactTime.Remove(playerId);
		m_mPlayerLastContactType.Remove(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Get time since last contact in seconds
	float GetTimeSinceContact(int playerId)
	{
		if (!m_mPlayerLastContactTime.Contains(playerId))
			return -1.0; // Player not tracked

		float lastContactTime = m_mPlayerLastContactTime.Get(playerId);
		float currentTime = GetCurrentWorldTime();
		return currentTime - lastContactTime;
	}

	//------------------------------------------------------------------------------------------------
	//! Get the type of last contact
	IRRU_EContactType GetLastContactType(int playerId)
	{
		if (!m_mPlayerLastContactType.Contains(playerId))
			return IRRU_EContactType.NONE;

		return m_mPlayerLastContactType.Get(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Get all tracked player IDs
	void GetTrackedPlayers(out array<int> playerIds)
	{
		if (!playerIds)
			playerIds = {};

		for (int i = 0; i < m_mPlayerLastContactTime.Count(); i++)
		{
			playerIds.Insert(m_mPlayerLastContactTime.GetKey(i));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Get the contact status color based on time since last contact
	static int GetContactStatusColor(float timeSinceContact)
	{
		float warningThreshold = IRRU_ContactViewSettings.GetWarningThreshold();
		float criticalThreshold = IRRU_ContactViewSettings.GetCriticalThreshold();

		if (timeSinceContact < 0)
			return Color.GRAY; // Not tracked

		if (timeSinceContact < warningThreshold)
			return Color.GREEN; // Recent contact

		if (timeSinceContact < criticalThreshold)
			return Color.YELLOW; // Warning - getting bored

		return Color.RED; // Critical - no contact for too long
	}

	//------------------------------------------------------------------------------------------------
	//! Format time since contact as string
	static string FormatTimeSinceContact(float timeSinceContact)
	{
		if (timeSinceContact < 0)
			return "N/A";

		int minutes = Math.Floor(timeSinceContact / 60);
		int seconds = Math.Floor(Math.Mod(timeSinceContact, 60));

		return string.Format("%1:%2", minutes, seconds.ToString(2));
	}

	//------------------------------------------------------------------------------------------------
	protected float GetCurrentWorldTime()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return 0;

		return world.GetWorldTime() / 1000.0; // Convert from ms to seconds
	}

	//------------------------------------------------------------------------------------------------
	//! Get all groups with their aggregated contact data
	//! \param outGroupData Array to fill with group data objects
	void GetGroupContactData(out array<ref IRRU_ContactViewGroupData> outGroupData)
	{
		if (!outGroupData)
			outGroupData = new array<ref IRRU_ContactViewGroupData>();

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return;

		array<SCR_AIGroup> allGroups = {};
		groupsManager.GetAllPlayableGroups(allGroups);

		foreach (SCR_AIGroup group : allGroups)
		{
			if (!group)
				continue;

			array<int> playerIds = group.GetPlayerIDs();
			if (!playerIds || playerIds.Count() == 0)
				continue;

			// TODO: Check if group is ignored before processing
			// if (IsGroupIgnored(group.GetGroupID())) continue;

			IRRU_ContactViewGroupData groupData = new IRRU_ContactViewGroupData();
			groupData.SetGroupId(group.GetGroupID());
			groupData.SetGroupName(group.GetCustomName());

			// If no custom name, try to get callsign
			if (groupData.GetGroupName().IsEmpty())
			{
				string company, platoon, squad, character, format;
				group.GetCallsigns(company, platoon, squad, character, format);
				if (!squad.IsEmpty())
					groupData.SetGroupName(squad);
				else if (!platoon.IsEmpty())
					groupData.SetGroupName(platoon);
				else
					groupData.SetGroupName(string.Format("Group %1", group.GetGroupID()));
			}

			float totalTime = 0;
			float worstTime = 0;
			int criticalCount = 0;
			int warningCount = 0;
			int greenCount = 0;
			int validPlayerCount = 0;

			float warningThreshold = IRRU_ContactViewSettings.GetWarningThreshold();
			float criticalThreshold = IRRU_ContactViewSettings.GetCriticalThreshold();

			foreach (int playerId : playerIds)
			{
				groupData.AddPlayer(playerId);

				float timeSinceContact = GetTimeSinceContact(playerId);
				if (timeSinceContact < 0)
					continue; // Player not tracked yet

				validPlayerCount++;
				totalTime += timeSinceContact;

				if (timeSinceContact > worstTime)
					worstTime = timeSinceContact;

				if (timeSinceContact >= criticalThreshold)
					criticalCount++;
				else if (timeSinceContact >= warningThreshold)
					warningCount++;
				else
					greenCount++;
			}

			if (validPlayerCount > 0)
				groupData.SetAverageTimeSinceContact(totalTime / validPlayerCount);
			else
				groupData.SetAverageTimeSinceContact(0);

			groupData.SetWorstTimeSinceContact(worstTime);
			groupData.SetStatusCounts(criticalCount, warningCount, greenCount);

			outGroupData.Insert(groupData);
		}

		// Sort groups by average time (worst first)
		SortGroupsByTime(outGroupData);
	}

	//------------------------------------------------------------------------------------------------
	//! Sort groups by average time since contact (highest/worst first)
	protected void SortGroupsByTime(array<ref IRRU_ContactViewGroupData> groups)
	{
		if (!groups || groups.Count() < 2)
			return;

		// Simple bubble sort (good enough for small lists of groups)
		int count = groups.Count();
		for (int i = 0; i < count - 1; i++)
		{
			for (int j = 0; j < count - i - 1; j++)
			{
				if (groups[j].GetAverageTimeSinceContact() < groups[j + 1].GetAverageTimeSinceContact())
				{
					// Swap
					ref IRRU_ContactViewGroupData temp = groups[j];
					groups[j] = groups[j + 1];
					groups[j + 1] = temp;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Get total counts across all groups
	void GetTotalStatusCounts(out int criticalCount, out int warningCount, out int greenCount)
	{
		criticalCount = 0;
		warningCount = 0;
		greenCount = 0;

		float warningThreshold = IRRU_ContactViewSettings.GetWarningThreshold();
		float criticalThreshold = IRRU_ContactViewSettings.GetCriticalThreshold();

		for (int i = 0; i < m_mPlayerLastContactTime.Count(); i++)
		{
			int playerId = m_mPlayerLastContactTime.GetKey(i);
			float timeSinceContact = GetTimeSinceContact(playerId);

			if (timeSinceContact < 0)
				continue;

			if (timeSinceContact >= criticalThreshold)
				criticalCount++;
			else if (timeSinceContact >= warningThreshold)
				warningCount++;
			else
				greenCount++;
		}
	}
}

//------------------------------------------------------------------------------------------------
enum IRRU_EContactType
{
	NONE,
	FIRED,
	DAMAGED
}
