class IRRU_OpenLoadoutManagerSystemAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript() { return true; }
	override bool CanBroadcastScript() { return false; }

	override bool CanBeShownScript(IEntity user)
	{
		return user && SCR_ArsenalComponent.FindArsenalComponent(GetOwner());
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pUserEntity)
		{
			Print("[IRRU Loadout Manager] The Arsenal interaction has no user entity.", LogLevel.ERROR);
			return;
		}

		IRRU_LoadoutManagerSystemMenu.Open(pOwnerEntity, pUserEntity);
	}
}
