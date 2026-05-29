class BaconRISAttachments_DetachAttachmentUserAction: SCR_AttachementAction
{
	private IEntity m_owner;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_owner = pOwnerEntity;
		m_Attachment = pOwnerEntity;

		super.Init(pOwnerEntity, pManagerComponent);
	};

	override bool HasLocalEffectOnlyScript()
	{
		return true;
	};

	override bool CanBroadcastScript()
	{
		return false;
	};

	override void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
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
