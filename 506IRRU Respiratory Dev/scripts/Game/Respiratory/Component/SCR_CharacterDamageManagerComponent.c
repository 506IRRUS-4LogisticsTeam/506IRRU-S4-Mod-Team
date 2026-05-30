//! Respiratory mod extension - adds pneumothorax trigger on chest damage

modded class SCR_CharacterDamageManagerComponent
    : SCR_CharacterDamageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override void FullHeal(bool ignoreHealingDOT = true)
	{
		super.FullHeal(ignoreHealingDOT);

		IEntity owner = GetOwner();
		if (!owner)
			return;

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			owner.FindComponent(IRRU_PneumothoraxComponent));
		if (pneumo)
			pneumo.Treat();
	}

	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		if (Replication.IsRunning() && !Replication.IsServer())
			return;

		if (damageContext.damageValue <= 0)
			return;

		SCR_ECharacterControlType controlType = SCR_CharacterHelper.GetCharacterControlType(GetOwner());
		if (controlType == SCR_ECharacterControlType.AI || controlType == SCR_ECharacterControlType.POSSESSED_AI)
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

		// Calculate damage as ratio of hitzone max health
		float maxHealth = charHZ.GetMaxHealth();
		float damageRatio = 0.0;
		if (maxHealth > 0)
			damageRatio = damageContext.damageValue / maxHealth;

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			owner.FindComponent(IRRU_PneumothoraxComponent));
		if (pneumo)
			pneumo.TryTriggerPneumothorax(damageRatio);
	}
}
