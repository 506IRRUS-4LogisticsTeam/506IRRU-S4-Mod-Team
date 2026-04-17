//! Per-character component that detects unconscious and death state transitions
//! and reports ticket costs to the game mode component.
//!
//! Ticket costs:
//!   Unconscious: +1 ticket
//!   Death:       +5 tickets
//!
//! A 30-second invulnerability window after revival prevents double-dipping.

[ComponentEditorProps(category: "GameScripted/Misc", description: "Tracks player state for ticket system")]
class IRRU_TicketSystemDamageTrackerClass : ScriptComponentClass
{
}

class IRRU_TicketSystemDamageTracker : ScriptComponent
{
	protected const int TICKETS_UNCONSCIOUS = 1;
	protected const int TICKETS_DEATH = 5;
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
		{
			m_Ctrl.m_OnLifeStateChanged.Insert(OnLifeStateChanged);
		}
		else
		{
			Print("[TicketSystem] WARNING: SCR_CharacterControllerComponent not found on character!", LogLevel.WARNING);
		}
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
	protected void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		if (!Replication.IsServer())
			return;

		// Only count tickets for real players, not AI or GM-possessed AI
		SCR_ECharacterControlType controlType = SCR_CharacterHelper.GetCharacterControlType(GetOwner());
		if (controlType == SCR_ECharacterControlType.AI || controlType == SCR_ECharacterControlType.POSSESSED_AI || controlType == SCR_ECharacterControlType.UNKNOWN)
		{
			IRRU_TicketSystemGameModeComponent dbgTicketSystem = IRRU_TicketSystemGameModeComponent.GetInstance();
			if (dbgTicketSystem && dbgTicketSystem.IsDebugEnabled())
				Print(string.Format("[TicketSystem] Skipping entity (controlType: %1)", controlType));
			return;
		}

		IRRU_TicketSystemGameModeComponent ticketSystem = IRRU_TicketSystemGameModeComponent.GetInstance();
		if (!ticketSystem)
		{
			Print("[TicketSystem] WARNING: GameModeComponent instance not found!", LogLevel.WARNING);
			return;
		}

		// Player went unconscious
		if (newLifeState == ECharacterLifeState.INCAPACITATED && previousLifeState == ECharacterLifeState.ALIVE)
		{
			if (m_bInvulnerable)
			{
				if (ticketSystem.IsDebugEnabled())
					Print(string.Format("[TicketSystem] %1: went unconscious but invulnerable - no ticket", GetPlayerName()));
				return;
			}

			ticketSystem.AddTickets(TICKETS_UNCONSCIOUS, string.Format("%1 unconscious", GetPlayerName()));
		}

		// Player died
		if (newLifeState == ECharacterLifeState.DEAD)
		{
			ticketSystem.AddTickets(TICKETS_DEATH, string.Format("%1 died", GetPlayerName()));
		}

		// Player revived - start invulnerability window
		if (newLifeState == ECharacterLifeState.ALIVE && previousLifeState == ECharacterLifeState.INCAPACITATED)
		{
			m_bInvulnerable = true;

			GetGame().GetCallqueue().Remove(ClearInvulnerability);
			GetGame().GetCallqueue().CallLater(ClearInvulnerability, INVULNERABILITY_WINDOW * 1000, false);

			if (ticketSystem.IsDebugEnabled())
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
