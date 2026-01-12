class IRRU_ContactViewHUDComponentClass : ScriptComponentClass
{
}

class IRRU_ContactViewHUDComponent : ScriptComponent
{
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

		if (Replication.IsRunning() && Replication.IsServer())
			return;

		m_bInitialized = true;

		SetEventMask(owner, EntityEvent.FRAME);

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
			m_EditorManager.GetOnOpened().Insert(OnEditorOpened);
			m_EditorManager.GetOnClosed().Insert(OnEditorClosed);

			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print("[ContactView] Subscribed to editor events");

			if (m_EditorManager.IsOpened())
				OnEditorOpened();

			return;
		}

		core.Event_OnEditorManagerInitOwner.Insert(OnEditorManagerInit);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print("[ContactView] Waiting for editor manager init...");
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
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print("[ContactView] ERROR: No workspace available");
			return;
		}

		if (m_sPanelLayout.IsEmpty())
		{
			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print("[ContactView] ERROR: Panel layout not configured");
			return;
		}

		m_wPanelRoot = workspace.CreateWidgets(m_sPanelLayout);
		if (!m_wPanelRoot)
		{
			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print(string.Format("[ContactView] ERROR: Failed to create panel from layout: %1", m_sPanelLayout));
			return;
		}

		m_ContactViewPanel = new IRRU_ContactViewPanel(m_wPanelRoot);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print("[ContactView] Panel created successfully");

		m_ContactViewPanel.Show();
	}

	//------------------------------------------------------------------------------------------------
	void ToggleContactViewPanel()
	{
		if (!m_EditorManager || !m_EditorManager.IsOpened())
			return;

		if (m_ContactViewPanel)
			m_ContactViewPanel.Toggle();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (m_ContactViewPanel)
			m_ContactViewPanel.Update(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	void ~IRRU_ContactViewHUDComponent()
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
