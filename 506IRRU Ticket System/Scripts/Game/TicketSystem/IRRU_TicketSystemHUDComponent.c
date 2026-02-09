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
	protected bool m_bEditorManagerFound = false;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only run on clients, not on dedicated server
		if (Replication.IsRunning() && Replication.IsServer())
			return;

		Print("[TicketSystem] HUD component OnPostInit - client side");

		SetEventMask(owner, EntityEvent.FRAME);
		TryInitEditorManager();
	}

	//------------------------------------------------------------------------------------------------
	protected void TryInitEditorManager()
	{
		if (m_bEditorManagerFound)
			return;

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return;

		m_EditorManager = core.GetEditorManager();
		if (m_EditorManager)
		{
			m_bEditorManagerFound = true;
			m_EditorManager.GetOnOpened().Insert(OnEditorOpened);
			m_EditorManager.GetOnClosed().Insert(OnEditorClosed);

			Print("[TicketSystem] Editor manager found, subscribed to events");

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
		m_bEditorManagerFound = true;

		m_EditorManager.GetOnOpened().Insert(OnEditorOpened);
		m_EditorManager.GetOnClosed().Insert(OnEditorClosed);

		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (core)
			core.Event_OnEditorManagerInitOwner.Remove(OnEditorManagerInit);

		Print("[TicketSystem] Editor manager initialized via callback");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEditorOpened()
	{
		// Only show for full GMs, not limited editor (photo mode)
		if (m_EditorManager && m_EditorManager.IsLimited())
		{
			Print("[TicketSystem] Editor opened but limited - not showing panel");
			return;
		}

		Print("[TicketSystem] Editor opened - creating/showing panel");

		if (!m_TicketPanel)
			InitializePanel();
		else
			m_TicketPanel.Show();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEditorClosed()
	{
		Print("[TicketSystem] Editor closed - hiding panel");

		if (m_TicketPanel)
			m_TicketPanel.Hide();
	}

	//------------------------------------------------------------------------------------------------
	protected void InitializePanel()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			Print("[TicketSystem] ERROR: No workspace available");
			return;
		}

		if (m_sPanelLayout.IsEmpty())
		{
			Print("[TicketSystem] ERROR: Panel layout is empty - set it on the prefab");
			return;
		}

		Print(string.Format("[TicketSystem] Creating panel from layout: %1", m_sPanelLayout));

		m_wPanelRoot = workspace.CreateWidgets(m_sPanelLayout);
		if (!m_wPanelRoot)
		{
			Print("[TicketSystem] ERROR: Failed to create widgets from layout");
			return;
		}

		m_TicketPanel = new IRRU_TicketSystemPanel(m_wPanelRoot);
		m_TicketPanel.Show();

		Print("[TicketSystem] Panel created and shown successfully");
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		// Retry editor manager init if not found yet
		if (!m_bEditorManagerFound)
			TryInitEditorManager();

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
