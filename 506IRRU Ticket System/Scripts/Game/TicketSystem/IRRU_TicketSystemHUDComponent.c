//! Client-side HUD component that shows the ticket counter panel
//! when the Zeus/Ares editor is open.

class IRRU_TicketSystemHUDComponentClass : ScriptComponentClass
{
}

class IRRU_TicketSystemHUDComponent : ScriptComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Ticket Counter Panel Layout", "layout")]
	protected ResourceName m_sPanelLayout;

	protected ref IRRU_TicketSystemPanel m_TicketPanel;
	protected Widget m_wPanelRoot;
	protected SCR_EditorManagerEntity m_EditorManager;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only run on clients, not on dedicated server
		if (Replication.IsRunning() && Replication.IsServer())
			return;

		SetEventMask(owner, EntityEvent.FRAME);
		TryInitEditorManager();
	}

	//------------------------------------------------------------------------------------------------
	protected void TryInitEditorManager()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return;

		m_EditorManager = core.GetEditorManager();
		if (m_EditorManager)
		{
			m_EditorManager.GetOnOpened().Insert(OnEditorOpened);
			m_EditorManager.GetOnClosed().Insert(OnEditorClosed);

			if (m_EditorManager.IsOpened())
				OnEditorOpened();

			return;
		}

		core.Event_OnEditorManagerInitOwner.Insert(OnEditorManagerInit);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEditorManagerInit(SCR_EditorManagerEntity editorManager)
	{
		m_EditorManager = editorManager;

		m_EditorManager.GetOnOpened().Insert(OnEditorOpened);
		m_EditorManager.GetOnClosed().Insert(OnEditorClosed);

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (core)
			core.Event_OnEditorManagerInitOwner.Remove(OnEditorManagerInit);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEditorOpened()
	{
		if (!m_TicketPanel)
			InitializePanel();
		else
			m_TicketPanel.Show();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEditorClosed()
	{
		if (m_TicketPanel)
			m_TicketPanel.Hide();
	}

	//------------------------------------------------------------------------------------------------
	protected void InitializePanel()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		if (m_sPanelLayout.IsEmpty())
			return;

		m_wPanelRoot = workspace.CreateWidgets(m_sPanelLayout);
		if (!m_wPanelRoot)
			return;

		m_TicketPanel = new IRRU_TicketSystemPanel(m_wPanelRoot);
		m_TicketPanel.Show();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (m_TicketPanel)
			m_TicketPanel.Update(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	void ~IRRU_TicketSystemHUDComponent()
	{
		if (m_EditorManager)
		{
			m_EditorManager.GetOnOpened().Remove(OnEditorOpened);
			m_EditorManager.GetOnClosed().Remove(OnEditorClosed);
		}

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (core)
			core.Event_OnEditorManagerInitOwner.Remove(OnEditorManagerInit);

		if (m_wPanelRoot)
			m_wPanelRoot.RemoveFromHierarchy();
	}
}
