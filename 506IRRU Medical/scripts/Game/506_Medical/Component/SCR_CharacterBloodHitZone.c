//------------------------------------------------------------------------------------------------
//! WARNING - INTENTIONALLY SUPPRESSES ACE MEDICAL BEHAVIOUR. LOAD ORDER SENSITIVE.
//!
//! ACE Medical (ACE_Medical_Bleeding) mods this class and kills the character from its
//! OnDamageStateChanged the moment this hit zone reaches DESTROYED (0% blood), unless
//! ACE_Medical_CanBleedOut() returns false. We suppress that kill outright for players so the
//! IRRU_NoInstantDeathComponent bleedout timer is the only thing that can kill an unconscious
//! player. Server evidence (Nizla dedicated, 2026-07-19) showed a player still dying the instant
//! blood hit zero despite this gate - the kill likely originates in the vanilla super chain,
//! which runs BEFORE ACE's gate - so SCR_CharacterDamageManagerComponent.Kill() carries a
//! second, unconditional backstop. The debug print in CanBleedOut exists to prove in the RPT
//! whether this gate was consulted at death time and what it decided.
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
		IEntity owner = IRRU_GetCharacterOwner();
		int controllingPlayerId = IRRU_GetControllingPlayerId(owner);

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] CanBleedOut evaluated - controllingPlayerId: %1, owner: %2", controllingPlayerId, owner), LogLevel.WARNING);

		if (controllingPlayerId > 0)
			return false;

		return super.ACE_Medical_CanBleedOut();
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve the character entity via the hit zone container (the damage manager component, whose
	//! GetOwner() is guaranteed to be the character), falling back to the hit zone's own GetOwner().
	//! Guards against hit zone GetOwner() semantics differing from the character entity - the one
	//! remaining suspect for why ACE's own EntityUtils.IsPlayer gate failed on the dedicated server.
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

	//------------------------------------------------------------------------------------------------
	//! Effective drain = raw wound sum x IRRU bleeding scale, capped at the IRRU max rate. This is
	//! the exact number ACE_Medical_BloodLossDamageEffect drains per second - see banner.
	override float GetTotalBleedingAmount()
	{
		float bleedingRate = super.GetTotalBleedingAmount() * IRRU_NoInstantDeathSettings.GetBleedingRateScale();

		float maxBleedingRate = IRRU_NoInstantDeathSettings.GetMaxTotalBleedingRate();
		if (maxBleedingRate >= 0 && bleedingRate > maxBleedingRate)
			return maxBleedingRate;

		return bleedingRate;
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
