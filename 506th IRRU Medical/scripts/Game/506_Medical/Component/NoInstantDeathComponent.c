[ComponentEditorProps(category: "Health", description: "Overrides death to force unconsciousness")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	[RplProp(onRplName: "OnRep_IsUnconscious")]
	protected bool m_bIsUnconscious = false;

	protected float m_fBleedOutTime = 60.0;
	protected float m_fUnconsciousTimer = 0.0;
	protected RplComponent m_Rpl;
	protected const float CHECK_INTERVAL = 1.0;
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	protected bool m_bIsInitiatingKill = false;
	protected Instigator m_LastKnownInstigator;

	// Delay before disabling damage handling (in milliseconds)
	protected const int DAMAGE_DISABLE_DELAY_MS = 200; 

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		PrintFormat("[NoInstantDeath] Using fixed bleedout time: %1s", m_fBleedOutTime);
	}

	protected void UpdateUnconsciousTimer()
	{
		// ... (timer logic remains the same) ...
		if (!m_bIsUnconscious || !Replication.IsServer())
			return;

		m_fUnconsciousTimer += CHECK_INTERVAL;

		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			Print("[NoInstantDeath] Bleed-out timer expired. Killing character.");
			KillCharacter(GetOwner());
			return;
		}

		GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
	}

	void MakeUnconscious(IEntity owner)
	{
		if (m_bIsUnconscious || !m_CachedDmgManager)
			return;

		Print("[NoInstantDeath] Entering unconscious state.");
		m_bIsUnconscious = true;
		m_fUnconsciousTimer = 0.0;
		m_bIsInitiatingKill = false;

		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();
		if (m_LastKnownInstigator) {
			Print("[NoInstantDeath] Stored last known instigator.");
		} else {
			Print("[NoInstantDeath] Warning: Could not get last known instigator.");
		}

		m_CachedDmgManager.ForceUnconsciousness(0.1);

		// Schedule the disabling of damage handling instead of doing it immediately
		PrintFormat("[NoInstantDeath] Scheduling damage handling disable in %1ms.", DAMAGE_DISABLE_DELAY_MS);
		GetGame().GetCallqueue().CallLater(DisableDamageHandlingDelayed, DAMAGE_DISABLE_DELAY_MS, false);

		if (Replication.IsServer())
		{
			Replication.BumpMe();
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
		}
	}
	
	// --- New function to disable damage handling after a delay ---
	protected void DisableDamageHandlingDelayed()
	{
		// Check if still unconscious and manager exists (state might have changed rapidly)
		if (!m_bIsUnconscious || !m_CachedDmgManager) 
		{
			Print("[NoInstantDeath] Condition changed before damage handling disable could execute. Aborting disable.");
			return;
		}
			
		if (m_CachedDmgManager.IsDamageHandlingEnabled())
		{
			m_CachedDmgManager.EnableDamageHandling(false);
			Print("[NoInstantDeath] Disabled damage handling (delayed).");
		} else {
			Print("[NoInstantDeath] Damage handling was already disabled when delayed call executed.");
		}
	}
	// --- End new function ---

	void KillCharacter(IEntity owner)
	{
		// ... (KillCharacter logic remains the same) ...
		if (!m_bIsUnconscious || !m_CachedDmgManager) {
			Print("[NoInstantDeath] KillCharacter called but not unconscious or no dmg manager. Aborting.");
			return;
		}
		
		Print("[NoInstantDeath] Forcing character death (timer expired).");
		m_bIsUnconscious = false;
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		// Also remove the delayed damage disable call, just in case it hasn't run yet
		GetGame().GetCallqueue().Remove(DisableDamageHandlingDelayed); 


		m_bIsInitiatingKill = true;
		Print("[NoInstantDeath] Set IsInitiatingKill flag.");

		if (!m_CachedDmgManager.IsDamageHandlingEnabled())
		{
			m_CachedDmgManager.EnableDamageHandling(true);
			Print("[NoInstantDeath] Re-enabled damage handling.");
		}

		Instigator instigatorToUse = m_LastKnownInstigator;
		
		if (!instigatorToUse) {
			Print("[NoInstantDeath] Error: Stored instigator is null. Cannot proceed with Kill if it requires notnull.");
			HitZone defaultHZ = m_CachedDmgManager.GetDefaultHitZone(); 
			if (defaultHZ) {
				Print("[NoInstantDeath] Fallback: Setting health to 0 via Default HitZone due to null instigator.");
				defaultHZ.SetHealth(0); 
			} else {
				Print("[NoInstantDeath] Critical Error: Cannot Kill due to null instigator and no Default HitZone.");
			}
			ResetInitiatingKillFlag(); 
			return;
		}
		
		PrintFormat("[NoInstantDeath] Calling DamageManager.Kill() with stored instigator: %1", instigatorToUse);
		m_CachedDmgManager.Kill(instigatorToUse); 
	}

	bool IsInitiatingKill()
	{
		return m_bIsInitiatingKill;
	}
	
	void ResetInitiatingKillFlag()
	{
		m_bIsInitiatingKill = false;
		Print("[NoInstantDeath] Reset IsInitiatingKill flag.");
	}

	void OnRep_IsUnconscious()
	{
		IEntity owner = GetOwner();
		if (!owner) return;
		PrintFormat("[NoInstantDeath] Replicated unconscious state: %1", m_bIsUnconscious);
	}

	bool IsUnconscious()
	{
		return m_bIsUnconscious;
	}
}
