// ============================================================================
//  NoInstantDeathComponent.c   – 2025-06-16
//  * 6-min bleed-out, 5-HP buffer, DEAD-state guard, ALIVE cancel
//  * Debug-toggle via NoInstantDeath_Settings
//  * Prints remaining time every 15 s when debug is on
// ============================================================================

[ComponentEditorProps(category: "Health",
    description: "Overrides death to force player bleed-out")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	// ─── debug helper ────────────────────────────────────────────────────
	static void NID_DebugPrint(string msg)
	{
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print("[NoInstantDeath] " + msg);
	}

	// ─── state flags ─────────────────────────────────────────────────────
	protected bool  m_bNID_Initialized  = false;
	protected bool  m_bIsUnconscious    = false;
	protected bool  m_bIsInitiatingKill = false;

	// ─── timer config ────────────────────────────────────────────────────
	protected const float m_fBleedOutTime = 360.0;   // seconds
	protected const float CHECK_INTERVAL  =   1.0;   // seconds

	protected float       m_fUnconsciousTimer = 0.0;
	protected Instigator  m_LastKnownInstigator;

	// ─── cached components ───────────────────────────────────────────────
	protected RplComponent                        m_Rpl;
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	protected SCR_CharacterControllerComponent    m_Ctrl;

	// ─── initialise refs ─────────────────────────────────────────────────
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
		{
			if (!m_CachedDmgManager)
				Print("[NoInstantDeath] ERROR: DamageManager missing – cannot init.");
			return;
		}

		m_CachedDmgManager.GetOnDamageStateChanged().Insert(HandleDamageStateChange);
		m_bNID_Initialized = true;
		Replication.BumpMe();
		NID_DebugPrint(GetNameStr(GetOwner()) + ": initialized.");
	}

	// ─── switch to unconscious ───────────────────────────────────────────
	void MakeUnconscious(IEntity owner)
	{
		if (!m_bNID_Initialized || m_bIsUnconscious || !m_CachedDmgManager)
			return;

		m_bIsUnconscious     = true;
		m_fUnconsciousTimer  = 0.0;
		m_bIsInitiatingKill  = false;
		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();

		// 5-HP buffer
		HitZone core  = m_CachedDmgManager.GetDefaultHitZone();
		if (core && core.GetHealth() < 5.0) core.SetHealth(5.0);
		HitZone head  = m_CachedDmgManager.GetHitZoneByName("Head");
		if (head && head.GetHealth() < 5.0) head.SetHealth(5.0);
		HitZone torso = m_CachedDmgManager.GetHitZoneByName("Torso");
		if (torso && torso.GetHealth() < 5.0) torso.SetHealth(5.0);

		m_CachedDmgManager.ForceUnconsciousness();

		if (Replication.IsServer())
		{
			Replication.BumpMe();
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer,
			     CHECK_INTERVAL * 1000, false);
		}

		NID_DebugPrint(GetNameStr(owner) + ": entering unconscious state.");
	}

	// ─── timer loop ───────────────────────────────────────────────────────
	protected void UpdateUnconsciousTimer()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_bIsUnconscious || !Replication.IsServer())
			return;

		// Cancel if revived
		if (m_Ctrl && m_Ctrl.GetLifeState() == ECharacterLifeState.ALIVE)
		{
			StopBleedoutTimer("revived (life-state ALIVE)");
			return;
		}

		// Block premature DEAD
		if (m_Ctrl && m_Ctrl.GetLifeState() == ECharacterLifeState.DEAD)
		{
			NID_DebugPrint(GetNameStr(owner) + " – DEAD state blocked during bleed-out.");
			m_CachedDmgManager.ForceUnconsciousness();
			HitZone core = m_CachedDmgManager.GetDefaultHitZone();
			if (core && core.GetHealth() < 1.0) core.SetHealth(1.0);
		}

		m_fUnconsciousTimer += CHECK_INTERVAL;

		// 🔔 15-second debug ping
		if (NoInstantDeath_Settings.IsDebugEnabled()
		    && Math.Mod(m_fUnconsciousTimer, 15.0) < CHECK_INTERVAL)
		{
			NID_DebugPrint(GetNameStr(owner) + ": bleed-out remaining " +
			               (m_fBleedOutTime - m_fUnconsciousTimer) + " / " +
			               m_fBleedOutTime + " s");
		}

		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			NID_DebugPrint(GetNameStr(owner) + ": bleed-out expired → character dies.");
			KillCharacter(owner);
			return;
		}

		GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer,
		     CHECK_INTERVAL * 1000, false);
	}

	// ─── kill on timer expiry ────────────────────────────────────────────
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

	// ─── damage-state callback ───────────────────────────────────────────
	protected void HandleDamageStateChange(EDamageState newState)
	{
		if (!m_bNID_Initialized || !Replication.IsServer())
			return;

		if (m_bIsUnconscious &&
		   (newState == EDamageState.UNDAMAGED ||
		    newState == EDamageState.INTERMEDIARY))
		{
			StopBleedoutTimer("damage-state conscious");
		}
	}

	// ─── stop timer (revive) ─────────────────────────────────────────────
	protected void StopBleedoutTimer(string reason)
	{
		if (!m_bIsUnconscious)
			return;

		NID_DebugPrint(GetNameStr(GetOwner()) +
		               ": bleed-out cancelled (" + reason + ").");

		m_bIsUnconscious    = false;
		m_fUnconsciousTimer = 0.0;
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		if (Replication.IsServer()) Replication.BumpMe();
	}

	// ─── replication stub ────────────────────────────────────────────────
	void OnRep_IsUnconscious() {}

	// ─── helper: readable owner name ─────────────────────────────────────
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

	// ─── public accessors ────────────────────────────────────────────────
	bool IsUnconscious()          { return m_bIsUnconscious; }
	bool IsInitiatingKill()       { return m_bIsInitiatingKill; }
	void ResetInitiatingKillFlag(){ m_bIsInitiatingKill = false; }
}
