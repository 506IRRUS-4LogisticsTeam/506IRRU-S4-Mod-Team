//------------------------------------------------------------------------------------------------
//! LOAD ORDER SENSITIVE - intentionally suppresses ACE Medical behaviour.
//!
//! ACE Medical kills the character the moment this hit zone reaches 0% blood unless
//! ACE_Medical_CanBleedOut() returns false. For players that kill is suppressed so the
//! IRRU_NoInstantDeathComponent bleedout timer is the only thing that can kill an unconscious
//! player (AI keep ACE behaviour via super). SCR_CharacterDamageManagerComponent.Kill() carries
//! the backstop for mods that re-add an un-gated kill ("Keep Gun When Uncon" does).
//!
//! The bleeding rate scale and cap are applied HERE, not through the game mode's DOT scale:
//! ACE's blood-loss effect reads GetTotalBleedingAmount() directly and nothing downstream applies
//! SCR_GameModeHealthSettings, so that knob is inert under ACE (verified on a dedicated server,
//! 2026-07-19). Configure bleeding in IRRU_NoInstantDeathSettings.conf.
//------------------------------------------------------------------------------------------------
modded class SCR_CharacterBloodHitZone : SCR_RegeneratingHitZone
{
	override protected bool ACE_Medical_CanBleedOut()
	{
		if (GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(GetOwner()) > 0)
			return false;

		return super.ACE_Medical_CanBleedOut();
	}

	//------------------------------------------------------------------------------------------------
	//! Effective drain in ml/s: scaled, then capped
	override float GetTotalBleedingAmount()
	{
		float bleedingRate = super.GetTotalBleedingAmount() * IRRU_NoInstantDeathSettings.GetBleedingRateScale();

		float maxBleedingRate = IRRU_NoInstantDeathSettings.GetMaxTotalBleedingRate();
		if (maxBleedingRate >= 0 && bleedingRate > maxBleedingRate)
			return maxBleedingRate;

		return bleedingRate;
	}

	//------------------------------------------------------------------------------------------------
	override void ACE_Medical_UpdateTotalBleedingAmount()
	{
		super.ACE_Medical_UpdateTotalBleedingAmount();

		if (super.GetTotalBleedingAmount() > 0)
			return;

		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(GetHitZoneContainer());
		if (damageManager)
			damageManager.RemoveAllBleedingParticles();
	}
}
