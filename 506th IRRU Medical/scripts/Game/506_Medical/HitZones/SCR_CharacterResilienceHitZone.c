//! Modded resilience hitzone to reduce incoming damage
//! ACE Medical handles regeneration scaling, we handle damage reduction

modded class SCR_CharacterResilienceHitZone : SCR_RegeneratingHitZone
{
	//------------------------------------------------------------------------------------------------
	//! Reduce incoming resilience damage to make characters beefier
	//! This applies to all damage sources (bullets hitting armor, explosions, etc.)
	//! Does NOT conflict with ACE Medical's regeneration scaling system
	override float ComputeEffectiveDamage(notnull BaseDamageContext damageContext, bool isDOT)
	{
		float damage = super.ComputeEffectiveDamage(damageContext, isDOT);

		// Apply damage reduction: 0.5 = 50% damage reduction (characters take half damage)
		float damageScale = IRRU_NoInstantDeathSettings.GetResilienceDamageScale();

		return damage * damageScale;
	}
}
