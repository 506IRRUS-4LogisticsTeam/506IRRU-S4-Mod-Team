/*
//------------------------------------------------------------------------------------------------
//! Hardcoded bleed-rate multiplier applied at the per-effect computation site.
//! Bypasses ACE's m_fBleedingRateScale setting so the rate is consistent regardless
//! of whether the mod's ACE settings file is actually loaded by the active scenario.
modded class SCR_CharacterBloodHitZone : SCR_RegeneratingHitZone
{
	protected const float IRRU_BLEEDING_RATE_MULTIPLIER = 0.20;

	//------------------------------------------------------------------------------------------------
	override protected float ACE_Medical_ComputeBleedingRateForDamageEffect(SCR_CharacterDamageManagerComponent damageManager, SCR_BleedingDamageEffect bleedingEffect)
	{
		return super.ACE_Medical_ComputeBleedingRateForDamageEffect(damageManager, bleedingEffect) * IRRU_BLEEDING_RATE_MULTIPLIER;
	}
}
*/
