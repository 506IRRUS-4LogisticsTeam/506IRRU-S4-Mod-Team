//! Modded resilience hitzone to make players beefier by reducing damage

modded class SCR_CharacterResilienceHitZone : SCR_RegeneratingHitZone
{
	//------------------------------------------------------------------------------------------------
	//! Override damage calculation to reduce resilience damage based on settings
	override float ComputeEffectiveDamage(notnull BaseDamageContext damageContext, bool isDOT)
	{
		float damage = super.ComputeEffectiveDamage(damageContext, isDOT);

		// Scale down damage using configurable multiplier
		// 0.5 = 50% damage reduction (default), 1.0 = no reduction, 0.01 = 99% reduction
		float scale = IRRU_NoInstantDeathSettings.GetResilienceDamageScale();
		return damage * scale;
	}
}
