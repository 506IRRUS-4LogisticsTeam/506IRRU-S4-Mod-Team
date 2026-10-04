enum IRRU_EKitOperation
{
	NONE,
	LIST,
	LOAD,
	APPLY,
	SAVE,
	RENAME,
	DELETE
}

class IRRU_LoadoutManagerSystemEvents : ScriptedWidgetEventHandler
{
	IRRU_LoadoutManagerSystemMenu m_Menu;

	override bool OnClick(Widget w, int x, int y, int button)
	{
		Widget widget = w;
		while (widget && !ButtonWidget.Cast(widget))
			widget = widget.GetParent();
		if (!m_Menu || !widget || !widget.IsEnabled())
			return false;
		m_Menu.HandleKitClick(widget);
		return true;
	}

	override bool OnChange(Widget w, bool finished)
	{
		if (m_Menu)
			m_Menu.CancelKitConfirmation();
		return false;
	}
}

class IRRU_LoadoutManagerSystemMenu
{
	protected static const ResourceName MENU_LAYOUT = "{506C020000000001}UI/layouts/Menus/LoadoutManagerSystem/IRRU_LoadoutManagerSystem.layout";
	protected static const ResourceName ROW_LAYOUT = "{506C020000000002}UI/layouts/Menus/LoadoutManagerSystem/IRRU_LoadoutManagerSystemRow.layout";
	protected static const int OPERATION_TIMEOUT_MS = 20000;
	protected static const float MAX_INTERACTION_DISTANCE = 5;
	static ref IRRU_LoadoutManagerSystemMenu s_Instance;

	protected Widget m_Root;
	protected Widget m_KitList;
	protected TextWidget m_Status;
	protected TextWidget m_Count;
	protected TextWidget m_Access;
	protected EditBoxWidget m_Name;
	protected SCR_PlayerLoadoutComponent m_Network;
	protected ref IRRU_LoadoutManagerSystemEvents m_Events;
	protected IEntity m_Arsenal;
	protected IEntity m_Character;
	protected ref array<string> m_Names = {};
	protected ref array<Widget> m_Rows = {};
	protected bool m_Shared;
	protected bool m_CanManageShared;
	protected bool m_SharedListReady;
	protected IRRU_EKitOperation m_Operation;
	protected string m_PendingName;
	protected string m_Confirmation;
	protected string m_SelectedName;
	protected bool m_PendingOverwrite;
	protected bool m_ModalRegistered;

	static bool IsOpen()
	{
		return s_Instance && s_Instance.m_Root && s_Instance.m_ModalRegistered;
	}

	static void Open(IEntity arsenal, IEntity character)
	{
		if (s_Instance)
			s_Instance.CloseLoadoutManagerSystem();

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			Print("[IRRU Loadout Manager] UI workspace is unavailable.", LogLevel.ERROR);
			return;
		}

		Widget root = workspace.CreateWidgets(MENU_LAYOUT);
		if (!root)
		{
			Print("[IRRU Loadout Manager] Could not create the manager layout.", LogLevel.ERROR);
			return;
		}

		s_Instance = new IRRU_LoadoutManagerSystemMenu();
		s_Instance.Initialize(root, arsenal, character);
	}

	protected void Initialize(Widget root, IEntity arsenal, IEntity character)
	{
		m_Root = root;
		m_Arsenal = arsenal;
		m_Character = character;
		ValidateArsenal();
		if (!m_Root)
			return;
		m_KitList = m_Root.FindAnyWidget("KitRows");
		m_Status = TextWidget.Cast(m_Root.FindAnyWidget("Status"));
		m_Count = TextWidget.Cast(m_Root.FindAnyWidget("Count"));
		m_Access = TextWidget.Cast(m_Root.FindAnyWidget("Access"));
		m_Name = EditBoxWidget.Cast(m_Root.FindAnyWidget("KitName"));
		m_Events = new IRRU_LoadoutManagerSystemEvents();
		m_Events.m_Menu = this;
		m_Root.AddHandler(m_Events);

		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController)
			m_Network = SCR_PlayerLoadoutComponent.Cast(playerController.FindComponent(SCR_PlayerLoadoutComponent));
		if (!m_KitList || !m_Name || !m_Status || !m_Count || !m_Access)
		{
			Print("[IRRU Kits] Required UI widgets or player component are missing.", LogLevel.ERROR);
			CloseLoadoutManagerSystem();
			return;
		}

		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, OnKitBack);
		if (m_Network)
		{
			m_Network.m_IRRU_OnKitPermissions.Insert(OnKitPermissions);
			m_Network.m_IRRU_OnSharedKitList.Insert(OnSharedKitList);
			m_Network.m_IRRU_OnKitApplyResult.Insert(OnKitApplyResult);
			m_Network.m_IRRU_OnSharedKitWriteResult.Insert(OnSharedKitWriteResult);
			m_Network.m_IRRU_OnPersonalKitCapture.Insert(OnPersonalKitCapture);
			m_Network.IRRU_RequestKitPermissions();
		}
		RefreshPersonalKits();
		if (m_Network)
			SetKitStatus("Select a kit to load, or enter a name to save your current equipment.");
		else
			KitFailure("Player loadout networking is unavailable. The menu opened read-only.");
		GetGame().GetWorkspace().AddModal(m_Root, m_Root.FindAnyWidget("Personal"));
		m_ModalRegistered = true;
		ActivateMenuInput();
		GetGame().GetCallqueue().CallLater(ActivateMenuInput, 0, true);
		GetGame().GetCallqueue().CallLater(CheckKitInteraction, 250, true);
	}

	protected void ActivateMenuInput()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!m_Root || !m_ModalRegistered || workspace.GetModal() != m_Root)
			return;

		// Input contexts expire each frame; modal widgets alone do not suppress gameplay.
		InputManager inputManager = GetGame().GetInputManager();
		inputManager.ActivateContext("MenuContext");
		inputManager.ActivateContext("InteractableDialogContext");
		if (workspace.GetFocusedWidget() == m_Name)
			inputManager.ActivateContext("MenuTextEditContext");
	}

	protected void ValidateArsenal()
	{
		if (!m_Arsenal)
			return;
		SCR_ArsenalComponent arsenalComponent = SCR_ArsenalComponent.FindArsenalComponent(m_Arsenal);
		if (!arsenalComponent)
		{
			KitFailure("The selected box is not an Arsenal.");
			CloseLoadoutManagerSystem();
			return;
		}
	}

	protected void Cleanup()
	{
		GetGame().GetCallqueue().Remove(ActivateMenuInput);
		GetGame().GetCallqueue().Remove(CheckKitInteraction);
		GetGame().GetCallqueue().Remove(OnKitTimeout);
		GetGame().GetCallqueue().Remove(CloseLoadoutManagerSystem);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, OnKitBack);
		if (m_Network)
		{
			m_Network.m_IRRU_OnKitPermissions.Remove(OnKitPermissions);
			m_Network.m_IRRU_OnSharedKitList.Remove(OnSharedKitList);
			m_Network.m_IRRU_OnKitApplyResult.Remove(OnKitApplyResult);
			m_Network.m_IRRU_OnSharedKitWriteResult.Remove(OnSharedKitWriteResult);
			m_Network.m_IRRU_OnPersonalKitCapture.Remove(OnPersonalKitCapture);
		}
		if (m_Events)
			m_Events.m_Menu = null;
		if (m_Root && m_Events)
			m_Root.RemoveHandler(m_Events);
		if (m_Root && m_ModalRegistered)
		{
			GetGame().GetWorkspace().RemoveModal(m_Root);
			m_ModalRegistered = false;
		}
		if (m_Root)
			m_Root.RemoveFromHierarchy();
		m_Root = null;
		m_Network = null;
		if (s_Instance == this)
			s_Instance = null;
	}

	protected void OnKitBack()
	{
		CloseLoadoutManagerSystem();
	}

	protected void CloseLoadoutManagerSystem()
	{
		Cleanup();
	}

	protected void CheckKitInteraction()
	{
		if (!m_Arsenal || !m_Character || SCR_PlayerController.GetLocalControlledEntity() != m_Character)
		{
			CloseLoadoutManagerSystem();
			return;
		}
		if (vector.Distance(m_Arsenal.GetOrigin(), m_Character.GetOrigin()) > MAX_INTERACTION_DISTANCE)
			CloseLoadoutManagerSystem();
	}

	void HandleKitClick(Widget button)
	{
		string action = button.GetName();
		if (action == "Close")
		{
			CloseLoadoutManagerSystem();
			return;
		}
		if (m_Operation != IRRU_EKitOperation.NONE || !m_Network)
			return;

		if (action == "Personal" || action == "Shared")
		{
			m_Shared = action == "Shared";
			m_SelectedName = "";
			m_Name.SetText("");
			CancelKitConfirmation();
			m_Names.Clear();
			RebuildKitRows();
			if (m_Shared)
				RefreshSharedKits();
			else
				RefreshPersonalKits();
			return;
		}

		if (action == "Refresh")
		{
			CancelKitConfirmation();
			m_Network.IRRU_RequestKitPermissions();
			if (m_Shared)
				RefreshSharedKits();
			else
				RefreshPersonalKits();
			return;
		}

		int rowIndex = m_Rows.Find(button);
		if (rowIndex >= 0 && rowIndex < m_Names.Count())
		{
			m_SelectedName = m_Names[rowIndex];
			m_Name.SetText(m_SelectedName);
			CancelKitConfirmation();
			UpdateKitControls();
			SetKitStatus(string.Format("Selected '%1'. Loading replaces your current Arsenal loadout.", m_SelectedName));
			return;
		}

		if (action == "Load")
			LoadSelectedKit();
		else if (!m_Shared || m_CanManageShared)
			ManageKit(action);
	}

	void CancelKitConfirmation()
	{
		m_Confirmation = "";
	}

	protected bool ConfirmKitAction(string key, string message)
	{
		if (m_Confirmation == key)
		{
			CancelKitConfirmation();
			return true;
		}
		m_Confirmation = key;
		SetKitStatus(message + " Click the same button again to confirm.");
		return false;
	}

	protected void SetKitStatus(string message)
	{
		if (m_Status)
			m_Status.SetText(message);
	}

	protected void KitFailure(string message)
	{
		Print("[IRRU Kits] " + message, LogLevel.WARNING);
		SetKitStatus(message);
	}

	protected void BeginOperation(IRRU_EKitOperation operation, string name = "")
	{
		m_Operation = operation;
		m_PendingName = name;
		GetGame().GetCallqueue().Remove(OnKitTimeout);
		GetGame().GetCallqueue().CallLater(OnKitTimeout, OPERATION_TIMEOUT_MS, false);
		UpdateKitControls();
	}

	protected void EndOperation()
	{
		m_Operation = IRRU_EKitOperation.NONE;
		GetGame().GetCallqueue().Remove(OnKitTimeout);
		UpdateKitControls();
	}

	protected void OnKitTimeout()
	{
		m_SharedListReady = false;
		EndOperation();
		KitFailure("No server response. The result is unknown; refresh the list before retrying.");
	}

	protected void OnKitPermissions(bool canManage)
	{
		m_CanManageShared = canManage;
		UpdateKitControls();
	}

	protected void UpdateKitControls()
	{
		if (!m_Root)
			return;
		bool idle = m_Network && m_Operation == IRRU_EKitOperation.NONE;
		bool selected = !m_SelectedName.IsEmpty() && m_Names.Find(m_SelectedName) >= 0;
		bool writable = !m_Shared || m_CanManageShared;
		array<string> navigationButtons = {"Personal", "Shared", "Refresh"};
		foreach (string name : navigationButtons)
			SetKitButtonEnabled(name, idle);
		SetKitButtonEnabled("Load", idle && selected && (!m_Shared || m_SharedListReady));
		SetKitButtonEnabled("Save", idle && writable && m_Character);
		array<string> managementButtons = {"Overwrite", "Rename", "Delete"};
		bool externalKit = IRRU_LoadoutManagerSystem.IsGRSKit(m_SelectedName);
		foreach (string managementButton : managementButtons)
			SetKitButtonEnabled(managementButton, idle && writable && selected && !externalKit && (!m_Shared || m_SharedListReady));

		for (int i = 0; i < m_Rows.Count(); i++)
		{
			Widget row = m_Rows[i];
			row.SetEnabled(idle);
			if (i < m_Names.Count() && m_Names[i] == m_SelectedName)
				row.SetColor(Color.FromRGBA(79, 99, 74, 255));
			else
				row.SetColor(Color.FromRGBA(31, 46, 36, 255));
		}

		if (m_Name)
			m_Name.SetEnabled(idle && writable);
		if (m_Access)
		{
			if (!m_Shared)
				m_Access.SetText("PERSONAL | Local profile; GRS entries are read-only");
			else if (m_CanManageShared)
				m_Access.SetText("SHARED | Admin / unrestricted GM management; GRS entries read-only");
			else
				m_Access.SetText("SHARED | Everyone can load; admins / unrestricted GMs can edit");
		}
	}

	protected void SetKitButtonEnabled(string name, bool enabled)
	{
		Widget button = m_Root.FindAnyWidget(name);
		if (button)
		{
			button.SetEnabled(enabled);
			if (enabled)
				button.SetOpacity(1);
			else
				button.SetOpacity(0.4);
		}
	}

	protected void RefreshPersonalKits()
	{
		string error;
		if (!IRRU_LoadoutManagerSystem.ListKits(false, m_Names, error))
		{
			KitFailure(string.Format("Could not read personal kits [%1].", error));
			return;
		}
		RebuildKitRows();
	}

	protected void RefreshSharedKits()
	{
		if (!m_Network || !m_Shared)
			return;
		m_SharedListReady = false;
		BeginOperation(IRRU_EKitOperation.LIST);
		m_Network.IRRU_RequestSharedKitList();
	}

	protected void OnSharedKitList(bool ok, array<string> names, string error)
	{
		if (m_Operation != IRRU_EKitOperation.LIST)
			return;
		if (!ok || !names)
		{
			EndOperation();
			KitFailure(string.Format("Shared kit list could not be retrieved [%1].", error));
			return;
		}
		m_Names = names;
		m_SharedListReady = true;
		EndOperation();
		RebuildKitRows();
		SetKitStatus("Shared kits refreshed. Select a kit to load or manage.");
	}

	protected void RebuildKitRows()
	{
		foreach (Widget row : m_Rows)
			row.RemoveFromHierarchy();
		m_Rows.Clear();
		m_Names.Sort();
		if (m_Names.Find(m_SelectedName) < 0)
			m_SelectedName = "";
		foreach (string kitName : m_Names)
		{
			Widget row = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_KitList);
			if (!row)
			{
				KitFailure("Could not create a kit list row.");
				break;
			}
			TextWidget label = TextWidget.Cast(row.FindAnyWidget("RowName"));
			if (label)
				label.SetText(kitName);
			m_Rows.Insert(row);
		}
		m_Count.SetText(string.Format("%1 saved kits", m_Names.Count()));
		UpdateKitControls();
	}

	protected bool ReadKitName(out string name)
	{
		name = m_Name.GetText();
		if (!IRRU_LoadoutManagerSystem.IsValidName(name))
		{
			KitFailure("Enter a kit name (1-48 characters), without path separators or reserved filename characters.");
			return false;
		}
		return true;
	}

	protected bool KitNameExists(string name)
	{
		string candidate = name;
		candidate.ToLower();
		foreach (string existingName : m_Names)
		{
			string existing = existingName;
			existing.ToLower();
			if (candidate == existing)
				return true;
		}
		return false;
	}

	protected void ManageKit(string action)
	{
		if (action != "Save" && IRRU_LoadoutManagerSystem.IsGRSKit(m_SelectedName))
		{
			KitFailure("GRS kits are read-only here. Load one, then save it under a new personal kit name.");
			return;
		}
		string name;
		string error;
		bool overwrite = action == "Overwrite";
		if (action == "Save" || action == "Rename")
		{
			if (!ReadKitName(name))
				return;
			if (action == "Rename" && name == m_SelectedName)
			{
				SetKitStatus("The kit name is unchanged.");
				return;
			}
			if (KitNameExists(name))
			{
				KitFailure("That name already exists. Select its kit and use Overwrite.");
				return;
			}
		}
		else
			name = m_SelectedName;

		if (action != "Save" && m_SelectedName.IsEmpty())
		{
			KitFailure("Select a kit first.");
			return;
		}
		if (action == "Delete" && !ConfirmKitAction("Delete:" + name, string.Format("Delete '%1' permanently?", name)))
			return;
		if (action == "Overwrite" && !ConfirmKitAction("Overwrite:" + name, string.Format("Replace '%1' with your current equipment?", name)))
			return;

		if (action == "Save" || overwrite)
		{
			if (m_Shared)
			{
				BeginOperation(IRRU_EKitOperation.SAVE, name);
				m_Network.IRRU_RequestSaveSharedKit(name, overwrite);
				SetKitStatus(string.Format("Saving shared kit '%1'...", name));
				return;
			}

			m_PendingOverwrite = overwrite;
			BeginOperation(IRRU_EKitOperation.SAVE, name);
			m_Network.IRRU_RequestPersonalKitCapture(name);
			SetKitStatus(string.Format("Capturing personal kit '%1'...", name));
			return;
		}

		if (action == "Rename")
		{
			if (m_Shared)
			{
				BeginOperation(IRRU_EKitOperation.RENAME, name);
				m_Network.IRRU_RequestRenameSharedKit(m_SelectedName, name);
				SetKitStatus("Renaming shared kit...");
				return;
			}
			if (!IRRU_LoadoutManagerSystem.RenameKit(false, m_SelectedName, name, error))
			{
				KitFailure(string.Format("Rename failed [%1].", error));
				return;
			}
			m_SelectedName = name;
			CancelKitConfirmation();
			RefreshPersonalKits();
			SetKitStatus("Kit renamed.");
			return;
		}

		if (action == "Delete")
		{
			if (m_Shared)
			{
				BeginOperation(IRRU_EKitOperation.DELETE, name);
				m_Network.IRRU_RequestDeleteSharedKit(name);
				SetKitStatus("Deleting shared kit...");
				return;
			}
			if (!IRRU_LoadoutManagerSystem.DeleteKit(false, name, error))
			{
				KitFailure(string.Format("Delete failed [%1].", error));
				return;
			}
			m_SelectedName = "";
			CancelKitConfirmation();
			RefreshPersonalKits();
			SetKitStatus("Kit deleted.");
		}
	}

	protected void LoadSelectedKit()
	{
		if (m_SelectedName.IsEmpty())
			return;
		if (!ConfirmKitAction("Load:" + m_SelectedName, "Loading replaces your current Arsenal loadout."))
			return;

		if (m_Shared)
		{
			BeginOperation(IRRU_EKitOperation.LOAD, m_SelectedName);
			m_Network.IRRU_RequestLoadSharedKit(m_SelectedName);
			SetKitStatus(string.Format("Loading shared kit '%1'...", m_SelectedName));
			return;
		}

		BeginOperation(IRRU_EKitOperation.APPLY, m_SelectedName);
		string loadout;
		string error;
		if (!IRRU_LoadoutManagerSystem.LoadKit(false, m_SelectedName, loadout, error))
		{
			EndOperation();
			KitFailure(string.Format("Load failed [%1].", error));
			return;
		}
		m_Network.IRRU_RequestApplyLoadout(m_SelectedName, loadout);
		SetKitStatus(string.Format("Loading '%1'...", m_SelectedName));
	}

	protected void OnKitApplyResult(bool ok, string name, string error)
	{
		if ((m_Operation != IRRU_EKitOperation.LOAD && m_Operation != IRRU_EKitOperation.APPLY) || name != m_PendingName)
			return;
		EndOperation();
		if (!ok)
			KitFailure(string.Format("Load failed [%1].", error));
		else
			SetKitStatus(string.Format("Loaded Arsenal kit '%1'.", name));
	}

	protected void OnPersonalKitCapture(bool ok, string name, string loadout, string error)
	{
		if (m_Operation != IRRU_EKitOperation.SAVE || m_Shared || name != m_PendingName)
			return;
		if (!ok)
		{
			EndOperation();
			KitFailure(string.Format("Could not capture your Arsenal loadout [%1].", error));
			return;
		}

		string saveError;
		if (!IRRU_LoadoutManagerSystem.SaveKit(false, name, loadout, m_PendingOverwrite, saveError))
		{
			EndOperation();
			KitFailure(string.Format("Save failed [%1].", saveError));
			return;
		}
		EndOperation();
		m_SelectedName = name;
		CancelKitConfirmation();
		RefreshPersonalKits();
		SetKitStatus(string.Format("Saved personal kit '%1'.", name));
	}

	protected void OnSharedKitWriteResult(string operation, bool ok, string name, string error)
	{
		IRRU_EKitOperation expectedOperation = IRRU_EKitOperation.NONE;
		if (operation == "SAVE")
			expectedOperation = IRRU_EKitOperation.SAVE;
		else if (operation == "RENAME")
			expectedOperation = IRRU_EKitOperation.RENAME;
		else if (operation == "DELETE")
			expectedOperation = IRRU_EKitOperation.DELETE;
		if (m_Operation != expectedOperation || name != m_PendingName)
			return;

		EndOperation();
		if (!ok)
		{
			KitFailure(string.Format("Shared operation failed [%1].", error));
			m_Network.IRRU_RequestKitPermissions();
			return;
		}
		if (operation == "DELETE")
			m_SelectedName = "";
		else
			m_SelectedName = name;
		CancelKitConfirmation();
		SetKitStatus("Shared change saved. Refreshing...");
		RefreshSharedKits();
	}
}
