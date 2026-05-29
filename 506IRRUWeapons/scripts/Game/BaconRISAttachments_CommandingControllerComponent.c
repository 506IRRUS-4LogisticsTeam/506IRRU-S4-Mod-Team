[EntityEditorProps(category: "GameScripted/Commanding", description: "This component should be attached to player controller and is used by commanding to send requests to server.")]
class BaconRISAttachments_CommandingControllerComponentClass : SCR_PlayerControllerCommandingComponentClass
{
};

//------------------------------------------------------------------------------------------------
class BaconRISAttachments_CommandingControllerComponent : SCR_PlayerControllerCommandingComponent
{
	ref array<ref BaconRISAttachments_AttachmentToggleAction> m_validAttachments = {};

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	override void RPC_RequestExecuteCommand(int commandIndex, RplId cursorTargetID, RplId groupRplID, vector targetPoisition, int playerID)
	{
		return;
	}

	// ---- finding actions
	bool SetupCurrentWeapon()
	{
		ChimeraCharacter playerCharacter = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!playerCharacter)
			return false;

		BaseWeaponComponent weapon = SCR_WeaponLib.GetCurrentWeaponComponent(playerCharacter);
		if (!weapon)
			return false;

		IEntity gun = weapon.GetOwner();

		BaseInventoryStorageComponent comp = BaseInventoryStorageComponent.Cast(gun.FindComponent(BaseInventoryStorageComponent));

		array<BaseInventoryStorageComponent> storages = {};
		comp.GetOwnedStorages(storages, 5, true);
		foreach (BaseInventoryStorageComponent storage : storages) {
			Print(string.Format("->>>> %1", storage), LogLevel.DEBUG);
		}

		array<AttachmentSlotComponent> slots = {};

		int numAttachments = weapon.GetAttachments(slots);
		if (numAttachments < 1)
			return false;

		m_validAttachments.Clear();

		foreach (AttachmentSlotComponent attachment : slots)
		{
			IEntity attachedEntity = attachment.GetAttachedEntity();
			if (!attachedEntity)
				continue;

			BaconRISAttachments_AttachmentToggleAction maybeAction = GetToggleAction(attachedEntity);
			if (!maybeAction)
				continue;

			m_validAttachments.Insert(maybeAction);
		};

		if (m_validAttachments.Count() > 0)
			return true;

		return false;
	};

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);

		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		m_CommandingManager = SCR_CommandingManagerComponent.GetInstance();


		// BaseContainer container = holder.GetResource().ToBaseContainer();
		// m_CommandingMenuPairsConfig = SCR_PlayerCommandingMenuActionsSetup.Cast(BaseContainerTools.CreateInstanceFromContainer(container));

		if (!m_RadialMenuController)
			return;

		m_RadialMenuController.GetOnTakeControl().Insert(OnControllerTakeControl);
		m_RadialMenuController.GetOnControllerChanged().Insert(OnControllerLostControl);

		InputManager input = GetGame().GetInputManager();
		input.AddActionListener("BaconRISLaser_OpenCommandMenu", EActionTrigger.DOWN, OnPlayerRadialMenuOpen);


//		foreach (SCR_PlayerCommandingConfigActionPair actionConfigPair : m_CommandingMenuPairsConfig.m_aActionConfigPairs)
//		{
//			if (!actionConfigPair)
//				continue;
//
//			input.AddActionListener(actionConfigPair.GetActionName(), EActionTrigger.DOWN, OpenCommandingMenu);
//		}

		SCR_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
		SCR_MapEntity.GetOnMapClose().Insert(OnMapClose);

		m_PhysicsHelper.InitPhysicsHelper();
	}

	BaconRISAttachments_AttachmentToggleAction GetToggleAction(IEntity entity)
	{
		if (!entity)
			return null;

		ActionsManagerComponent actionsManagerComponent = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent));
		if (!actionsManagerComponent)
			return null;

		// proto external int GetActionsList(out notnull array<BaseUserAction> outActions);
		array<BaseUserAction> actions = {};
		int actionsNum = actionsManagerComponent.GetActionsList(actions);

		foreach (BaseUserAction action : actions)
		{
			BaconRISAttachments_AttachmentToggleAction maybeAction = BaconRISAttachments_AttachmentToggleAction.Cast(action);
			if (!maybeAction)
				continue;

			return maybeAction;
		};

		return null;
	};

	// -------- opening and updating

	override void OnPlayerRadialMenuOpen()
	{
		Print("BaconRISAttachments_CommandingControllerComponent.OnPlayerRadialMenuOpen | Opening...", LogLevel.DEBUG);

		// if (!m_RadialMenu || !m_CommandingMenuConfig)
		if (!m_RadialMenu)
			return;

		m_RadialMenu.ClearEntries();

		if (!SetupCurrentWeapon())
			return;

		if (m_validAttachments.Count() < 1)
			return;

		PlayerCamera camera = PlayerCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (!camera)
			return;

		m_SelectedEntity = camera.GetCursorTarget();

		AddEntries();

		/*
		SCR_PlayerCommandingMenuCategoryElement rootCategory = m_CommandingMenuConfig.GetRootCategory();
		if (!rootCategory)
			return;

		AddElementsFromCategory(rootCategory);
		*/
	}

	override void UpdateRadialMenu(IEntity owner, bool isOpen)
	{
		if (!m_RadialMenu || !m_CommandingMenuConfig || !isOpen)
			return;

		if (!SetupCurrentWeapon())
			return;

		if (m_validAttachments.Count() < 1)
			return;

		PlayerCamera camera = PlayerCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (!camera)
			return;

		m_SelectedEntity = camera.GetCursorTarget();

		AddEntries();

		/*
		SCR_PlayerCommandingMenuCategoryElement rootCategory = m_CommandingMenuConfig.GetRootCategory();
		if (!rootCategory)
			return;

		AddElementsFromCategory(rootCategory);
		*/
	}

	// ------- adding entries

	void AddEntries()
	{
		foreach (BaconRISAttachments_AttachmentToggleAction toggleAction : m_validAttachments)
		{
			BaconRISAttachments_AttachmentRadialMenuEntry entry = new BaconRISAttachments_AttachmentRadialMenuEntry(toggleAction.GetOwner(), toggleAction);
			// entry.SetEntryLayout(s_sEntryLayout);
			m_RadialMenu.AddEntry(entry);
		};

		Print("Adding back entry", LogLevel.DEBUG);
		AddBackEntry();
	};

	void AddBackEntry()
	{
		SCR_SelectionMenuEntry entry = new SCR_SelectionMenuEntry();

		entry.SetName("back");
		entry.SetId("Back");
		entry.SetIcon("{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset", "cancel");
		entry.Enable(true);

		m_RadialMenu.AddEntry(entry);
	};

	override SCR_SelectionMenuEntry AddRadialMenuElement(SCR_PlayerCommandingMenuBaseElement newElement, SCR_SelectionMenuCategoryEntry parentCategory = null)
	{
		SCR_PlayerCommandingMenuCommand commandElement = SCR_PlayerCommandingMenuCommand.Cast(newElement);
		if (commandElement)
			return AddCommandElement(commandElement, parentCategory);

		SCR_PlayerCommandingMenuCategoryElement categoryElement = SCR_PlayerCommandingMenuCategoryElement.Cast(newElement);
		if (categoryElement)
			return AddCategoryElement(categoryElement, parentCategory);

		return null;
	};
};

// ---- attachment actions

class BaconRISAttachments_AttachmentRadialMenuEntry: SCR_SelectionMenuEntry
{
	InventoryItemComponent m_itemComponent;
	IEntity m_owner;
	BaconRISAttachments_AttachmentToggleAction m_toggleAction;

	//------------------------------------------------------------------------------------------------
	void UpdateVisuals()
	{
		SCR_SelectionMenuEntryPreviewComponent entryWidget = SCR_SelectionMenuEntryPreviewComponent.Cast(m_EntryComponent);
		if (!entryWidget)
			return;

		string actionName;
		m_toggleAction.GetActionNameScript(actionName);

		SetName(actionName);
		SetId("baconrisattachments_action_"+m_owner);
		entryWidget.SetPreviewItem(m_owner);
	}

	override void SetEntryComponent(SCR_SelectionMenuEntryComponent entryComponent)
	{
		m_EntryComponent = entryComponent;

		UpdateVisuals();
	}

	void BaconRISAttachments_AttachmentRadialMenuEntry(IEntity owner, BaconRISAttachments_AttachmentToggleAction toggleAction)
	{
		if (!owner || !toggleAction)
			return;

		m_itemComponent = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
		if (!m_itemComponent)
			return;

		m_toggleAction = toggleAction;
		m_owner = owner;

		SetCustomLayout("{93472DECDA62C46F}UI/layouts/Common/RadialMenu/SelectionMenuEntryPreview.layout");

		UpdateVisuals();
	};

	/*
	override UIInfo GetUIInfoScript()
	{
		if (!m_owner)
			return null;

		return m_itemComponent.GetUIInfo();
	};


	override bool CanBeShownScript(IEntity user, BaseSelectionMenu sourceMenu)
	{
		if (!m_toggleAction)
			return false;

		return m_toggleAction.CanBeShownScript(user);
	};
	*/

	override void OnPerform()
	{
		IEntity user = SCR_PlayerController.GetLocalControlledEntity();

		if (!user)
			return;

		ActionsPerformerComponent actionsComponent = ActionsPerformerComponent.Cast(user.FindComponent(ActionsPerformerComponent));
		if (!actionsComponent)
			return;

		// m_toggleAction.PerformAction(m_owner, user);
		actionsComponent.PerformAction(m_toggleAction);

		super.OnPerform();
	};
};

// ---- control hints

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class BaconRISAttachments_CanUseAttachmentRadialMenuCondition : SCR_AvailableActionCondition
{

	//------------------------------------------------------------------------------------------------
	override bool IsAvailable(notnull SCR_AvailableActionsConditionData data)
	{
		if (!data)
			return false;

		return GetReturnResult(CanBeUsed());
	};

	//------------------------------------------------------------------------------------------------
	bool CanBeUsed()
	{
		ChimeraCharacter playerCharacter = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!playerCharacter)
			return false;

		BaseWeaponComponent weapon = SCR_WeaponLib.GetCurrentWeaponComponent(playerCharacter);
		if (!weapon)
			return false;

		array<AttachmentSlotComponent> slots = {};

		int numAttachments = weapon.GetAttachments(slots);
		if (numAttachments < 1)
			return false;

		foreach (AttachmentSlotComponent attachment : slots)
		{
			IEntity attachedEntity = attachment.GetAttachedEntity();
			if (!attachedEntity)
				continue;

			BaconRISAttachments_AttachmentToggleAction maybeAction = GetToggleAction(attachedEntity);
			if (!maybeAction)
				continue;

			return true;
		};

		return false;
	};

	BaconRISAttachments_AttachmentToggleAction GetToggleAction(IEntity entity)
	{
		if (!entity)
			return null;

		ActionsManagerComponent actionsManagerComponent = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent));
		if (!actionsManagerComponent)
			return null;

		// proto external int GetActionsList(out notnull array<BaseUserAction> outActions);
		array<BaseUserAction> actions = {};
		int actionsNum = actionsManagerComponent.GetActionsList(actions);

		foreach (BaseUserAction action : actions)
		{
			BaconRISAttachments_AttachmentToggleAction maybeAction = BaconRISAttachments_AttachmentToggleAction.Cast(action);
			if (!maybeAction)
				continue;

			return maybeAction;
		};

		return null;
	};
};

[BaseContainerProps(configRoot: true)]
modded class SCR_RadialMenuController {
	override void Control(IEntity owner, SCR_RadialMenu radialMenu = null) {
		PrintFormat("SCR_RadialMenuController.Control -> %1 %2", owner, radialMenu);

		super.Control(owner, radialMenu);
	}
}

//modded class SCR_RadialMenuController {
//	//------------------------------------------------------------------------------------------------
//	//! Take control over selected menu
//	//! Filling no menu will take control over the global radial menu
//	override void Control(IEntity owner, SCR_RadialMenu radialMenu = null)
//	{
//		PrintFormat("bacon -> %1 %2", owner, radialMenu);
//
//		super.Control(owner, radialMenu);
//		// Clear previous inputs
//		if (radialMenu)
//			radialMenu.GetOnControllerChanged().Remove(OnMenuControllerChanged);
//
//		m_Owner = owner;
//
//		// Setup radial menu
//		if (radialMenu == null)
//			radialMenu = SCR_RadialMenu.GlobalRadialMenu();
//
//		m_RadialMenu = radialMenu;
//		if (!m_RadialMenu)
//			return;
//
//		m_RadialMenu.SetController(m_Owner, m_RMControls);
//
//		if (m_RadialMenu)
//			m_RadialMenu.GetOnControllerChanged().Insert(OnMenuControllerChanged);
//
//		InvokeOnTakeControl();
//	}
//}
