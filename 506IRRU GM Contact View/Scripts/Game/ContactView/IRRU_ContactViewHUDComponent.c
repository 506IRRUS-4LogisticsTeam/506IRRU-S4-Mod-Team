//! HUD component that manages the Contact View panel
//! Integrates with game HUD manager and editor system
//! Panel only shows when GM is in editor view

class IRRU_ContactViewHUDComponentClass : ScriptComponentClass
{
}

class IRRU_ContactViewHUDComponent : ScriptComponent
{
	// Layout resource - update this GUID after registering in Workbench
	[Attribute("{EAC917FB909C7478}UI/Layouts/ContactViewPanel2.layout", UIWidgets.ResourceNamePicker, "Contact View Panel Layout", "layout")]
	protected ResourceName m_sPanelLayout;

	protected ref IRRU_ContactViewPanel m_ContactViewPanel;
	protected Widget m_wPanelRoot;
	protected SCR_EditorManagerEntity m_EditorManager;
	protected bool m_bInitialized = false;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only initialize on client
		if (Replication.IsRunning() && Replication.IsServer())
			return;

		m_bInitialized = true;

		// Enable frame updates for panel refresh
		SetEventMask(owner, EntityEvent.FRAME);

		// Try to get editor manager now, or wait for it
		TryInitEditorManager();
	}

	//------------------------------------------------------------------------------------------------
	protected void TryInitEditorManager()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
		{
			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print("[ContactView] EditorManagerCore not found, cannot initialize");
			return;
		}

		m_EditorManager = core.GetEditorManager();
		if (m_EditorManager)
		{
			// Subscribe to editor open/close events
			m_EditorManager.GetOnOpened().Insert(OnEditorOpened);
			m_EditorManager.GetOnClosed().Insert(OnEditorClosed);

			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print("[ContactView] Subscribed to editor events");

			// If editor is already open, show panel
			if (m_EditorManager.IsOpened())
				OnEditorOpened();

			return;
		}

		// Editor manager not ready yet, subscribe to init event
		core.Event_OnEditorManagerInitOwner.Insert(OnEditorManagerInit);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print("[ContactView] Waiting for editor manager init...");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEditorManagerInit(SCR_EditorManagerEntity editorManager)
	{
		m_EditorManager = editorManager;

		// Subscribe to editor open/close events
		m_EditorManager.GetOnOpened().Insert(OnEditorOpened);
		m_EditorManager.GetOnClosed().Insert(OnEditorClosed);

		// Unsubscribe from init event
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (core)
			core.Event_OnEditorManagerInitOwner.Remove(OnEditorManagerInit);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print("[ContactView] Editor manager initialized, subscribed to events");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEditorOpened()
	{
		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print("[ContactView] Editor opened - showing panel");

		if (!m_ContactViewPanel)
			InitializeContactViewPanel();
		else
			m_ContactViewPanel.Show();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEditorClosed()
	{
		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print("[ContactView] Editor closed - hiding panel");

		if (m_ContactViewPanel)
			m_ContactViewPanel.Hide();
	}

	//------------------------------------------------------------------------------------------------
	protected void InitializeContactViewPanel()
	{
		// Get the workspace to create widgets
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print("[ContactView] ERROR: No workspace available");
			return;
		}

		// Check if layout is configured
		if (m_sPanelLayout.IsEmpty())
		{
			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print("[ContactView] ERROR: Panel layout not configured");
			return;
		}

		// Create the panel widget from layout
		m_wPanelRoot = workspace.CreateWidgets(m_sPanelLayout);
		if (!m_wPanelRoot)
		{
			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print(string.Format("[ContactView] ERROR: Failed to create panel from layout: %1", m_sPanelLayout));
			return;
		}

		// Create the panel handler
		m_ContactViewPanel = new IRRU_ContactViewPanel(m_wPanelRoot);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print("[ContactView] Panel created successfully");

		// Show the panel since we're being called from OnEditorOpened
		m_ContactViewPanel.Show();
	}

	//------------------------------------------------------------------------------------------------
	void ToggleContactViewPanel()
	{
		// Only allow toggle if editor is open
		if (!m_EditorManager || !m_EditorManager.IsOpened())
			return;

		if (m_ContactViewPanel)
			m_ContactViewPanel.Toggle();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		// Update the panel if it exists and is visible
		if (m_ContactViewPanel)
			m_ContactViewPanel.Update(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	void ~IRRU_ContactViewHUDComponent()
	{
		// Cleanup event subscriptions
		if (m_EditorManager)
		{
			m_EditorManager.GetOnOpened().Remove(OnEditorOpened);
			m_EditorManager.GetOnClosed().Remove(OnEditorClosed);
		}

		// Also cleanup the init event if still subscribed
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (core)
			core.Event_OnEditorManagerInitOwner.Remove(OnEditorManagerInit);

		// Cleanup panel widget
		if (m_wPanelRoot)
			m_wPanelRoot.RemoveFromHierarchy();
	}
}
