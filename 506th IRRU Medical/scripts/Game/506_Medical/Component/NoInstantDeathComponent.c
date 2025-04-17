[ComponentEditorProps(category: "Health", description: "Overrides death to force unconsciousness")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	[RplProp(onRplName: "OnRep_IsUnconscious")]
	protected bool m_bIsUnconscious = false;

	protected float m_fBleedOutTime = 60.0;
	protected float m_fUnconsciousTimer = 0.0;
	protected RplComponent m_Rpl;
	protected const float CHECK_INTERVAL = 1.0; // Check only once per second instead of every frame
	
	// Cache damage manager to avoid repeated lookups
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		
		// Cache the damage manager component
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));

		// ✅ Load ACE-style mod settings
		NoInstantDeath_Settings settings = ACE_SettingsHelperT<NoInstantDeath_Settings>.GetModSettings();
		if (settings)
		{
			m_fBleedOutTime = settings.m_fBleedoutTime;
			PrintFormat("[NoInstantDeath] Loaded bleedout time: %1s", m_fBleedOutTime);
		}
		else
		{
			Print("[NoInstantDeath] No settings found — using default bleedout time.");
		}
	}

	// New method to update the unconscious timer using scheduled callbacks
	protected void UpdateUnconsciousTimer()
	{
		if (!m_bIsUnconscious || !Replication.IsServer())
			return;
			
		m_fUnconsciousTimer += CHECK_INTERVAL;
		
		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			Print("[NoInstantDeath] Bleed-out timer expired. Killing character.");
			KillCharacter(GetOwner());
			return; // Don't schedule another check if we're killing the character
		}
		
		// Schedule next check
		GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
	}

	void MakeUnconscious(IEntity owner)
	{
		if (m_bIsUnconscious)
			return;

		// Use cached damage manager instead of looking it up again
		if (!m_CachedDmgManager)
			return;
            
		// Check if ACE Medical's Second Chance is already triggered
		if (m_CachedDmgManager.ACE_Medical_IsInitialized() && m_CachedDmgManager.ACE_Medical_WasSecondChanceTrigged())
		{
			Print("[NoInstantDeath] ACE Medical Second Chance already triggered, skipping duplicate unconsciousness.");
			return;
		}

		Print("[NoInstantDeath] Entering unconscious state.");
		m_bIsUnconscious = true;
		m_fUnconsciousTimer = 0.0;

		m_CachedDmgManager.ForceUnconsciousness(0.05); // Small health buffer to simulate knockdown

		if (Replication.IsServer())
		{
			Replication.BumpMe();
			// Start timer checks on server only
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
		}
	}

	void KillCharacter(IEntity owner)
	{
		Print("[NoInstantDeath] Forcing character death.");
		m_bIsUnconscious = false;
		
		// Make sure to clear any pending timer callbacks
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

		// Use cached damage manager instead of looking it up again
		if (m_CachedDmgManager)
			m_CachedDmgManager.Kill(m_CachedDmgManager.GetInstigator());

		if (Replication.IsServer())
			Replication.BumpMe();
	}

	void OnRep_IsUnconscious()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		PrintFormat("[NoInstantDeath] Replicated unconscious state: %1", m_bIsUnconscious);
	}

	bool IsUnconscious()
	{
		return m_bIsUnconscious;
	}
}
