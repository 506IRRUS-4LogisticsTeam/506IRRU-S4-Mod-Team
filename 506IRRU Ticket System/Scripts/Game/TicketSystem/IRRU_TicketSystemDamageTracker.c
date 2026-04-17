//! Per-character component that tracks ticket invulnerability after a revive.
//!
//! The medical mod's IRRU_NoInstantDeathComponent is the authoritative source
//! for casualty events - see IRRU_TicketSystemMedicalHook.c. This tracker
//! exists solely to expose an invulnerability flag that the medical hook
//! checks before awarding +1 tickets, so a player revived from unconscious
//! can't immediately be re-counted as a casualty within the 30s window.

[ComponentEditorProps(category: "GameScripted/Misc", description: "Tracks player state for ticket system")]
class IRRU_TicketSystemDamageTrackerClass : ScriptComponentClass
{
}

class IRRU_TicketSystemDamageTracker : ScriptComponent
{
	protected const float INVULNERABILITY_WINDOW = 30.0;

	protected SCR_CharacterControllerComponent m_Ctrl;
	protected bool m_bInvulnerable = false;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;

		m_Ctrl = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));

		if (m_Ctrl)
			m_Ctrl.m_OnLifeStateChanged.Insert(OnLifeStateChanged);
		else
			Print("[TicketSystem] WARNING: SCR_CharacterControllerComponent not found on character!", LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (m_Ctrl)
			m_Ctrl.m_OnLifeStateChanged.Remove(OnLifeStateChanged);

		GetGame().GetCallqueue().Remove(ClearInvulnerability);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	bool IsTicketInvulnerable()
	{
		return m_bInvulnerable;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		if (!Replication.IsServer())
			return;

		// Start invulnerability window on revive
		if (newLifeState == ECharacterLifeState.ALIVE && previousLifeState == ECharacterLifeState.INCAPACITATED)
		{
			m_bInvulnerable = true;

			GetGame().GetCallqueue().Remove(ClearInvulnerability);
			GetGame().GetCallqueue().CallLater(ClearInvulnerability, INVULNERABILITY_WINDOW * 1000, false);

			IRRU_TicketSystemGameModeComponent ticketSystem = IRRU_TicketSystemGameModeComponent.GetInstance();
			if (ticketSystem && ticketSystem.IsDebugEnabled())
				Print(string.Format("[TicketSystem] %1: revived - %2s ticket invulnerability started", GetPlayerName(), INVULNERABILITY_WINDOW));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearInvulnerability()
	{
		m_bInvulnerable = false;

		IRRU_TicketSystemGameModeComponent ticketSystem = IRRU_TicketSystemGameModeComponent.GetInstance();
		if (ticketSystem && ticketSystem.IsDebugEnabled())
			Print(string.Format("[TicketSystem] %1: ticket invulnerability expired", GetPlayerName()));
	}

	//------------------------------------------------------------------------------------------------
	protected string GetPlayerName()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return "Unknown";

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return "Unknown";

		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(owner);
		if (!ch)
			return "Unknown";

		int playerId = pm.GetPlayerIdFromControlledEntity(ch);
		if (playerId > 0)
		{
			string name = pm.GetPlayerName(playerId);
			if (!name.IsEmpty())
				return name;
		}

		return owner.ToString();
	}
}
