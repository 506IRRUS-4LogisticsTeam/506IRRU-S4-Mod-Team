[BaseContainerProps(configRoot: true)]
class IRRU_InventoryRestoreSettings
{
	static const string MOD_VERSION = "1.0.0";

	[Attribute(defvalue: "1", desc: "Enable verbose debug output to the RPT log", category: "Inventory Check", uiwidget: UIWidgets.CheckBox)]
	bool m_bDebugEnabled;

	protected static ref IRRU_InventoryRestoreSettings s_Instance;
	protected static bool s_Initialized = false;

	static IRRU_InventoryRestoreSettings GetInstance()
	{
		if (!s_Instance)
		{
			Resource holder = BaseContainerTools.LoadContainer("Configs/IRRU_InventoryRestoreSettings.conf");
			if (holder && holder.GetResource())
			{
				BaseContainer container = holder.GetResource().ToBaseContainer();
				if (container)
					s_Instance = IRRU_InventoryRestoreSettings.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
			}

			if (!s_Instance)
			{
				Print("[InventoryCheck] Config not found, using defaults", LogLevel.WARNING);
				s_Instance = new IRRU_InventoryRestoreSettings();
				s_Instance.m_bDebugEnabled = true;
			}

			if (!s_Initialized)
			{
				s_Initialized = true;
				Print(string.Format("[InventoryCheck] Mod v%1 initialized - Debug: %2",
					MOD_VERSION, s_Instance.m_bDebugEnabled));
			}
		}
		return s_Instance;
	}

	static bool IsDebugEnabled()
	{
		IRRU_InventoryRestoreSettings settings = GetInstance();
		if (settings)
			return settings.m_bDebugEnabled;
		return true;
	}

	static void SetDebugEnabled(bool enabled)
	{
		IRRU_InventoryRestoreSettings settings = GetInstance();
		if (settings)
			settings.m_bDebugEnabled = enabled;
	}
}