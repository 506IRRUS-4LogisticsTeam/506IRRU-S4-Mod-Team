//------------------------------------------------------------------------------------------------
//! WARNING - INTENTIONALLY SUPPRESSES ACE MEDICAL BEHAVIOUR. LOAD ORDER SENSITIVE.
//!
//! ACE Medical (ACE_Medical_Bleeding) mods this class and kills the character from its
//! OnDamageStateChanged the moment this hit zone reaches DESTROYED (0% blood), unless
//! ACE_Medical_CanBleedOut() returns false. We suppress that kill outright for players so the
//! IRRU_NoInstantDeathComponent bleedout timer is the only thing that can kill an unconscious
//! player. A second, un-gated kill also exists on servers running the "Keep Gun When Uncon"
//! workshop mod (6088A3044B7ECBFD): it overrides UpdateConsciousness with a copy of the vanilla
//! body, resurrecting the blood-destroyed Kill() whenever it loads after ACE Medical (confirmed
//! 2026-07-19 from its source). SCR_CharacterDamageManagerComponent.Kill() therefore carries an
//! unconditional backstop; this mod now ships the weapon-stow feature itself, so that mod should
//! be REMOVED from server configs.
//!
//! THE BLEEDING RATE SCALE IS APPLIED HERE, NOT IN SCR_GameModeHealthSettings. Server log
//! evidence (2026-07-19): the game mode component verifiably held m_fDOTScale 0.25 while actual
//! blood drain matched the raw wound sum 1:1. ACE_Medical_BloodLossDamageEffect consumes
//! GetTotalBleedingAmount() directly and nothing downstream applies the game mode scale, so
//! every historical value of that knob (1.25 prefab, 0.175 layers, 0.39 mission header) was
//! inert under ACE. This getter is the single choke point the drain pipeline actually reads;
//! scale and cap are both enforced here, and the value it returns - which is what the NID
//! periodic log prints as "Bleeding: X ml/s" - is therefore the EFFECTIVE drain in ml/s.
//! m_fMaxTotalBleedingRate in IRRU_NoInstantDeathSettings caps the post-scale rate.
//!
//! Compatibility consequences:
//! - AI is deliberately left on ACE behaviour via super, so AI still bleed out and die normally.
//! - A disconnected player's body stops being player controlled, falls back to ACE behaviour,
//!   and can bleed out.
//! - Blood stays pinned at 0% for downed players until they are revived. That is expected.
//! - Any mod overriding ACE_Medical_CanBleedOut(), GetTotalBleedingAmount(), or
//!   OnDamageStateChanged() without calling super re-enables ACE's kill or drops the scale, and
//!   players start dying at 0% blood again with no error logged.
//------------------------------------------------------------------------------------------------
modded class SCR_CharacterBloodHitZone : SCR_RegeneratingHitZone
{
	override protected bool ACE_Medical_CanBleedOut()
	{
		if (IRRU_GetControllingPlayerId(IRRU_GetCharacterOwner()) > 0)
			return false;

		return super.ACE_Medical_CanBleedOut();
	}

	protected IEntity IRRU_GetCharacterOwner()
	{
		HitZoneContainerComponent container = GetHitZoneContainer();
		if (container)
		{
			IEntity containerOwner = container.GetOwner();
			if (containerOwner)
				return containerOwner;
		}

		return GetOwner();
	}

	override float GetTotalBleedingAmount()
	{
		float bleedingRate = super.GetTotalBleedingAmount() * IRRU_NoInstantDeathSettings.GetBleedingRateScale();

		float maxBleedingRate = IRRU_NoInstantDeathSettings.GetMaxTotalBleedingRate();
		if (maxBleedingRate >= 0 && bleedingRate > maxBleedingRate)
			return maxBleedingRate;

		return bleedingRate;
	}

	override void ACE_Medical_UpdateTotalBleedingAmount()
	{
		super.ACE_Medical_UpdateTotalBleedingAmount();

		if (super.GetTotalBleedingAmount() > 0)
			return;

		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(GetHitZoneContainer());
		if (damageManager)
			damageManager.IRRU_ClearBleedingParticles();
	}

	protected int IRRU_GetControllingPlayerId(IEntity entity)
	{
		if (!entity)
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		return playerManager.GetPlayerIdFromControlledEntity(entity);
	}
}
