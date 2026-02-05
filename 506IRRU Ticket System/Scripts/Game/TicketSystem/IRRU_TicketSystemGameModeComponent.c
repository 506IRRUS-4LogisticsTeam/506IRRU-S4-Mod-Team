//! Server-side game mode component that tracks the ticket count
//! and replicates it to all clients for HUD display.

class IRRU_TicketSystemGameModeComponentClass : SCR_BaseGameModeComponentClass
{
}

class IRRU_TicketSystemGameModeComponent : SCR_BaseGameModeComponent
{
	[Attribute("0", UIWidgets.CheckBox, "Enable debug logging")]
	protected bool m_bDebugEnabled;

	[RplProp(onRplName: "OnTicketCountChanged")]
	protected int m_iTicketCount = 0;

	protected static IRRU_TicketSystemGameModeComponent s_Instance;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
	}

	//------------------------------------------------------------------------------------------------
	static IRRU_TicketSystemGameModeComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Add tickets (server-only). Called by damage trackers when players go unconscious or die.
	//! \param amount Number of tickets to add
	//! \param reason Debug reason string
	void AddTickets(int amount, string reason = "")
	{
		if (!Replication.IsServer())
			return;

		m_iTicketCount = m_iTicketCount + amount;

		Replication.BumpMe();

		if (m_bDebugEnabled)
			Print(string.Format("[TicketSystem] +%1 tickets (%2) - Total: %3", amount, reason, m_iTicketCount));
	}

	//------------------------------------------------------------------------------------------------
	int GetTicketCount()
	{
		return m_iTicketCount;
	}

	//------------------------------------------------------------------------------------------------
	//! Set tickets to a specific value (server-only, admin command)
	//! \param value New ticket count
	//! \param reason Debug reason string
	void SetTickets(int value, string reason = "")
	{
		if (!Replication.IsServer())
			return;

		m_iTicketCount = value;

		Replication.BumpMe();

		if (m_bDebugEnabled)
			Print(string.Format("[TicketSystem] Tickets set to %1 (%2)", value, reason));
	}

	//------------------------------------------------------------------------------------------------
	//! Reset tickets to zero (server-only, admin command)
	//! \param reason Debug reason string
	void ResetTickets(string reason = "")
	{
		SetTickets(0, reason);
	}

	//------------------------------------------------------------------------------------------------
	bool IsDebugEnabled()
	{
		return m_bDebugEnabled;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTicketCountChanged()
	{
		if (m_bDebugEnabled)
			Print(string.Format("[TicketSystem] Ticket count updated: %1", m_iTicketCount));
	}

	//------------------------------------------------------------------------------------------------
	void ~IRRU_TicketSystemGameModeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}
}
