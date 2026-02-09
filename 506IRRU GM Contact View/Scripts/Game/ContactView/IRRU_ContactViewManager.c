class IRRU_ContactViewManager
{
	private static ref IRRU_ContactViewManager s_Instance;

	protected ref map<int, float> m_mPlayerLastContactTime;
	protected ref map<int, float> m_mReplicatedTimeSinceContact;

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
		m_mReplicatedTimeSinceContact = new map<int, float>();
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerFired(int playerId)
	{
		if (playerId <= 0)
			return;

		m_mPlayerLastContactTime.Set(playerId, GetCurrentWorldTime());
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerDamaged(int playerId)
	{
		if (playerId <= 0)
			return;

		m_mPlayerLastContactTime.Set(playerId, GetCurrentWorldTime());
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerJoined(int playerId)
	{
		if (playerId <= 0)
			return;

		m_mPlayerLastContactTime.Set(playerId, GetCurrentWorldTime());
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerLeft(int playerId)
	{
		m_mPlayerLastContactTime.Remove(playerId);
		m_mReplicatedTimeSinceContact.Remove(playerId);
	}

	//------------------------------------------------------------------------------------------------
	float GetLastContactTime(int playerId)
	{
		if (!m_mPlayerLastContactTime.Contains(playerId))
			return -1.0;

		return m_mPlayerLastContactTime.Get(playerId);
	}

	//------------------------------------------------------------------------------------------------
	void UpdateFromReplicatedData(array<int> playerIds, array<float> timeSinceContact)
	{
		if (!playerIds || !timeSinceContact)
			return;

		m_mReplicatedTimeSinceContact.Clear();

		int count = Math.Min(playerIds.Count(), timeSinceContact.Count());
		for (int i = 0; i < count; i++)
		{
			m_mReplicatedTimeSinceContact.Set(playerIds[i], timeSinceContact[i]);
		}
	}

	//------------------------------------------------------------------------------------------------
	float GetTimeSinceContact(int playerId)
	{
		// Clients use replicated data
		if (Replication.IsRunning() && !Replication.IsServer())
		{
			if (m_mReplicatedTimeSinceContact.Contains(playerId))
				return m_mReplicatedTimeSinceContact.Get(playerId);

			return -1.0;
		}

		// Server calculates directly
		if (!m_mPlayerLastContactTime.Contains(playerId))
			return -1.0;

		return GetCurrentWorldTime() - m_mPlayerLastContactTime.Get(playerId);
	}

	//------------------------------------------------------------------------------------------------
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
	protected float GetCurrentWorldTime()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return 0;

		return world.GetWorldTime() / 1000.0;
	}
}
