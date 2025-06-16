// ============================================================================
//  NoInstantDeathComponent.c  (EnforceScript – patched 2025-06-16)
//
//  Players get a reversible 6-min bleed-out instead of instant death.
//  • Adds a 5 HP safety buffer when the knockout is triggered           (Fix C)
//  • Uses ForceUnconsciousness() **without** the bogus “360 s” param
//    – the bleed-out timer is handled entirely by our own CallLater loop.
// ============================================================================

[ComponentEditorProps(category: "Health",
        description: "Overrides death to force player bleed-out")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	// ─── external init flag (set only by player-controller) ───────────────
	protected bool m_bNID_Initialized = false;   // ← AI stays FALSE
	bool   NID_IsInitialized()        { return m_bNID_Initialized; }

	void   NID_Initialize()
	{
		if (m_bNID_Initialized) return;

		if (!m_CachedDmgManager)
		{
			Print("[NoInstantDeath] ERROR: DamageManager missing – cannot init.");
			return;
		}

		m_CachedDmgManager.GetOnDamageStateChanged().Insert(HandleDamageStateChange);
		m_bNID_Initialized = true;
		Replication.BumpMe();
		PrintFormat("[NoInstantDeath] %1: initialized.",
		            GetPlayerOrEntityNameStr(GetOwner()));
	}

	// ─── config ───────────────────────────────────────────────────────────
	[RplProp(onRplName: "OnRep_IsUnconscious")]
	protected bool  m_bIsUnconscious = false;

	protected const float m_fBleedOutTime = 360.0;  // seconds
	protected const float CHECK_INTERVAL   =   1.0; // seconds

	protected float m_fUnconsciousTimer = 0.0;
	protected bool  m_bIsInitiatingKill = false;
	protected Instigator m_LastKnownInstigator;

	// ─── cached components ────────────────────────────────────────────────
	protected RplComponent                        m_Rpl;
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	protected SCR_CharacterControllerComponent    m_Ctrl;

	// ─── lifecycle ────────────────────────────────────────────────────────
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_Rpl              = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_Ctrl             = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));
	}

	override void OnDelete(IEntity owner)
	{
		if (m_CachedDmgManager && m_bNID_Initialized)
			m_CachedDmgManager.GetOnDamageStateChanged().Remove(HandleDamageStateChange);

		if (Replication.IsServer())
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

		super.OnDelete(owner);
	}

	// ─── bleed-out core ───────────────────────────────────────────────────
	void MakeUnconscious(IEntity owner)
	{
		if (!m_bNID_Initialized || m_bIsUnconscious || !m_CachedDmgManager)
			return;

		PrintFormat("[NoInstantDeath] %1: entering unconscious state.",
		            GetPlayerOrEntityNameStr(owner));

		m_bIsUnconscious     = true;
		m_fUnconsciousTimer  = 0.0;
		m_bIsInitiatingKill  = false;
		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();

		// --- Fix C: ensure at least 5 HP buffer so DESTROYED is never queued
		HitZone core = m_CachedDmgManager.GetDefaultHitZone();
		if (core && core.GetHealth() < 5.0)
			core.SetHealth(5.0);

		// Knock the engine into the unconscious animation state
		m_CachedDmgManager.ForceUnconsciousness();  // param = resilience HP, not time

		if (Replication.IsServer())
		{
			Replication.BumpMe();
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer,
			                                   CHECK_INTERVAL * 1000, false);
		}
	}

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
			if (hz) hz.SetHealth(0);   // fallback hard-kill
			m_bIsInitiatingKill = false;
			return;
		}
		m_CachedDmgManager.Kill(inst);
	}

	// ─── timer ────────────────────────────────────────────────────────────
	protected void UpdateUnconsciousTimer()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_bIsUnconscious || !Replication.IsServer())
			return;

		if (m_Ctrl && m_Ctrl.GetLifeState() == ECharacterLifeState.ALIVE)
		{
			StopBleedoutTimer("life-state is ALIVE");
			return;
		}

		m_fUnconsciousTimer += CHECK_INTERVAL;

		if (Math.Mod(m_fUnconsciousTimer, 30.0) < CHECK_INTERVAL)
			PrintFormat("[NoInstantDeath] %1: bleed-out remaining %2 / %3",
			            GetPlayerOrEntityNameStr(owner),
			            m_fBleedOutTime - m_fUnconsciousTimer, m_fBleedOutTime);

		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			PrintFormat("[NoInstantDeath] %1: bleed-out expired – character dies.",
			            GetPlayerOrEntityNameStr(owner));
			KillCharacter(owner);
		}
		else
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer,
			                                   CHECK_INTERVAL * 1000, false);
	}

	// ─── damage-state callback ────────────────────────────────────────────
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

	// ─── central stop ─────────────────────────────────────────────────────
	protected void StopBleedoutTimer(string reason)
	{
		if (!m_bIsUnconscious)
			return;

		PrintFormat("[NoInstantDeath] %1: bleed-out cancelled (%2).",
		            GetPlayerOrEntityNameStr(GetOwner()), reason);

		m_bIsUnconscious    = false;
		m_fUnconsciousTimer = 0.0;
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		if (Replication.IsServer()) Replication.BumpMe();
	}

	// ─── replication noop ────────────────────────────────────────────────
	void OnRep_IsUnconscious() {}

	// ─── helpers ─────────────────────────────────────────────────────────
	protected string GetPlayerOrEntityNameStr(IEntity e)
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

	// ─── public state accessors ──────────────────────────────────────────
	bool IsUnconscious()          { return m_bIsUnconscious; }
	bool IsInitiatingKill()       { return m_bIsInitiatingKill; }
	void ResetInitiatingKillFlag(){ m_bIsInitiatingKill = false; }
}
