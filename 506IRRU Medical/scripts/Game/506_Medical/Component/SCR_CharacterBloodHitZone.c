modded class SCR_CharacterBloodHitZone : SCR_RegeneratingHitZone
{
	override protected bool ACE_Medical_CanBleedOut()
	{
		if (IRRU_IsPlayerControlled(GetOwner()))
			return false;

		return super.ACE_Medical_CanBleedOut();
	}

	protected bool IRRU_IsPlayerControlled(IEntity entity)
	{
		if (!entity)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		return playerManager.GetPlayerIdFromControlledEntity(entity) > 0;
	}
}
