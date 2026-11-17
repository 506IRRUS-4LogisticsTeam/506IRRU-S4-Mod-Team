//! Respiratory mod extension - offers the chest seal on the chest in the inventory body-part treatment menu.
//! Vanilla only matches its own consumable types, so the seal never showed there.
modded class SCR_ApplicableMedicalItemPredicate
{
	//------------------------------------------------------------------------------------------------
	override protected bool IsMatch(BaseInventoryStorageComponent storage, IEntity item, array<GenericComponent> queriedComponents, array<BaseItemAttributeData> queriedAttributes)
	{
		SCR_ConsumableItemComponent consumable = SCR_ConsumableItemComponent.Cast(queriedComponents[0]);
		if (!consumable || consumable.GetConsumableType() != SCR_EConsumableType.IRRU_CHEST_SEAL)
			return super.IsMatch(storage, item, queriedComponents, queriedAttributes);

		IRRU_ConsumableChestSeal effect = IRRU_ConsumableChestSeal.Cast(consumable.GetConsumableEffect());
		return effect && effect.CanApplyEffectToHZ(characterEntity, characterEntity, hitZoneGroup);
	}
}
