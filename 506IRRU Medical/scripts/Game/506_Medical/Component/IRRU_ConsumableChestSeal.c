[BaseContainerProps()]
class IRRU_ConsumableChestSeal : SCR_ConsumableEffectHealthItems
{
	//------------------------------------------------------------------------------------------------
	override void ApplyEffect(notnull IEntity target, notnull IEntity user, IEntity item, ItemUseParameters animParams)
	{
		ChimeraCharacter char = ChimeraCharacter.Cast(target);
		if (!char)
			return;

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			char.FindComponent(IRRU_PneumothoraxComponent));
		if (pneumo && pneumo.HasPneumothorax())
			pneumo.Treat();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanApplyEffect(notnull IEntity target, notnull IEntity user, out SCR_EConsumableFailReason failReason)
	{
		ChimeraCharacter char = ChimeraCharacter.Cast(target);
		if (!char)
			return false;

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			char.FindComponent(IRRU_PneumothoraxComponent));
		if (!pneumo || !pneumo.HasPneumothorax())
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanApplyEffectToHZ(notnull IEntity target, notnull IEntity user, ECharacterHitZoneGroup group, out SCR_EConsumableFailReason failReason = SCR_EConsumableFailReason.NONE)
	{
		return CanApplyEffect(target, user, failReason);
	}

	//------------------------------------------------------------------------------------------------
	void IRRU_ConsumableChestSeal()
	{
		m_eConsumableType = SCR_EConsumableType.IRRU_CHEST_SEAL;
	}
}
