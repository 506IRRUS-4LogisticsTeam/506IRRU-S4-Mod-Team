modded class SCR_CharacterControllerComponent : CharacterControllerComponent
{
	override void OnConsciousnessChanged(bool conscious)
	{
		if (GetLifeState() != ECharacterLifeState.INCAPACITATED)
			return;

		//AIControlComponent aiControl = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
		//if (!aiControl || !aiControl.IsAIActivated())
		//	return;
		
		IEntity currentWeapon;
		BaseWeaponManagerComponent wpnMan = GetWeaponManagerComponent();
		if (wpnMan && wpnMan.GetCurrentWeapon())
			currentWeapon = wpnMan.GetCurrentWeapon().GetOwner();
				
		if (currentWeapon)
		{
			bool dropGrenade = false;
			
			SCR_CharacterCommandHandlerComponent handler = SCR_CharacterCommandHandlerComponent.Cast(GetAnimationComponent().GetCommandHandler());
			
			EWeaponType wt = wpnMan.GetCurrentWeapon().GetWeaponType();
			if (currentWeapon.FindComponent(GrenadeMoveComponent))
			{
				BaseTriggerComponent triggerComp = BaseTriggerComponent.Cast(currentWeapon.FindComponent(BaseTriggerComponent));
				
				if ((triggerComp && triggerComp.WasTriggered()) || (handler && handler.IsThrowingAction()))
				{
					dropGrenade = true;
				}
			}
			
			if (dropGrenade)
				handler.DropLiveGrenadeFromHand(false); 
			else 
				TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedContextual, true);
		}
	}
};
