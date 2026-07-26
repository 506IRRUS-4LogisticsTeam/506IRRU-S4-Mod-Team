//------------------------------------------------------------------------------------------------
class IRRU_RFPropagationNetworkComponentClass : SCR_BaseGameModeComponentClass
{
};

class IRRU_RFPropagationNetworkComponent : SCR_BaseGameModeComponent
{
	protected static IRRU_RFPropagationNetworkComponent s_Instance;

	[RplProp(onRplName: "OnSettingsReceived")]
	protected bool m_bRFPropagationEnabled;

	[RplProp(onRplName: "OnSettingsReceived")]
	protected bool m_bDebugEnabled;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		if (Replication.IsServer())
		{
			IRRU_RFPropagationSettings settings = IRRU_RFPropagationSettings.GetInstance();
			m_bRFPropagationEnabled = settings.IsRFPropagationEnabled();
			m_bDebugEnabled = settings.IsDebugEnabled();

			Replication.BumpMe();

			Print(string.Format("[IRRU RFPropagation] Server settings loaded - RF: %1 | Debug: %2",
				m_bRFPropagationEnabled, m_bDebugEnabled));
		}
	}

	//------------------------------------------------------------------------------------------------
	static IRRU_RFPropagationNetworkComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSettingsReceived()
	{
		Print(string.Format("[IRRU RFPropagation] Received server settings - RF: %1 | Debug: %2",
			m_bRFPropagationEnabled, m_bDebugEnabled));
	}

	//------------------------------------------------------------------------------------------------
	static bool IsRFPropagationEnabled()
	{
		if (!s_Instance)
			return false;

		return s_Instance.m_bRFPropagationEnabled;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDebugEnabled()
	{
		if (!s_Instance)
			return false;

		return s_Instance.m_bDebugEnabled;
	}
}
