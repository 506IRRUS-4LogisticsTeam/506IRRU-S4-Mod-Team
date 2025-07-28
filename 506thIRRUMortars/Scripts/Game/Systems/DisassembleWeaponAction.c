class DisassembleWeaponAction: SCR_InventoryAction
{
	protected IEntity m_eOriginalWeapon;
	
	override protected void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		vector transform[4];
		GetGame().GetCameraManager().CurrentCamera().GetWorldTransform(transform);
		ClientStaticWeaponComponent.GetInstance().DisassembleWeapon(RplComponent.Cast(pUserEntity.FindComponent(RplComponent)).Id(), RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent)).Id(), StaticWeaponComponent.Cast(pOwnerEntity.FindComponent(StaticWeaponComponent)).GetWeaponPrefab(), StaticWeaponComponent.Cast(pOwnerEntity.FindComponent(StaticWeaponComponent)).GetBipodPrefab(), transform);
	}
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_eOriginalWeapon = pOwnerEntity;
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		if(SCR_BaseCompartmentManagerComponent.Cast(m_eOriginalWeapon.FindComponent(SCR_BaseCompartmentManagerComponent)).GetOccupantCount() > 0)
			return false;
		
		if(StaticWeaponComponent.Cast(m_eOriginalWeapon.FindComponent(StaticWeaponComponent)).GetIsMoving())
			return false;
		
		if(!SCR_CharacterControllerComponent.Cast(user.FindComponent(SCR_CharacterControllerComponent)).IsCharacterStationary())
			return false;
		
		return true;
	}
}