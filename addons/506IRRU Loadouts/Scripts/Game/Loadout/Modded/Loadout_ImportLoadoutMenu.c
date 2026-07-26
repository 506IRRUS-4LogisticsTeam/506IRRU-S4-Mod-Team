sealed class Loadout_Import_Menu : ScriptedWidgetEventHandler
{
	protected Widget m_Root;
	protected EditBoxWidget m_LoadoutText;
	protected ButtonWidget m_ApplyButton;
	protected ButtonWidget m_CloseButton;
	protected IEntity m_PlayerEntity;

	static ref Loadout_Import_Menu s_Instance;

	static void Open(IEntity playerEntity)
	{
		if (s_Instance)
			s_Instance.Close();

		auto workspace = GetGame().GetWorkspace();
		Widget root = workspace.CreateWidgets("{CBFECEB4D015AE87}UI/Loadout_Import_Menu.layout");
		if (!root)
		{
			Print("Root not found");
			return;
		}

		Loadout_Import_Menu menu = Loadout_Import_Menu.Cast(root.FindHandler(Loadout_Import_Menu));
		if (!menu) {
			Print("Menu Not Found");
			menu = new Loadout_Import_Menu();
			root.AddHandler(menu);
		}

		menu.InitWithRoot(root, playerEntity);
		s_Instance = menu;
		
		Widget focusW = FindFirstFocusable(root);
		if (focusW)
			workspace.SetFocusedWidget(focusW, true);
	}
	
	static Widget FindFirstFocusable(Widget w)
	{
		if (!w)
			return null;
	
		if (w.IsFocusable())
			return w;
	
		Widget child = w.GetChildren();
		while (child)
		{
			Widget found = FindFirstFocusable(child);
			if (found)
				return found;
	
			child = child.GetSibling();
		}
	
		return null;
	}

	void InitWithRoot(Widget root, IEntity playerEntity)
	{
		m_Root = root;
		m_PlayerEntity = playerEntity;
			Print("Got Here");

		m_LoadoutText = EditBoxWidget.Cast(m_Root.FindAnyWidget("LoadoutText"));
		m_ApplyButton = ButtonWidget.Cast(m_Root.FindAnyWidget("ApplyButton"));
		m_CloseButton = ButtonWidget.Cast(m_Root.FindAnyWidget("CloseButton"));
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_CloseButton) {
			Close();
			return true;
		}

		if (w == m_ApplyButton) {
			ApplyLoadoutFromText();
			return true;
		}

		return false;
	}

	protected void ApplyLoadoutFromText()
	{
		if (!m_PlayerEntity || !m_LoadoutText)
			return;

		string loadoutData = m_LoadoutText.GetText();
		if (loadoutData.IsEmpty())
			return;

		if (!Replication.IsServer())
			return;

		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if (!loadContext.ImportFromString(loadoutData))
			return;

		Loadout_ApplyLoadout.ClearCharacterEquipment(m_PlayerEntity);
		SCR_PlayerArsenalLoadout.ApplyLoadoutString(m_PlayerEntity, loadContext);
		Close();
	}

	void Close()
	{
		if (m_Root)
			m_Root.RemoveFromHierarchy();

		m_Root = null;
		m_LoadoutText = null;
		m_ApplyButton = null;
		m_CloseButton = null;
		m_PlayerEntity = null;
		if (s_Instance == this)
			s_Instance = null;
	}
}