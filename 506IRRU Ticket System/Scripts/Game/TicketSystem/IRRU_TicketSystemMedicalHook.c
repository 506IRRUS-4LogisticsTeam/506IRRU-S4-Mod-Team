//! Hooks into IRRU_NoInstantDeathComponent to award tickets at the
//! authoritative casualty moments: MakeUnconscious (+1) and KillCharacter (+5).
//!
//! The medical mod captures the causing instigator at unconscious time into
//! m_LastKnownInstigator and holds it through the bleedout window. This hook
//! reads that value and only awards a ticket when it represents a real
//! casualty (another character caused it). Self-damage, world damage, and
//! DEAD transitions that don't go through KillCharacter (respawn clicks,
//! CTD cleanup, engine glitches) are all implicitly ignored.

modded class IRRU_NoInstantDeathComponent : IRRU_NoInstantDeathComponent
{
	protected const int IRRU_TICKETS_UNCONSCIOUS = 1;
	protected const int IRRU_TICKETS_DEATH = 5;

	//------------------------------------------------------------------------------------------------
	override void MakeUnconscious(IEntity owner)
	{
		super.MakeUnconscious(owner);

		//if (!Replication.IsServer())
		//	return;

		IRRU_AwardCasualtyTickets(owner, IRRU_TICKETS_UNCONSCIOUS, "unconscious", true);
	}

	//------------------------------------------------------------------------------------------------
	override void KillCharacter(IEntity owner)
	{
		super.KillCharacter(owner);

		//if (!Replication.IsServer())
		//	return;

		IRRU_AwardCasualtyTickets(owner, IRRU_TICKETS_DEATH, "died", false);
	}

	//------------------------------------------------------------------------------------------------
	//! \param owner Character this casualty event belongs to
	//! \param amount Tickets to add
	//! \param reason Debug/log tag
	//! \param respectInvulnerability If true, suppress the award when the player
	//!   is in the revive invulnerability window (used for +1, not for +5)
	protected void IRRU_AwardCasualtyTickets(IEntity owner, int amount, string reason, bool respectInvulnerability)
	{
		if (!owner)
			return;

		// Only count real player casualties - filter out AI and GM-possessed AI
		SCR_ECharacterControlType controlType = SCR_CharacterHelper.GetCharacterControlType(owner);
		if (controlType == SCR_ECharacterControlType.AI || controlType == SCR_ECharacterControlType.POSSESSED_AI || controlType == SCR_ECharacterControlType.UNKNOWN)
			return;

		IRRU_TicketSystemGameModeComponent ticketSystem = IRRU_TicketSystemGameModeComponent.GetInstance();
		if (!ticketSystem)
			return;

		string instigatorDesc = IRRU_GetInstigatorDesc(m_LastKnownInstigator);

		// No instigator recorded -> not a real casualty (e.g. initial damage was world/null)
		if (!m_LastKnownInstigator)
		{
			if (ticketSystem.IsDebugEnabled())
				Print(string.Format("[TicketSystem] %1 %2 - no instigator recorded, no ticket", GetNameStr(owner), reason));
			return;
		}

		// Self-inflicted or world-caused casualties don't count
		IEntity attacker = m_LastKnownInstigator.GetInstigatorEntity();
		if (!attacker || attacker == owner)
		{
			if (ticketSystem.IsDebugEnabled())
				Print(string.Format("[TicketSystem] %1 %2 - instigator=%3 is self/world, no ticket", GetNameStr(owner), reason, instigatorDesc));
			return;
		}

		// Revive invulnerability only applies to going unconscious again
		if (respectInvulnerability)
		{
			IRRU_TicketSystemDamageTracker tracker = IRRU_TicketSystemDamageTracker.Cast(owner.FindComponent(IRRU_TicketSystemDamageTracker));
			if (tracker && tracker.IsTicketInvulnerable())
			{
				if (ticketSystem.IsDebugEnabled())
					Print(string.Format("[TicketSystem] %1: %2 but invulnerable (instigator=%3) - no ticket", GetNameStr(owner), reason, instigatorDesc));
				return;
			}
		}

		ticketSystem.AddTickets(amount, string.Format("%1 %2 by %3", GetNameStr(owner), reason, instigatorDesc));
	}

	//------------------------------------------------------------------------------------------------
	//! Build a human-readable description of an instigator for debug logs.
	//! Format: "PlayerName#id" for players, "PrefabName (faction=KEY)" for AI/entities,
	//! "<null>" if the instigator is missing.
	protected string IRRU_GetInstigatorDesc(Instigator inst)
	{
		if (!inst)
			return "<null>";

		int playerId = inst.GetInstigatorPlayerID();
		IEntity entity = inst.GetInstigatorEntity();

		if (playerId > 0)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (playerName.IsEmpty())
				playerName = "Player";
			return string.Format("%1#%2", playerName, playerId);
		}

		if (!entity)
			return string.Format("<no entity, type=%1>", inst.GetInstigatorType());

		string prefabName = "<unknown>";
		EntityPrefabData prefabData = entity.GetPrefabData();
		if (prefabData)
		{
			ResourceName prefabRN = prefabData.GetPrefabName();
			if (!prefabRN.IsEmpty())
			{
				TStringArray parts = new TStringArray();
				prefabRN.Split("/", parts, true);
				if (parts.Count() > 0)
					prefabName = parts[parts.Count() - 1];
			}
		}

		string factionKey = "?";
		SCR_FactionAffiliationComponent fc = SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent));
		if (fc)
		{
			Faction faction = fc.GetAffiliatedFaction();
			if (faction)
				factionKey = faction.GetFactionKey();
		}

		return string.Format("%1 (faction=%2, id=%3)", prefabName, factionKey, entity.GetID());
	}
}
