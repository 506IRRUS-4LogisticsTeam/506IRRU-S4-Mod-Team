class BaconRISAttachments_AttachmentToggleAction: SCR_InventoryAction
{

};

class BaconRISAttachments_LightToggleAction: BaconRISAttachments_AttachmentToggleAction
{
	private BaconRISAttachments_LightManagerComponent m_lightManagerComponent;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);

		m_lightManagerComponent = BaconRISAttachments_LightManagerComponent.Cast(pOwnerEntity.FindComponent(BaconRISAttachments_LightManagerComponent));
	};

	bool HasLight()
	{
		if (!m_lightManagerComponent)
			return false;

		if (!m_lightManagerComponent.HasLight())
			return false;

		return true;
	};

	override bool CanBeShownScript(IEntity user)
	{
		if (!HasLight())
			return false;

		if(!m_Item || !m_Item.GetParentSlot())
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if(!character)
			return false;

		CharacterControllerComponent controller = character.GetCharacterController();
		if(!controller)
			return false;

		if(!controller.GetInspect())
			return false;

		BaseWeaponManagerComponent weaponManager = controller.GetWeaponManagerComponent();
		if(!weaponManager)
			return false;

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if(!inventoryManager)
			return false;

		BaseWeaponComponent weaponComp = controller.GetWeaponManagerComponent().GetCurrentWeapon();
		if(!weaponComp || weaponComp.GetOwner() != m_Item.GetParentSlot().GetOwner())
			return false;

		return true;
	};

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		SCR_InventoryStorageManagerComponent genericInventoryManager =  SCR_InventoryStorageManagerComponent.Cast(pUserEntity.FindComponent( SCR_InventoryStorageManagerComponent ));
		if ( !genericInventoryManager )
			return;

		PerformActionInternal( genericInventoryManager, pOwnerEntity, pUserEntity);
	};

	override void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!HasLight())
			return;

		if (m_lightManagerComponent.IsTurnedOn())
		{
			m_lightManagerComponent.TurnOff();
		}
		else
		{
			m_lightManagerComponent.TurnOn(pUserEntity, manager);
		};
	};

	override bool GetActionNameScript(out string outName)
	{
		if (!m_Item)
			return false;

		if (!HasLight())
			return false;

		UIInfo itemInfo = m_Item.GetUIInfo();

		if (m_lightManagerComponent.IsTurnedOn())
			outName = itemInfo.GetName() + " OFF";
		else
			outName = itemInfo.GetName() + " ON";

		return true;
	};

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	};

	override bool CanBroadcastScript()
	{
		return true;
	};
};

class BaconRISAttachments_LightPickupAction: SCR_PickUpItemAction
{
	private BaconRISAttachments_LightManagerComponent m_lightManagerComponent;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_lightManagerComponent = BaconRISAttachments_LightManagerComponent.Cast(pOwnerEntity.FindComponent(BaconRISAttachments_LightManagerComponent));
	};

	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if(m_lightManagerComponent)
			m_lightManagerComponent.TurnOff();

		super.PerformActionInternal(manager, pOwnerEntity, pUserEntity);
	};
};



class BaconRISAttachments_LightDetachAction: SCR_AttachementAction
{
	private BaconRISAttachments_LightManagerComponent m_lightManagerComponent;
	private IEntity m_owner;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_owner = pOwnerEntity;
		m_Attachment = pOwnerEntity;

		m_lightManagerComponent = BaconRISAttachments_LightManagerComponent.Cast(pOwnerEntity.FindComponent(BaconRISAttachments_LightManagerComponent));

		super.Init(pOwnerEntity, pManagerComponent);
	};

	override bool HasLocalEffectOnlyScript() {
		return true;
	}

	override bool CanBroadcastScript() {
		return false;
	}

	override void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		//m_lightManagerComponent.RpcAsk_TurnOffPlease();

		super.PerformActionInternal( manager, pOwnerEntity, pUserEntity);
	};


	override bool CanBeShownScript(IEntity user)
	{
		if (!m_Attachment)
			return false;

		if(!m_InventoryItemComp || !m_InventoryItemComp.GetParentSlot())
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if(!character)
			return false;

		CharacterControllerComponent controller = character.GetCharacterController();
		if(!controller)
			return false;

		if(!controller.GetInspect())
			return false;

		BaseWeaponManagerComponent weaponManager = controller.GetWeaponManagerComponent();
		if(!weaponManager)
			return false;

		if(!m_InventoryManager)
			m_InventoryManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));

		if(!m_InventoryManager)
			return false;

		BaseWeaponComponent weaponComp = controller.GetWeaponManagerComponent().GetCurrentWeapon();
		if(!weaponComp || weaponComp.GetOwner() != m_InventoryItemComp.GetParentSlot().GetOwner())
			return false;

		return true;
	};

};
