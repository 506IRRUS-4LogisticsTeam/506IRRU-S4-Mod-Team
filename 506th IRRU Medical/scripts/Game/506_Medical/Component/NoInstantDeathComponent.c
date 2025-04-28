[ComponentEditorProps(category: "Health", description: "Overrides death to force unconsciousness")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	[RplProp(onRplName: "OnRep_IsUnconscious")]
	protected bool m_bIsUnconscious = false; // Tracks if component has forced unconsciousness

	protected float m_fBleedOutTime = 360; // Fixed bleedout duration
	protected float m_fUnconsciousTimer = 0.0; // Tracks time spent unconscious
	
	protected RplComponent m_Rpl;
	protected const float CHECK_INTERVAL = 1.0; // How often the timer updates
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	
	// Flag to coordinate with SCR_CharacterDamageManagerComponent override: true if this component is initiating the kill via timer.
	protected bool m_bIsInitiatingKill = false; 
	
	// Stores the instigator responsible for the initial lethal damage
	protected Instigator m_LastKnownInstigator; 


	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT); // Ensure INIT event is processed
	}

	override void EOnInit(IEntity owner) 
	{
		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));

		// Subscribe to damage state changes
		if (m_CachedDmgManager)
		{
			m_CachedDmgManager.GetOnDamageStateChanged().Insert(HandleDamageStateChange);
			PrintFormat("[NoInstantDeath] Subscribed to DamageStateChanged on %1", owner);
		} else {
			PrintFormat("[NoInstantDeath] Error: Could not find SCR_CharacterDamageManagerComponent on %1", owner);
		}
		
		PrintFormat("[NoInstantDeath] Using fixed bleedout time: %1s", m_fBleedOutTime);
	}

	// Timer loop executed on the server while m_bIsUnconscious is true
	protected void UpdateUnconsciousTimer()
	{
		// Additional check: If somehow the timer runs when not unconscious server-side, stop it.
		if (!m_bIsUnconscious || !Replication.IsServer()) 
		{
			PrintFormat("[NoInstantDeath] UpdateUnconsciousTimer called inappropriately (IsUnconscious=%1, IsServer=%2). Removing timer.", m_bIsUnconscious, Replication.IsServer());
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer); // Ensure removal
			m_fUnconsciousTimer = 0.0; // Reset timer value just in case
			return; 
		}

		m_fUnconsciousTimer += CHECK_INTERVAL;
		// PrintFormat("[NoInstantDeath] Unconscious Timer: %1 / %2", m_fUnconsciousTimer, m_fBleedOutTime); // Optional Debug

		// Check if bleedout time expired
		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			Print("[NoInstantDeath] Bleed-out timer expired. Killing character.");
			KillCharacter(GetOwner()); 
			// No return needed here, KillCharacter handles state and removes timer
		}
		else // Only schedule next update if timer hasn't expired
		{
			// Schedule next update
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false); 
		}
	}

	// Called by SCR_CharacterDamageManagerComponent override upon detecting lethal damage
	void MakeUnconscious(IEntity owner)
	{
		if (m_bIsUnconscious || !m_CachedDmgManager) return;

		Print("[NoInstantDeath] Entering unconscious state.");
		m_bIsUnconscious = true;
		m_fUnconsciousTimer = 0.0; 
		m_bIsInitiatingKill = false; // Ensure flag is false when entering state

		// Store the instigator for later use in KillCharacter
		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();
		if (!m_LastKnownInstigator) Print("[NoInstantDeath] Warning: Could not get last known instigator.");
		
		// Set health slightly above zero and force unconscious state (DESTROYED)
		// This should trigger HandleDamageStateChange, but we set m_bIsUnconscious first
		m_CachedDmgManager.ForceUnconsciousness(0.1); 

		if (Replication.IsServer())
		{
			Replication.BumpMe(); // Sync unconscious state
			// Start the timer ONLY if we successfully became unconscious
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer); // Remove any potentially lingering timers first
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false); // Start timer
			Print("[NoInstantDeath] Started unconscious timer.");
		}
	}
	

	// Called when the bleedout timer expires
	void KillCharacter(IEntity owner)
	{
		// This check should ideally only pass if the timer legitimately ran out
		if (!m_bIsUnconscious || !m_CachedDmgManager) 
		{
			PrintFormat("[NoInstantDeath] KillCharacter called but no longer unconscious (IsUnconscious=%1). Aborting kill.", m_bIsUnconscious);
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer); // Still remove timer as safety
			m_fUnconsciousTimer = 0.0;
			return; 
		}
		
		Print("[NoInstantDeath] Forcing character death (timer expired)." );
		// State will be updated by DamageManager.Kill -> HandleDamageStateChange potentially
		// m_bIsUnconscious = false; // Let HandleDamageStateChange manage this based on actual state change
		
		// Clean up timer explicitly here too
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		m_fUnconsciousTimer = 0.0; // Reset timer value

		// Set flag to signal this is the intended kill call
		m_bIsInitiatingKill = true;
		Print("[NoInstantDeath] Set IsInitiatingKill flag.");


		// Use the stored instigator for the Kill call
		Instigator instigatorToUse = m_LastKnownInstigator;
		if (!instigatorToUse) {
			Print("[NoInstantDeath] Error: Stored instigator is null. Attempting health-set fallback.");
			HitZone defaultHZ = m_CachedDmgManager.GetDefaultHitZone(); 
			if (defaultHZ) { defaultHZ.SetHealth(0); } 
			else { Print("[NoInstantDeath] Critical Error: Cannot finalize Kill."); }
			ResetInitiatingKillFlag(); // Reset flag as we didn't use Kill() override path
			return;
		}
		
		// Call the damage manager's Kill method; override will check the flag
		PrintFormat("[NoInstantDeath] Calling DamageManager.Kill() with stored instigator: %1", instigatorToUse);
		m_CachedDmgManager.Kill(instigatorToUse); 
		// The DamageManager.Kill call should eventually trigger HandleDamageStateChange again with the final (likely DEAD) state.
	}

	// Handles Damage State changes from the Damage Manager
	protected void HandleDamageStateChange(EDamageState newDamageState, HitZone HZ)
	{
		if (!Replication.IsServer()) return; // Server only logic

		bool wasUnconscious = m_bIsUnconscious;
		// DESTROYED state is used for unconsciousness by SCR_CharacterDamageManagerComponent when forcing it.
		// Other states (IDLE, DAMAGED) mean conscious. DEAD means dead.
		bool isNowConscious = (newDamageState != EDamageState.DESTROYED); 

		PrintFormat("[NoInstantDeath] HandleDamageStateChange: NewState=%1, WasUnconscious=%2, IsNowConscious=%3", typename.EnumToString(EDamageState, newDamageState), wasUnconscious, isNowConscious);

		// Check for transition FROM unconscious TO conscious/dead
		if (wasUnconscious && isNowConscious)
		{
			PrintFormat("[NoInstantDeath] Detected transition OUT OF unconscious state (New state: %1). Stopping timer.", typename.EnumToString(EDamageState, newDamageState));
			m_bIsUnconscious = false;
			m_fUnconsciousTimer = 0.0;
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer); // Crucial: Stop the timer
			Replication.BumpMe(); // Replicate the change in m_bIsUnconscious
		}
		
		// Note: Transition TO unconscious is handled by MakeUnconscious starting the timer.
		// We don't need special handling for EDamageState.DEAD here, as the timer stop logic covers it.
	}

	// Getter for the coordination flag
	bool IsInitiatingKill() { return m_bIsInitiatingKill; }
	
	// Called by SCR_CharacterDamageManagerComponent override to reset the flag
	void ResetInitiatingKillFlag() { m_bIsInitiatingKill = false; Print("[NoInstantDeath] Reset IsInitiatingKill flag."); }

	// Called when m_bIsUnconscious is updated via network
	void OnRep_IsUnconscious() 
	{ 
		IEntity owner = GetOwner(); 
		if (!owner) return; 
		// Optional: Add client-side visual/sound cues based on state change if needed
		PrintFormat("[NoInstantDeath] CLIENT %1: Replicated unconscious state: %2", owner, m_bIsUnconscious); 
	}

	// Getter for unconscious state
	bool IsUnconscious() { return m_bIsUnconscious; }
	
	// Cleanup event subscriptions
	override void OnDelete(IEntity owner) 
	{
		if (m_CachedDmgManager)
		{
			m_CachedDmgManager.GetOnDamageStateChanged().Remove(HandleDamageStateChange);
			PrintFormat("[NoInstantDeath] Unsubscribed from DamageStateChanged on %1", owner);
		}
		
		// Ensure timer is cleaned up if component is deleted while timer is running
		if (Replication.IsServer()) 
		{
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		}

		super.OnDelete(owner);
	}
}
