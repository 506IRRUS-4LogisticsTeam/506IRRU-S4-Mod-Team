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
		if (group != ECharacterHitZoneGroup.UPPERTORSO)
			return false;

		return CanApplyEffect(target, user, failReason);
	}

	//------------------------------------------------------------------------------------------------
	//! Self-application (item in hand, inventory Use, inventory body-part menu) builds its animation here.
	//! The base version sends body part Invalid, which the gauze animation never plays out, so the seal
	//! was never applied - always animate on the chest, as the medic action does via m_eHitZoneGroup.
	override ItemUseParameters GetAnimationParameters(IEntity item, notnull IEntity target, ECharacterHitZoneGroup group = ECharacterHitZoneGroup.VIRTUAL)
	{
		ChimeraCharacter char = ChimeraCharacter.Cast(target);
		if (!char)
			return null;

		SCR_CharacterDamageManagerComponent damageMgr = SCR_CharacterDamageManagerComponent.Cast(char.GetDamageManager());
		if (!damageMgr)
			return null;

		float usageDuration = m_fApplyToSelfDuration;
		SCR_ChimeraCharacter itemUser = GetItemUser(item);
		if (itemUser)
			usageDuration /= GetUsageSpeedFactor(itemUser);

		ItemUseParameters params = ItemUseParameters();
		params.SetEntity(item);
		params.SetAllowMovementDuringAction(true);
		params.SetKeepInHandAfterSuccess(false);
		params.SetCommandID(GetApplyToSelfAnimCmnd(target));
		params.SetCommandIntArg(1);
		params.SetCommandFloatArg(0.0);
		params.SetMaxAnimLength(usageDuration);
		params.SetIntParam(damageMgr.FindAssociatedBandagingBodyPart(ECharacterHitZoneGroup.UPPERTORSO));

		return params;
	}

	//------------------------------------------------------------------------------------------------
	void IRRU_ConsumableChestSeal()
	{
		m_eConsumableType = SCR_EConsumableType.IRRU_CHEST_SEAL;
	}
}
