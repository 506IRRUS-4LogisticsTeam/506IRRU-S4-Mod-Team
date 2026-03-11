//! Respiratory mod extension - adds pneumothorax trigger on chest damage

modded class SCR_CharacterDamageManagerComponent
    : SCR_CharacterDamageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		if (Replication.IsRunning() && !Replication.IsServer())
			return;

		if (damageContext.damageValue <= 0)
			return;

		if (damageContext.damageType == EDamageType.HEALING || damageContext.damageType == EDamageType.REGENERATION)
			return;

		SCR_CharacterHitZone charHZ = SCR_CharacterHitZone.Cast(damageContext.struckHitZone);
		if (!charHZ)
			return;

		if (charHZ.GetHitZoneGroup() != ECharacterHitZoneGroup.UPPERTORSO)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		if (!GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner))
			return;

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			owner.FindComponent(IRRU_PneumothoraxComponent));
		if (pneumo)
			pneumo.TryTriggerPneumothorax();
	}
}
