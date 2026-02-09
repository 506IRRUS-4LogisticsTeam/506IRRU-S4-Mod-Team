class IRRU_ContactViewGameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class IRRU_ContactViewGameModeComponent : SCR_BaseGameModeComponent
{
	[Attribute("300", UIWidgets.Slider, "Time in seconds before player shows as warning (yellow)", "60 1800 30")]
	protected float m_fWarningThreshold;

	[Attribute("600", UIWidgets.Slider, "Time in seconds before player shows as critical (red)", "120 3600 60")]
	protected float m_fCriticalThreshold;

	[Attribute("0", UIWidgets.CheckBox, "Enable debug logging")]
	protected bool m_bDebugEnabled;

	[Attribute("1", UIWidgets.Slider, "How often to sync data to clients (seconds)", "0.5 5 0.5")]
	protected float m_fSyncInterval;

	protected bool m_bInitialized = false;
	protected float m_fTimeSinceSync = 0;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStart()
	{
		if (m_bInitialized)
			return;

		m_bInitialized = true;

		IRRU_ContactViewSettings.SetWarningThreshold(m_fWarningThreshold);
		IRRU_ContactViewSettings.SetCriticalThreshold(m_fCriticalThreshold);
		IRRU_ContactViewSettings.SetDebugEnabled(m_bDebugEnabled);

		IRRU_ContactViewManager.GetInstance();

		if (m_bDebugEnabled)
			Print("[ContactView] Game mode component initialized");
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		m_fTimeSinceSync += timeSlice;
		if (m_fTimeSinceSync < m_fSyncInterval)
			return;

		m_fTimeSinceSync = 0;
		SyncContactData();
	}

	//------------------------------------------------------------------------------------------------
	protected void SyncContactData()
	{
		IRRU_ContactViewManager manager = IRRU_ContactViewManager.GetInstance();
		if (!manager)
			return;

		array<int> playerIds = {};
		manager.GetTrackedPlayers(playerIds);

		if (playerIds.Count() == 0)
			return;

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

			float timeSinceContact = currentTime - lastContactTime;
			contactTimes.Insert(timeSinceContact);
		}

		Rpc(RpcAll_ReceiveContactData, playerIds, contactTimes);

		if (m_bDebugEnabled)
			Print(string.Format("[ContactView] Synced %1 players to clients", playerIds.Count()));
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_ReceiveContactData(array<int> playerIds, array<float> contactTimes)
	{
		IRRU_ContactViewManager manager = IRRU_ContactViewManager.GetInstance();
		if (!manager)
			return;

		manager.UpdateFromReplicatedData(playerIds, contactTimes);

		if (m_bDebugEnabled)
			Print(string.Format("[ContactView] Received %1 players from server", playerIds.Count()));
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
