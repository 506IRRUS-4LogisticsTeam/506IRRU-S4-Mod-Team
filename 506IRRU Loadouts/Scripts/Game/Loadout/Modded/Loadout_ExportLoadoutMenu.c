sealed class Loadout_Export_Menu : ScriptedWidgetEventHandler
{
	protected Widget m_Root;
	protected EditBoxWidget m_LoadoutText;
	protected ButtonWidget m_CloseButton;
	protected IEntity m_PlayerEntity;

	static ref Loadout_Export_Menu s_Instance;

	static void Open(IEntity playerEntity)
	{
		if (s_Instance)
			s_Instance.Close();

		auto workspace = GetGame().GetWorkspace();
		Widget root = workspace.CreateWidgets("{CBFECEB4D015AE87}UI/Loadout_Export_Menu.layout");
		if (!root)
			return;

		Loadout_Export_Menu menu = Loadout_Export_Menu.Cast(root.FindHandler(Loadout_Export_Menu));
		if (!menu) {
			Print("Menu Not Found");
			menu = new Loadout_Export_Menu();
			root.AddHandler(menu);
		}

		menu.InitWithRoot(root, playerEntity);
		s_Instance = menu;
	}

	void InitWithRoot(Widget root, IEntity playerEntity)
	{
		m_Root = root;
		m_PlayerEntity = playerEntity;

		m_LoadoutText = EditBoxWidget.Cast(m_Root.FindAnyWidget("LoadoutText"));
		m_CloseButton = ButtonWidget.Cast(m_Root.FindAnyWidget("CloseButton"));

		PopulateLoadoutText();
	}

	protected void PopulateLoadoutText()
	{
		if (!m_LoadoutText || !m_PlayerEntity)
			return;

		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(m_PlayerEntity, saveContext))
			return;

		string loadoutData = saveContext.ExportToString();
		m_LoadoutText.SetText(loadoutData);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_CloseButton) {
			Close();
			return true;
		}

		return false;
	}

	void Close()
	{
		if (m_Root)
			m_Root.RemoveFromHierarchy();

		m_Root = null;
		m_LoadoutText = null;
		m_CloseButton = null;
		m_PlayerEntity = null;
		if (s_Instance == this)
			s_Instance = null;
	}
}
