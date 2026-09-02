//------------------------------------------------------------------------------------------------
// IRRU_PredefinedGroupNames.c
//
// Preserves predefined SCR_GroupPreset names when vanilla changes group leaders.
//
// Vanilla SCR_AIGroup.SetGroupLeader() clears a custom group name when the
// previous name author does not match the newly assigned leader:
//
//     if (GetNameAuthorID() != playerID)
//         SetCustomName("", 0);
//
// Predefined groups have their name authored by the scenario rather than the
// joining player, so the first leader assignment clears the configured name.
//
// This script remembers predefined group names and restores them whenever
// vanilla announces a group leader change.
//------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------
// Registry
//------------------------------------------------------------------------------------------------
class IRRU_PredefinedGroupNameRegistry
{
	protected static ref map<int, string> s_mGroupNames = new map<int, string>();

	//--------------------------------------------------------------------------------------------
	static void RegisterGroup(int groupID, string groupName)
	{
		if (groupID < 0)
			return;

		if (groupName.IsEmpty())
			return;

		s_mGroupNames.Set(groupID, groupName);

		PrintFormat(
			"[IRRU Group Names] Registered predefined group ID %1 as '%2'",
			groupID,
			groupName
		);
	}

	//--------------------------------------------------------------------------------------------
	static bool GetGroupName(int groupID, out string groupName)
	{
		groupName = string.Empty;

		if (groupID < 0)
			return false;

		return s_mGroupNames.Find(groupID, groupName);
	}
}


//------------------------------------------------------------------------------------------------
// Capture predefined group names.
//------------------------------------------------------------------------------------------------
modded class SCR_GroupPreset
{
	//--------------------------------------------------------------------------------------------
	override void SetupGroup(SCR_AIGroup group)
	{
		super.SetupGroup(group);

		if (!group)
			return;

		string presetName = GetGroupName();

		if (presetName.IsEmpty())
			return;

		int groupID = group.GetGroupID();

		IRRU_PredefinedGroupNameRegistry.RegisterGroup(
			groupID,
			presetName
		);

		group.SetCustomName(
			presetName,
			-1
		);

		PrintFormat(
			"[IRRU Group Names] Applied initial predefined group name '%1' to group ID %2",
			presetName,
			groupID
		);
	}
}


//------------------------------------------------------------------------------------------------
// Listen for vanilla leader changes.
//------------------------------------------------------------------------------------------------
modded class SCR_AIGroup
{
	//--------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		s_OnPlayerLeaderChanged.Insert(IRRU_OnPlayerLeaderChanged);
	}

	//--------------------------------------------------------------------------------------------
	protected void IRRU_OnPlayerLeaderChanged(int groupID, int playerID)
	{
		// This callback is global, so only act when the changed group is this group.
		if (groupID != GetGroupID())
			return;

		string predefinedName;

		if (!IRRU_PredefinedGroupNameRegistry.GetGroupName(
			groupID,
			predefinedName
		))
			return;

		if (predefinedName.IsEmpty())
			return;

		PrintFormat(
			"[IRRU Group Names] Leader changed in predefined group ID %1 to player %2. Current custom name: '%3'",
			groupID,
			playerID,
			GetCustomName()
		);

		SetCustomName(
			predefinedName,
			playerID
		);

		PrintFormat(
			"[IRRU Group Names] Restored predefined group ID %1 name to '%2'",
			groupID,
			predefinedName
		);
	}
}