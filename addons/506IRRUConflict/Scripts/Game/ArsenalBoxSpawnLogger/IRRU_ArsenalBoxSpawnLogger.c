class IRRU_ArsenalBoxSpawnLoggerClass : ScriptComponentClass {}

class IRRU_ArsenalBoxSpawnLogger : ScriptComponent
{
	//! The prefab resource name to monitor
	protected static const ResourceName TARGET_PREFAB = "{45C675ED3CFBBEC5}Prefabs/Props/Military/Compositions/US/ArsenalBox_US.et";

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;

		string playerName = FindSpawnerName(owner);
		vector pos = owner.GetOrigin();

		Print(string.Format("[ArsenalBoxSpawnLogger] ArsenalBox_US spawned by: %1 | Position: (%2, %3, %4)",
			playerName,
			pos[0].ToString(),
			pos[1].ToString(),
			pos[2].ToString()), LogLevel.NORMAL);
	}

	//! Attempt to identify the player who spawned this entity
	protected string FindSpawnerName(IEntity owner)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return "Unknown (no PlayerManager)";

		// Check all connected editor managers to find one currently in placing mode
		SCR_EditorManagerCore editorCore = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (editorCore)
		{
			array<SCR_EditorManagerEntity> managers = {};
			editorCore.GetEditorEntities(managers);

			foreach (SCR_EditorManagerEntity mgr : managers)
			{
				if (!mgr || !mgr.IsOpened())
					continue;

				int playerId = mgr.GetPlayerID();
				if (playerId > 0)
				{
					string name = pm.GetPlayerName(playerId);
					if (!name.IsEmpty())
						return name;
				}
			}
		}

		// Fallback: find the nearest player to the spawned entity
		return FindNearestPlayerName(owner, pm);
	}

	//! Fallback: find the closest player to the entity position
	protected string FindNearestPlayerName(IEntity owner, PlayerManager pm)
	{
		array<int> playerIds = {};
		pm.GetPlayers(playerIds);

		float closestDist = float.MAX;
		string closestName = "Unknown";
		vector ownerPos = owner.GetOrigin();

		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = pm.GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				continue;

			float dist = vector.Distance(ownerPos, playerEntity.GetOrigin());
			if (dist < closestDist)
			{
				closestDist = dist;
				closestName = pm.GetPlayerName(playerId);
			}
		}

		if (closestName != "Unknown")
			return string.Format("%1 (nearest player, %2m away)", closestName, closestDist.ToString(1));

		return "Unknown (no players found)";
	}
}
