//------------------------------------------------------------------------------------------------
//! WARNING - INTENTIONALLY SUPPRESSES ACE MEDICAL BEHAVIOUR. LOAD ORDER SENSITIVE.
//!
//! ACE Medical (ACE_Medical_Bleeding) mods this class and kills the character from its
//! OnDamageStateChanged the moment this hit zone reaches DESTROYED (0% blood), unless
//! ACE_Medical_CanBleedOut() returns false. Unlike our SCR_GameModeHealthSettings override, this
//! is not a race won on timing - we suppress the kill outright for players, so ACE never reaches
//! its damageManager.Kill() call at all.
//!
//! Why: ACE gates that kill behind m_bBleedOutForPlayersEnabled, which on our servers did not take
//! effect even when set to 0 in the mission header. Players were dying the instant blood hit 0%
//! with most of the bleedout timer still remaining. Enforcing the rule here makes it independent
//! of whether ACE mod settings ever bind correctly.
//!
//! CRITICAL: with this in place a player can no longer die from blood loss by ANY path. The
//! bleedout timer in IRRU_NoInstantDeathComponent becomes the only thing that can kill an
//! unconscious player. If that component is missing from a character prefab, or fails to
//! initialise, that character is effectively immortal while unconscious.
//!
//! Compatibility consequences:
//! - AI is deliberately left on ACE behaviour via super, so AI still bleed out and die normally.
//! - EntityUtils.IsPlayer() is deliberately NOT used here even though that is ACE's own check, as
//!   it was a suspect for the original failure. PlayerManager.GetPlayerIdFromControlledEntity() is
//!   used instead, and is known to resolve correctly for unconscious players.
//! - A disconnected player's body stops being player controlled, so it falls back to ACE behaviour
//!   and can bleed out.
//! - Blood stays pinned at 0% for downed players until they are revived. That is expected.
//! - Any mod overriding ACE_Medical_CanBleedOut() or OnDamageStateChanged() without calling super
//!   re-enables ACE's kill, and players will start dying at 0% blood again with no error logged.
//------------------------------------------------------------------------------------------------
modded class SCR_CharacterBloodHitZone : SCR_RegeneratingHitZone
{
	override protected bool ACE_Medical_CanBleedOut()
	{
		if (IRRU_IsPlayerControlled(GetOwner()))
			return false;

		return super.ACE_Medical_CanBleedOut();
	}

	//------------------------------------------------------------------------------------------------
	//! Caps the combined bleed rate using IRRU_NoInstantDeathSettings. ACE already clamps its stored
	//! total with its own m_fMaxTotalBleedingRate; this clamps again at the read site so our cap
	//! applies regardless of whether ACE settings loaded. Note that anything reading ACE's
	//! m_fACE_Medical_TotalBleedingAmount member directly bypasses this - only callers of
	//! GetTotalBleedingAmount() are capped, which covers ACE_Medical_BloodLossDamageEffect.
	override float GetTotalBleedingAmount()
	{
		float bleedingRate = super.GetTotalBleedingAmount();

		float maxBleedingRate = IRRU_NoInstantDeathSettings.GetMaxTotalBleedingRate();
		if (maxBleedingRate >= 0 && bleedingRate > maxBleedingRate)
			return maxBleedingRate;

		return bleedingRate;
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
