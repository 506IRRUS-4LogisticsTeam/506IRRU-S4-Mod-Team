class IRRU_ChestSealUserAction : SCR_HealingUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!user || user == GetOwner())
			return false;

		ChimeraCharacter targetCharacter = ChimeraCharacter.Cast(GetOwner());
		if (!targetCharacter)
			return false;

		ChimeraCharacter userCharacter = ChimeraCharacter.Cast(user);
		if (!userCharacter)
			return false;

		CharacterControllerComponent userController = userCharacter.GetCharacterController();
		if (!userController || userController.IsUsingItem())
			return false;

		if (userCharacter.IsInVehicle() && !HealingAllowedFromSeat(userCharacter))
			return false;

		SCR_ConsumableItemComponent consumableComponent = GetConsumableComponent(userCharacter);
		if (!consumableComponent || consumableComponent.GetConsumableType() != m_eConsumableType)
			return false;

		SCR_CharacterDamageManagerComponent damageMan = SCR_CharacterDamageManagerComponent.Cast(targetCharacter.GetDamageManager());
		if (!damageMan || damageMan.GetState() == EDamageState.DESTROYED)
			return false;

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			targetCharacter.FindComponent(IRRU_PneumothoraxComponent));
		if (!pneumo || !pneumo.HasPneumothorax())
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		if (controller.GetLifeState() != ECharacterLifeState.ALIVE)
			return;

		SCR_ConsumableItemComponent consumableComponent = GetConsumableComponent(character);
		if (consumableComponent)
			consumableComponent.SetAlternativeModel(false);
	}
}
