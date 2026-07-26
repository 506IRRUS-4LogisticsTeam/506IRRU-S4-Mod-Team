modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		if (damageContext.damageValue <= 0)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		IEntity instigator = damageContext.instigator.GetInstigatorEntity();
		if (instigator == owner)
			return;

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);
		if (playerId <= 0)
			return;

		IRRU_ContactViewManager.GetInstance().OnPlayerDamaged(playerId);
	}
}
