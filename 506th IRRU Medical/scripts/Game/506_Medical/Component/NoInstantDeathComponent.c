// ============================================================================
//  NoInstantDeathComponent.c
//
//  Forces a bleed-out phase instead of an instant death.
//  The bleed-out timer is cancelled automatically when the character’s
//  life-state becomes ALIVE (polled once per second).
// ============================================================================

[ComponentEditorProps(category: "Health", description: "Overrides death to force unconsciousness")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	// ------------------------------------------------------------------ state
	[RplProp(onRplName: "OnRep_IsUnconscious")]
	protected bool  m_bIsUnconscious     = false;

	protected float m_fBleedOutTime      = 360.0; // seconds
	protected float m_fUnconsciousTimer  = 0.0;

	protected const float CHECK_INTERVAL = 1.0;   // seconds

	// ---------------------------------------------------------------- cache
	protected RplComponent                        m_Rpl;
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	protected SCR_CharacterControllerComponent    m_Ctrl;          // for life-state polling

	// ---------------------------------------------------------------- misc
	protected bool        m_bIsInitiatingKill = false;
	protected Instigator  m_LastKnownInstigator;

	// ==================================================================
	//  Helpers
	// ==================================================================

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

	// ==================================================================
	//  Init / teardown
	// ==================================================================

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

		if (!m_CachedDmgManager)
		{
			PrintFormat("[NoInstantDeath] %1: *** ERROR: DamageManager missing.", GetPlayerOrEntityNameStr(owner));
		}
		else
		{
			m_CachedDmgManager.GetOnDamageStateChanged().Insert(HandleDamageStateChange);
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (m_CachedDmgManager)
			m_CachedDmgManager.GetOnDamageStateChanged().Remove(HandleDamageStateChange);

		if (Replication.IsServer())
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

		super.OnDelete(owner);
	}

	// ==================================================================
	//  Core logic
	// ==================================================================

	void MakeUnconscious(IEntity owner)
	{
		if (m_bIsUnconscious || !m_CachedDmgManager) return;

		PrintFormat("[NoInstantDeath] %1: Entering unconscious state.", GetPlayerOrEntityNameStr(owner));

		m_bIsUnconscious     = true;
		m_fUnconsciousTimer  = 0.0;
		m_bIsInitiatingKill  = false;
		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();

		m_CachedDmgManager.ForceUnconsciousness(0.1);

		if (Replication.IsServer())
		{
			Replication.BumpMe();
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
		}
	}

	void KillCharacter(IEntity owner)
	{
		if (!m_bIsUnconscious || !m_CachedDmgManager)
		{
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			m_fUnconsciousTimer = 0.0;
			return;
		}

		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		m_fUnconsciousTimer = 0.0;
		m_bIsInitiatingKill = true;

		Instigator inst = m_LastKnownInstigator;
		if (!inst)
		{
			PrintFormat("[NoInstantDeath] %1: instigator null; fallback to health-set.", GetPlayerOrEntityNameStr(owner));
			HitZone hz = m_CachedDmgManager.GetDefaultHitZone();
			if (hz) hz.SetHealth(0);
			ResetInitiatingKillFlagInternal(owner);
			return;
		}
		m_CachedDmgManager.Kill(inst);
	}

	// -----------------------------------------------------------------
	//  Timer
	// -----------------------------------------------------------------

	protected void UpdateUnconsciousTimer()
	{
		IEntity owner = GetOwner();
		if (!owner) return;

		// safety: if already alive, stop timer
		if (m_Ctrl && m_Ctrl.GetLifeState() == ECharacterLifeState.ALIVE)
		{
			StopBleedoutTimer("Timer check: life-state is ALIVE");
			return;
		}

		if (!m_bIsUnconscious || !Replication.IsServer())
		{
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			m_fUnconsciousTimer = 0.0;
			return;
		}

		m_fUnconsciousTimer += CHECK_INTERVAL;

		if (Math.Mod(m_fUnconsciousTimer, 30.0) < CHECK_INTERVAL)
			PrintFormat("[NoInstantDeath] %1: Bleed-out remaining %2 / %3",
			            GetPlayerOrEntityNameStr(owner),
			            m_fBleedOutTime - m_fUnconsciousTimer, m_fBleedOutTime);

		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			PrintFormat("[NoInstantDeath] %1: Bleed-out expired — character dies.",
			            GetPlayerOrEntityNameStr(owner));
			KillCharacter(owner);
		}
		else
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
	}

	// -----------------------------------------------------------------
	//  Damage-state callback
	// -----------------------------------------------------------------

	protected void HandleDamageStateChange(EDamageState newState)
	{
		IEntity owner = GetOwner();
		if (!owner || !Replication.IsServer()) return;

		if (m_bIsUnconscious &&
		    (newState == EDamageState.UNDAMAGED || newState == EDamageState.INTERMEDIARY))
		{
			StopBleedoutTimer("DamageState → conscious");
		}
	}

	// -----------------------------------------------------------------
	//  Centralised shut-off
	// -----------------------------------------------------------------

	protected void StopBleedoutTimer(string reason)
	{
		if (!m_bIsUnconscious) return;

		PrintFormat("[NoInstantDeath] %1: Bleed-out cancelled (%2).",
		            GetPlayerOrEntityNameStr(GetOwner()), reason);

		m_bIsUnconscious    = false;
		m_fUnconsciousTimer = 0.0;
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		if (Replication.IsServer()) Replication.BumpMe();
	}

	// -----------------------------------------------------------------
	//  Replication / misc
	// -----------------------------------------------------------------

	void OnRep_IsUnconscious() {}

	bool IsUnconscious()     { return m_bIsUnconscious; }
	bool IsInitiatingKill()  { return m_bIsInitiatingKill; }

	void ResetInitiatingKillFlagInternal(IEntity owner) { m_bIsInitiatingKill = false; }
	void ResetInitiatingKillFlag()                      { ResetInitiatingKillFlagInternal(GetOwner()); }
}
