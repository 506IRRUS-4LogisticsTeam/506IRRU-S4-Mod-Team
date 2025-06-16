// ============================================================================
//  NoInstantDeathComponent.c   – 2025-06-16  (v3)
//  • 6-min bleed-out, 5 HP buffer
//  • One “Attempted to prevent DEAD state” line; WARNING + timer cancel if DEAD
//  • Failsafe: if damage-state DESTROYED arrives, timer cancels and warns
//  • 15-second remaining-time debug pings
// ============================================================================

[ComponentEditorProps(category: "Health",
        description: "Overrides death to force player bleed-out")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	// ─── debug utility ───────────────────────────────────────────────────
	static void NID_DebugPrint(string msg)
	{
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print("[NoInstantDeath] " + msg);
	}

	// ─── state ───────────────────────────────────────────────────────────
	protected bool m_bNID_Initialized  = false;
	protected bool m_bIsUnconscious    = false;
	protected bool m_bIsInitiatingKill = false;

	protected bool m_bDeadBlockPrinted = false;  // one-shot info
	protected bool m_bDeadWarned       = false;  // one-shot warn

	// ─── timer config ────────────────────────────────────────────────────
	protected const float m_fBleedOutTime = 360.0; // s
	protected const float CHECK_INTERVAL   =   1.0; // s
	protected float       m_fUnconsciousTimer = 0.0;

	// ─── cached refs ─────────────────────────────────────────────────────
	protected Instigator                          m_LastKnownInstigator;
	protected RplComponent                        m_Rpl;
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	protected SCR_CharacterControllerComponent    m_Ctrl;

	// ─── init / delete ───────────────────────────────────────────────────
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_Rpl              = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(
			owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_Ctrl             = SCR_CharacterControllerComponent.Cast(
			owner.FindComponent(SCR_CharacterControllerComponent));
	}

	override void OnDelete(IEntity owner)
	{
		if (m_CachedDmgManager && m_bNID_Initialized)
			m_CachedDmgManager.GetOnDamageStateChanged().Remove(HandleDamageStateChange);

		if (Replication.IsServer())
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

		super.OnDelete(owner);
	}

	// ─── external init (player controller) ───────────────────────────────
	bool NID_IsInitialized() { return m_bNID_Initialized; }

	void NID_Initialize()
	{
		if (m_bNID_Initialized || !m_CachedDmgManager)
			return;

		m_CachedDmgManager.GetOnDamageStateChanged().Insert(HandleDamageStateChange);
		m_bNID_Initialized = true;
		Replication.BumpMe();
		NID_DebugPrint(GetNameStr(GetOwner()) + ": initialized.");
	}

	// ─── knock-out transition ────────────────────────────────────────────
	void MakeUnconscious(IEntity owner)
	{
		if (!m_bNID_Initialized || m_bIsUnconscious || !m_CachedDmgManager)
			return;

		m_bIsUnconscious     = true;
		m_bDeadBlockPrinted  = false;
		m_bDeadWarned        = false;
		m_fUnconsciousTimer  = 0.0;
		m_bIsInitiatingKill  = false;
		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();

		ApplySafetyBuffer(5.0);
		m_CachedDmgManager.ForceUnconsciousness();

		if (Replication.IsServer())
		{
			Replication.BumpMe();
			GetGame().GetCallqueue().CallLater(
			    UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
		}

		NID_DebugPrint(GetNameStr(owner) + ": entering unconscious state.");
	}

	protected void ApplySafetyBuffer(float minHP)
	{
		HitZone core  = m_CachedDmgManager.GetDefaultHitZone();
		if (core  && core.GetHealth()  < minHP) core.SetHealth(minHP);
		HitZone head  = m_CachedDmgManager.GetHitZoneByName("Head");
		if (head  && head.GetHealth()  < minHP) head.SetHealth(minHP);
		HitZone torso = m_CachedDmgManager.GetHitZoneByName("Torso");
		if (torso && torso.GetHealth() < minHP) torso.SetHealth(minHP);
	}

	// ─── server-side timer ───────────────────────────────────────────────
	protected void UpdateUnconsciousTimer()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_bIsUnconscious || !Replication.IsServer())
			return;

		// Alive? → stop timer
		if (m_Ctrl && m_Ctrl.GetLifeState() == ECharacterLifeState.ALIVE)
		{
			StopBleedoutTimer("revived (life-state ALIVE)");
			return;
		}

		// DEAD guard
		if (m_Ctrl && m_Ctrl.GetLifeState() == ECharacterLifeState.DEAD)
		{
			if (!m_bDeadBlockPrinted)
			{
				NID_DebugPrint("Attempted to prevent DEAD state during bleed-out.");
				m_bDeadBlockPrinted = true;
			}

			m_CachedDmgManager.ForceUnconsciousness();
			HitZone core = m_CachedDmgManager.GetDefaultHitZone();
			if (core && core.GetHealth() < 1.0) core.SetHealth(1.0);

			if (m_Ctrl.GetLifeState() == ECharacterLifeState.DEAD && !m_bDeadWarned)
			{
				Print("[NoInstantDeath][WARNING] " + GetNameStr(owner) +
				      " reached DEAD life-state before timer expiry!");
				m_bDeadWarned = true;
				StopBleedoutTimer("life-state DEAD");
				return;
			}
		}

		// Timer + 15-s ping
		m_fUnconsciousTimer += CHECK_INTERVAL;

		if (NoInstantDeath_Settings.IsDebugEnabled()
		    && Math.Mod(m_fUnconsciousTimer, 15.0) < CHECK_INTERVAL)
		{
			NID_DebugPrint(GetNameStr(owner) + ": bleed-out remaining " +
			               (m_fBleedOutTime - m_fUnconsciousTimer) + " / " +
			               m_fBleedOutTime + " s");
		}

		// Expire?
		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			NID_DebugPrint(GetNameStr(owner) +
			               ": bleed-out expired → character dies.");
			KillCharacter(owner);
			return;
		}

		// queue next tick
		GetGame().GetCallqueue().CallLater(
		    UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
	}

	// ─── timer-expiry kill ───────────────────────────────────────────────
	void KillCharacter(IEntity owner)
	{
		if (!m_bIsUnconscious || !m_CachedDmgManager)
			return;

		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		m_bIsInitiatingKill = true;

		Instigator inst = m_LastKnownInstigator;
		if (!inst)
		{
			HitZone hz = m_CachedDmgManager.GetDefaultHitZone();
			if (hz) hz.SetHealth(0);
			m_bIsInitiatingKill = false;
			return;
		}
		m_CachedDmgManager.Kill(inst);
	}

	// ─── damage-state callback (revive & DESTROYED failsafe) ─────────────
	protected void HandleDamageStateChange(EDamageState newState)
	{
		if (!m_bNID_Initialized || !Replication.IsServer())
			return;

		if (m_bIsUnconscious)
		{
			if (newState == EDamageState.UNDAMAGED ||
			    newState == EDamageState.INTERMEDIARY)
			{
				StopBleedoutTimer("damage-state conscious");
			}
			else if (newState == EDamageState.DESTROYED)
			{
				Print("[NoInstantDeath][WARNING] " + GetNameStr(GetOwner()) +
				      " damage-state DESTROYED before timer expiry!");
				StopBleedoutTimer("damage-state DESTROYED");
			}
		}
	}

	protected void StopBleedoutTimer(string reason)
	{
		if (!m_bIsUnconscious)
			return;

		NID_DebugPrint(GetNameStr(GetOwner()) + ": bleed-out cancelled (" + reason + ").");

		m_bIsUnconscious    = false;
		m_fUnconsciousTimer = 0.0;
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		if (Replication.IsServer()) Replication.BumpMe();
	}

	void OnRep_IsUnconscious() {}

	// helper: name string
	protected string GetNameStr(IEntity e)
	{
		if (!e) return "UnknownEntity(null)";
		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(e);
			if (ch)
			{
				int pid = pm.GetPlayerIdFromControlledEntity(ch);
				if (pid > 0)
				{
					string n = pm.GetPlayerName(pid);
					if (!n.IsEmpty()) return n;
				}
			}
		}
		return e.ToString();
	}

	// public getters
	bool IsUnconscious()          { return m_bIsUnconscious; }
	bool IsInitiatingKill()       { return m_bIsInitiatingKill; }
	void ResetInitiatingKillFlag(){ m_bIsInitiatingKill = false; }
}
