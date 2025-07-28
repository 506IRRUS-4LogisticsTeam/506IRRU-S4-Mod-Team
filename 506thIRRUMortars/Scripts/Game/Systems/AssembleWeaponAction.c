class AssembleWeaponAction: SCR_InventoryAction
{
	protected IEntity m_eBipodBag;
	protected IEntity m_eOriginalWeapon;
	
	override protected void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		GetGame().GetWorld().QueryEntitiesBySphere(pOwnerEntity.GetOrigin(), 5, FilterBackpacks, null);
		
		if(!m_eBipodBag)
			return;
		
		ClientStaticWeaponComponent.GetInstance().SpawnWeapon(RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent)).Id(), RplComponent.Cast(m_eBipodBag.FindComponent(RplComponent)).Id(), pUserEntity.GetAngles());
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		if(!SCR_CharacterControllerComponent.Cast(user.FindComponent(SCR_CharacterControllerComponent)).IsCharacterStationary())
			return false;
		
		return true;
	}
	
	//Adds all backpacks with StaticWeaponBagComponent and is a bipod.
	protected bool FilterBackpacks(IEntity ent) // Sphere Query Method
	{		
		//Is the entity on a player?
		if(SCR_CharacterHelper.IsAPlayer(ent.GetParent()))
			return true;
		
		//Does this entity have a StaticWeaponBagComponent
		if(!StaticWeaponBagComponent.Cast(ent.FindComponent(StaticWeaponBagComponent)))
			return true;
		
		//Does it already exist?
		if(m_eBipodBag)
			return true;
		
		//Is this a bipod bag?
		if (StaticWeaponBagComponent.Cast(ent.FindComponent(StaticWeaponBagComponent)).IsBipod()) 
			m_eBipodBag = ent;
		
		return true;
	}
}