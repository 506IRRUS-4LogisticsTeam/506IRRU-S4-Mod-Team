class IRRU_ContactViewGroupData
{
	protected int m_iGroupId;
	protected string m_sGroupName;
	protected ref array<int> m_aPlayerIds;
	protected float m_fAverageTimeSinceContact;
	protected float m_fWorstTimeSinceContact;
	protected int m_iCriticalCount;
	protected int m_iWarningCount;
	protected int m_iGreenCount;
	protected bool m_bIsExpanded;

	//------------------------------------------------------------------------------------------------
	void IRRU_ContactViewGroupData()
	{
		m_aPlayerIds = new array<int>();
		m_bIsExpanded = false;
	}

	//------------------------------------------------------------------------------------------------
	void SetGroupId(int groupId)
	{
		m_iGroupId = groupId;
	}

	//------------------------------------------------------------------------------------------------
	int GetGroupId()
	{
		return m_iGroupId;
	}

	//------------------------------------------------------------------------------------------------
	void SetGroupName(string name)
	{
		m_sGroupName = name;
	}

	//------------------------------------------------------------------------------------------------
	string GetGroupName()
	{
		return m_sGroupName;
	}

	//------------------------------------------------------------------------------------------------
	void AddPlayer(int playerId)
	{
		if (m_aPlayerIds.Find(playerId) == -1)
			m_aPlayerIds.Insert(playerId);
	}

	//------------------------------------------------------------------------------------------------
	void ClearPlayers()
	{
		m_aPlayerIds.Clear();
	}

	//------------------------------------------------------------------------------------------------
	array<int> GetPlayerIds()
	{
		return m_aPlayerIds;
	}

	//------------------------------------------------------------------------------------------------
	int GetPlayerCount()
	{
		return m_aPlayerIds.Count();
	}

	//------------------------------------------------------------------------------------------------
	void SetAverageTimeSinceContact(float time)
	{
		m_fAverageTimeSinceContact = time;
	}

	//------------------------------------------------------------------------------------------------
	float GetAverageTimeSinceContact()
	{
		return m_fAverageTimeSinceContact;
	}

	//------------------------------------------------------------------------------------------------
	void SetWorstTimeSinceContact(float time)
	{
		m_fWorstTimeSinceContact = time;
	}

	//------------------------------------------------------------------------------------------------
	float GetWorstTimeSinceContact()
	{
		return m_fWorstTimeSinceContact;
	}

	//------------------------------------------------------------------------------------------------
	void SetStatusCounts(int critical, int warning, int green)
	{
		m_iCriticalCount = critical;
		m_iWarningCount = warning;
		m_iGreenCount = green;
	}

	//------------------------------------------------------------------------------------------------
	int GetCriticalCount()
	{
		return m_iCriticalCount;
	}

	//------------------------------------------------------------------------------------------------
	int GetWarningCount()
	{
		return m_iWarningCount;
	}

	//------------------------------------------------------------------------------------------------
	int GetGreenCount()
	{
		return m_iGreenCount;
	}

	//------------------------------------------------------------------------------------------------
	void SetExpanded(bool expanded)
	{
		m_bIsExpanded = expanded;
	}

	//------------------------------------------------------------------------------------------------
	bool IsExpanded()
	{
		return m_bIsExpanded;
	}

	//------------------------------------------------------------------------------------------------
	void ToggleExpanded()
	{
		m_bIsExpanded = !m_bIsExpanded;
	}

	//------------------------------------------------------------------------------------------------
	int GetGroupStatusColor()
	{
		return IRRU_ContactViewManager.GetContactStatusColor(m_fAverageTimeSinceContact);
	}

	//------------------------------------------------------------------------------------------------
	bool HasPlayersNeedingAttention()
	{
		return (m_iCriticalCount > 0 || m_iWarningCount > 0);
	}
}
