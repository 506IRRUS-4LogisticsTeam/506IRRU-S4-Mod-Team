//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class BaconRISAttachments_SightsIlluminationActionCondition : SCR_AvailableActionCondition
{

	//------------------------------------------------------------------------------------------------
	//! Return true if currently controlled vehicle turret has illumination for sights
	override bool IsAvailable(notnull SCR_AvailableActionsConditionData data)
	{
		if (!data)
			return false;

		BaconRISAttachments_DynamicEmissiveLevelSightsComponent sights = CurrentWeaponSights(data);
		if (!sights)
			return false;

		// Has illumination
		return GetReturnResult(sights.HasIllumination());
	};

	//------------------------------------------------------------------------------------------------
	protected BaconRISAttachments_DynamicEmissiveLevelSightsComponent CurrentWeaponSights(SCR_AvailableActionsConditionData data)
	{
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (!controlledEntity)
			return null;

		ChimeraCharacter character = ChimeraCharacter.Cast(controlledEntity);
		if (!character)
			return null;

		BaseWeaponManagerComponent weaponManager = character.GetCharacterController().GetWeaponManagerComponent();
		if (!weaponManager)
			return null;

		BaseSightsComponent currentSights = weaponManager.GetCurrentSights();
		if (!currentSights)
			return null;

		return BaconRISAttachments_DynamicEmissiveLevelSightsComponent.Cast(currentSights);;
	};
};
