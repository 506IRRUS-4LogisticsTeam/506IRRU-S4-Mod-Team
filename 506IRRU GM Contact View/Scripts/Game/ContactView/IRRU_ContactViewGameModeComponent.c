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

	protected bool m_bInitialized = false;

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
	override void OnPlayerConnected(int playerId)
	{
		IRRU_ContactViewManager.GetInstance().OnPlayerJoined(playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
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
