[ComponentEditorProps(category: "Health", description: "Overrides death to force unconsciousness")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	[RplProp(onRplName: "OnRep_IsUnconscious")]
	protected bool m_bIsUnconscious = false; // Tracks if component has forced unconsciousness

	protected float m_fBleedOutTime = 60.0; // Fixed bleedout duration
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
		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		PrintFormat("[NoInstantDeath] Using fixed bleedout time: %1s", m_fBleedOutTime);
	}

	// Timer loop executed on the server while m_bIsUnconscious is true
	protected void UpdateUnconsciousTimer()
	{
		if (!m_bIsUnconscious || !Replication.IsServer()) return; // Basic checks

		m_fUnconsciousTimer += CHECK_INTERVAL;

		// Check if bleedout time expired
		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			Print("[NoInstantDeath] Bleed-out timer expired. Killing character.");
			KillCharacter(GetOwner()); 
			return; 
		}

		// Schedule next update
		GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false); 
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
		
		// Set health slightly above zero and force unconscious state
		m_CachedDmgManager.ForceUnconsciousness(0.1); 

		if (Replication.IsServer())
		{
			Replication.BumpMe(); // Sync unconscious state
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false); // Start timer
		}
	}
	

	// Called when the bleedout timer expires
	void KillCharacter(IEntity owner)
	{
		if (!m_bIsUnconscious || !m_CachedDmgManager) return; // Only proceed if we think we are unconscious
		
		Print("[NoInstantDeath] Forcing character death (timer expired).");
		m_bIsUnconscious = false; // Update state
		
		// Clean up timers
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

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
	}

	// Getter for the coordination flag
	bool IsInitiatingKill() { return m_bIsInitiatingKill; }
	
	// Called by SCR_CharacterDamageManagerComponent override to reset the flag
	void ResetInitiatingKillFlag() { m_bIsInitiatingKill = false; Print("[NoInstantDeath] Reset IsInitiatingKill flag."); }

	// Called when m_bIsUnconscious is updated via network
	void OnRep_IsUnconscious() { IEntity owner = GetOwner(); if (!owner) return; PrintFormat("[NoInstantDeath] Replicated unconscious state: %1", m_bIsUnconscious); }

	// Getter for unconscious state
	bool IsUnconscious() { return m_bIsUnconscious; }
}
