class IRRU_ContactViewGameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class IRRU_ContactViewGameModeComponent : SCR_BaseGameModeComponent
{
	[Attribute("30", UIWidgets.Slider, "Time in seconds until player shows as green (no contact)", "10 3600 5")]
	protected float m_fGreenThreshold;

	[Attribute("1000", UIWidgets.Slider, "How often to sync data to clients (milliseconds)", "500 5000 100")]
	protected int m_iSyncIntervalMs;

	protected bool m_bInitialized = false;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		ScheduleSync();
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStart()
	{
		if (m_bInitialized)
			return;

		m_bInitialized = true;

		IRRU_ContactViewSettings.SetGreenThreshold(m_fGreenThreshold);
		IRRU_ContactViewManager.GetInstance();
	}

	//------------------------------------------------------------------------------------------------
	protected void ScheduleSync()
	{
		GetGame().GetCallqueue().Remove(SyncContactData);
		GetGame().GetCallqueue().CallLater(SyncContactData, m_iSyncIntervalMs, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void SyncContactData()
	{
		if (!Replication.IsServer())
			return;

		IRRU_ContactViewManager manager = IRRU_ContactViewManager.GetInstance();
		if (manager)
		{
			array<int> playerIds = {};
			manager.GetTrackedPlayers(playerIds);

			if (playerIds.Count() > 0)
			{
				array<float> contactTimes = {};
				float currentTime = GetCurrentWorldTime();

				foreach (int playerId : playerIds)
				{
					float lastContactTime = manager.GetLastContactTime(playerId);
					if (lastContactTime < 0)
					{
						contactTimes.Insert(-1);
						continue;
					}

					contactTimes.Insert(currentTime - lastContactTime);
				}

				Rpc(RpcAll_ReceiveContactData, playerIds, contactTimes);
			}
		}

		ScheduleSync();
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_ReceiveContactData(array<int> playerIds, array<float> contactTimes)
	{
		IRRU_ContactViewManager manager = IRRU_ContactViewManager.GetInstance();
		if (!manager)
			return;

		manager.UpdateFromReplicatedData(playerIds, contactTimes);
	}

	//------------------------------------------------------------------------------------------------
	protected float GetCurrentWorldTime()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return 0;

		return world.GetWorldTime() / 1000.0;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerConnected(int playerId)
	{
		if (!Replication.IsServer())
			return;

		IRRU_ContactViewManager.GetInstance().OnPlayerJoined(playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		if (!Replication.IsServer())
			return;

		IRRU_ContactViewManager.GetInstance().OnPlayerLeft(playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerSpawnFinalize_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, IEntity entity)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
			IRRU_ContactViewManager.GetInstance().OnPlayerJoined(playerId);
	}
}
