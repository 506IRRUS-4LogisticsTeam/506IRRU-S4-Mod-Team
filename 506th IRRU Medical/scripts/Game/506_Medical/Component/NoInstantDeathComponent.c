//! Main component for no-instant-death medical system

[ComponentEditorProps(category: "Health",
		description: "Overrides death to force player bleed-out")]
class IRRU_NoInstantDeathComponentClass : ScriptComponentClass
{
}

//! Prevents instant death and manages bleedout timer
class IRRU_NoInstantDeathComponent : ScriptComponent
{
	// ─── debug utility ───────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	static void DebugPrint(string msg)
	{
		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print("[NoInstantDeath][NID] " + msg);
	}

	// ─── state ───────────────────────────────────────────────────────────
	protected bool m_bNID_Initialized  = false;
	[RplProp(onRplName: "OnUnconsciousStateChanged")]
	protected bool m_bIsUnconscious    = false;
	protected bool m_bIsInitiatingKill = false;
	[RplProp(onRplName: "OnCPRStateChanged")]
	protected bool m_bReceivingCPR     = false;  // CPR flag to pause timer
	[RplProp()]
	protected float m_fCPRCooldownEnd  = 0.0;   // World time when CPR cooldown expires

	protected bool m_bDeadBlockPrinted = false;  // one-shot info
	protected bool m_bDeadWarned       = false;  // one-shot warn

	// ─── timer config ────────────────────────────────────────────────────
	protected const float CHECK_INTERVAL   =   1.0; // s
	[RplProp()]
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
		
		// NEW: Register for life state changes to catch bleed-to-unconscious
		if (m_Ctrl)
			m_Ctrl.m_OnLifeStateChanged.Insert(OnLifeStateChanged);
		
		// Log version info once per session (using static to prevent spam)
		static bool s_versionLogged = false;
		if (!s_versionLogged && IRRU_NoInstantDeathSettings.IsDebugEnabled())
		{
			Print(string.Format("[NoInstantDeath][NID] Medical Mod v%1 loaded successfully", 
			                   IRRU_NoInstantDeathSettings.MOD_VERSION));
			s_versionLogged = true;
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (m_CachedDmgManager && m_bNID_Initialized)
			m_CachedDmgManager.GetOnDamageStateChanged().Remove(HandleDamageStateChange);

		// NEW: Unregister life state listener
		if (m_Ctrl)
			m_Ctrl.m_OnLifeStateChanged.Remove(OnLifeStateChanged);

		if (Replication.IsServer())
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

		super.OnDelete(owner);
	}

	// ─── external init (player controller) ───────────────────────────────
	bool IsInitialized() { return m_bNID_Initialized; }

	void Initialize()
	{
		if (m_bNID_Initialized || !m_CachedDmgManager)
			return;

		m_CachedDmgManager.GetOnDamageStateChanged().Insert(HandleDamageStateChange);
		m_bNID_Initialized = true;
		if (m_Rpl)
			Replication.BumpMe();
		
		// Debug: Log initialization with timer value
		float bleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
		DebugPrint(string.Format("%1: initialized with %2s bleedout timer", 
		                            GetNameStr(GetOwner()), bleedoutTime));
	}

	// ─── NEW: Life state change handler ──────────────────────────────────
	protected void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		// Catch transition to unconscious from any source (bleeding, resilience, etc)
		if (newLifeState == ECharacterLifeState.INCAPACITATED && 
		    previousLifeState == ECharacterLifeState.ALIVE &&
		    m_bNID_Initialized && !m_bIsUnconscious)
		{
			DebugPrint(GetNameStr(GetOwner()) + 
			               ": detected unconscious from bleeding/resilience, starting timer");
			MakeUnconscious(GetOwner());
		}
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
			// Safety: Remove any existing timer before starting new one
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			
			if (m_Rpl)
				Replication.BumpMe();
			GetGame().GetCallqueue().CallLater(
			    UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
		}

		float bleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
		DebugPrint(string.Format("%1: entering unconscious state (timer: %2s)", 
		                            GetNameStr(owner), bleedoutTime));
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
				DebugPrint("Attempted to prevent DEAD state during bleed-out.");
				m_bDeadBlockPrinted = true;
			}

			m_CachedDmgManager.ForceUnconsciousness();
			HitZone core = m_CachedDmgManager.GetDefaultHitZone();
			if (core && core.GetHealth() < 1.0) core.SetHealth(1.0);

			if (m_Ctrl.GetLifeState() == ECharacterLifeState.DEAD && !m_bDeadWarned)
			{
				DebugPrint("[WARNING] " + GetNameStr(owner) +
				      " reached DEAD life-state before timer expiry!");
				m_bDeadWarned = true;
				StopBleedoutTimer("life-state DEAD");
				return;
			}
		}

		// Timer only increments if NOT receiving CPR
		if (!m_bReceivingCPR)
		{
			m_fUnconsciousTimer += CHECK_INTERVAL;
		}
		else
		{
			// Log CPR is pausing timer (only once every 10 seconds to avoid spam)
			if (IRRU_NoInstantDeathSettings.IsDebugEnabled() 
			    && Math.Mod(m_fUnconsciousTimer, 10.0) < CHECK_INTERVAL)
			{
				DebugPrint(GetNameStr(owner) + ": Timer paused - receiving CPR");
			}
		}
		
		// Bump replication periodically so clients get timer updates
		// Update every 0.5 seconds for smooth timer display
		if (m_Rpl && Math.Mod(m_fUnconsciousTimer, 0.5) < CHECK_INTERVAL)
		{
			Replication.BumpMe();
		}

		// Only show periodic updates in verbose debug mode (every 30 seconds instead of 15)
		if (IRRU_NoInstantDeathSettings.IsDebugEnabled()
		    && Math.Mod(m_fUnconsciousTimer, 30.0) < CHECK_INTERVAL)
		{
			float bleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
			DebugPrint(string.Format("%1: bleed-out remaining %2 / %3 s",
			               GetNameStr(owner),
			               (bleedoutTime - m_fUnconsciousTimer),
			               bleedoutTime));
		}

		// Expire?
		float bleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
		if (m_fUnconsciousTimer >= bleedoutTime)
		{
			DebugPrint(GetNameStr(owner) +
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
				DebugPrint("[WARNING] " + GetNameStr(GetOwner()) +
				      " damage-state DESTROYED before timer expiry!");
				StopBleedoutTimer("damage-state DESTROYED");
			}
		}
	}

	protected void StopBleedoutTimer(string reason)
	{
		if (!m_bIsUnconscious)
			return;

		DebugPrint(GetNameStr(GetOwner()) + ": bleed-out cancelled (" + reason + ").");

		m_bIsUnconscious    = false;
		m_fUnconsciousTimer = 0.0;
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		if (Replication.IsServer() && m_Rpl) 
			Replication.BumpMe();
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

	// ══════════════════════════════════════════════════════════════════════
	// NEW TIMER EXPOSURE METHODS FOR UI
	// ══════════════════════════════════════════════════════════════════════

	//! Get remaining bleedout time in seconds
	float GetBleedoutTimeRemaining()
	{
		if (!m_bIsUnconscious)
			return -1.0; // Not bleeding out
			
		return IRRU_NoInstantDeathSettings.GetBleedoutTime() - m_fUnconsciousTimer;
	}

	//! Get total bleedout time in seconds
	float GetBleedoutTimeTotal()
	{
		return IRRU_NoInstantDeathSettings.GetBleedoutTime();
	}

	//! Get bleedout timer percentage (0-100)
	float GetBleedoutPercentage()
	{
		if (!m_bIsUnconscious)
			return 100.0; // Full time if not bleeding out
			
		float remaining = GetBleedoutTimeRemaining();
		if (remaining <= 0)
			return 0.0;
			
		return (remaining / IRRU_NoInstantDeathSettings.GetBleedoutTime()) * 100.0;
	}

	// ══════════════════════════════════════════════════════════════════════
	// ORIGINAL PUBLIC GETTERS
	// ══════════════════════════════════════════════════════════════════════
	
	bool IsUnconscious()          { return m_bIsUnconscious; }
	bool IsInitiatingKill()       { return m_bIsInitiatingKill; }
	void ResetInitiatingKillFlag(){ m_bIsInitiatingKill = false; }
	
	// ══════════════════════════════════════════════════════════════════════
	// REPLICATION CALLBACKS
	// ══════════════════════════════════════════════════════════════════════
	
	//! Called when unconscious state changes (for client synchronization)
	protected void OnUnconsciousStateChanged()
	{
		// This is called on clients when the server updates m_bIsUnconscious
		// No action needed - the UI will read the updated value
	}
	
	// ─── CPR Methods ─────────────────────────────────────────────────────
	
	//! Set whether patient is receiving CPR
	void SetReceivingCPR(bool receiving)
	{
		if (m_bReceivingCPR == receiving)
			return;
			
		m_bReceivingCPR = receiving;
		
		if (m_Rpl)
			Replication.BumpMe();
		
		DebugPrint(string.Format("%1: CPR state changed to %2", 
		                            GetNameStr(GetOwner()), receiving));
	}
	
	//! Check if patient is receiving CPR
	bool IsReceivingCPR()
	{
		return m_bReceivingCPR;
	}
	
	//! RPC callback for CPR state changes
	protected void OnCPRStateChanged()
	{
		// Could add visual/audio feedback here
		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			DebugPrint(string.Format("%1: CPR state replicated - now %2", 
			                            GetNameStr(GetOwner()), m_bReceivingCPR));
	}
	
	//! Set CPR cooldown end time
	void SetCPRCooldownEnd(float cooldownEndTime)
	{
		m_fCPRCooldownEnd = cooldownEndTime;
		
		if (m_Rpl)
			Replication.BumpMe();
	}
	
	//! Get CPR cooldown end time
	float GetCPRCooldownEnd()
	{
		return m_fCPRCooldownEnd;
	}
	
	//! Check if currently on CPR cooldown
	bool IsOnCPRCooldown()
	{
		if (m_fCPRCooldownEnd <= 0)
			return false;
			
		float currentTime = GetGame().GetWorld().GetWorldTime();
		return currentTime < m_fCPRCooldownEnd;
	}
	
	//! Get remaining CPR cooldown in seconds
	float GetCPRCooldownRemaining()
	{
		if (!IsOnCPRCooldown())
			return 0.0;
			
		float currentTime = GetGame().GetWorld().GetWorldTime();
		return (m_fCPRCooldownEnd - currentTime) / 1000.0;
	}
}