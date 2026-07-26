//! UI panel handler for the ticket counter display.
//! Shows a small widget in the top-right corner with the current ticket count.

class IRRU_TicketSystemPanel : ScriptedWidgetEventHandler
{
	protected Widget m_wRoot;
	protected TextWidget m_wTicketCount;
	protected bool m_bVisible = false;
	protected float m_fUpdateInterval = 0.5;
	protected float m_fTimeSinceUpdate = 0;

	//------------------------------------------------------------------------------------------------
	void IRRU_TicketSystemPanel(Widget root)
	{
		m_wRoot = root;

		if (!m_wRoot)
			return;

		m_wTicketCount = TextWidget.Cast(m_wRoot.FindAnyWidget("TicketCounter"));

		if (m_wTicketCount)
			m_wTicketCount.SetText("0");

		m_wRoot.AddHandler(this);
		Hide();
	}

	//------------------------------------------------------------------------------------------------
	void Update(float timeSlice)
	{
		if (!m_bVisible)
			return;

		m_fTimeSinceUpdate += timeSlice;
		if (m_fTimeSinceUpdate < m_fUpdateInterval)
			return;

		m_fTimeSinceUpdate = 0;
		UpdateTicketDisplay();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTicketDisplay()
	{
		if (!m_wTicketCount)
			return;

		IRRU_TicketSystemGameModeComponent ticketSystem = IRRU_TicketSystemGameModeComponent.GetInstance();
		if (!ticketSystem)
		{
			m_wTicketCount.SetText("--");
			return;
		}

		int tickets = ticketSystem.GetTicketCount();
		m_wTicketCount.SetText(tickets.ToString());

		// Color-code based on severity
		if (tickets >= 20)
			m_wTicketCount.SetColorInt(0xFFFF3333); // Red
		else if (tickets >= 10)
			m_wTicketCount.SetColorInt(0xFFFFAA00); // Orange
		else if (tickets >= 5)
			m_wTicketCount.SetColorInt(0xFFFFFF00); // Yellow
		else
			m_wTicketCount.SetColorInt(0xFFFFFFFF); // White
	}

	//------------------------------------------------------------------------------------------------
	void Show()
	{
		if (m_wRoot)
		{
			m_wRoot.SetVisible(true);
			m_bVisible = true;
			UpdateTicketDisplay();
		}
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		if (m_wRoot)
		{
			m_wRoot.SetVisible(false);
			m_bVisible = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsVisible()
	{
		return m_bVisible;
	}
}
