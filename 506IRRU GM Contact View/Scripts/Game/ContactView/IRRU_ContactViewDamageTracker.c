//! Modded damage manager that tracks when players take damage for Contact View

modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	//------------------------------------------------------------------------------------------------
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		// Only track on authority (server)
		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		// Only track actual damage, not healing
		if (damageContext.damageValue <= 0)
			return;

		// Don't track self damage
		IEntity owner = GetOwner();
		if (!owner)
			return;

		IEntity instigator = damageContext.instigator.GetInstigatorEntity();
		if (instigator == owner)
			return;

		// Check if owner is a player
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);
		if (playerId <= 0)
			return;

		// Notify the contact view manager
		IRRU_ContactViewManager.GetInstance().OnPlayerDamaged(playerId);
	}
}
