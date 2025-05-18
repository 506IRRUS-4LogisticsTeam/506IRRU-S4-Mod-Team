[ComponentEditorProps(category: "Health", description: "Overrides death to force unconsciousness")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	[RplProp(onRplName: "OnRep_IsUnconscious")]
	protected bool m_bIsUnconscious = false;

	protected float m_fBleedOutTime = 360; 
	protected float m_fUnconsciousTimer = 0.0;
	
	protected RplComponent m_Rpl;
	protected const float CHECK_INTERVAL = 1.0; 
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	
	protected bool m_bIsInitiatingKill = false; 
	protected Instigator m_LastKnownInstigator; 

	// Helper to get player name or entity string
	protected string GetPlayerOrEntityNameStr(IEntity entity)
	{
		if (!entity) return "UnknownEntity(null)";

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (character)
			{
				int playerId = playerManager.GetPlayerIdFromControlledEntity(character);
				if (playerId > 0) 
				{
					string playerName = playerManager.GetPlayerName(playerId);
					if (!playerName.IsEmpty())
					{
						return playerName;
					}
				}
			}
		}
		return entity.ToString(); // Fallback
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner) 
	{
		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));

		if (m_CachedDmgManager)
		{
			m_CachedDmgManager.GetOnDamageStateChanged().Insert(HandleDamageStateChange);
			// PrintFormat("[NoInstantDeath] %1: Subscribed to DamageStateChanged.", GetPlayerOrEntityNameStr(owner)); // Production: Verbose
		} else {
			PrintFormat("[NoInstantDeath] %1: CRITICAL ERROR: Could not find SCR_CharacterDamageManagerComponent.", GetPlayerOrEntityNameStr(owner));
		}
		
		// PrintFormat("[NoInstantDeath] %1: Using fixed bleedout time: %2s", GetPlayerOrEntityNameStr(owner), m_fBleedOutTime); // Production: Verbose
	}

	protected void UpdateUnconsciousTimer()
	{
		IEntity owner = GetOwner();
		if (!owner) return;

		if (!m_bIsUnconscious || !Replication.IsServer()) 
		{
			// This case should ideally not happen if logic is sound elsewhere
			// PrintFormat("[NoInstantDeath] %1: UpdateUnconsciousTimer called inappropriately (IsUnconscious=%2, IsServer=%3). Removing timer.", GetPlayerOrEntityNameStr(owner), m_bIsUnconscious, Replication.IsServer());
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			m_fUnconsciousTimer = 0.0;
			return; 
		}

		m_fUnconsciousTimer += CHECK_INTERVAL;

		// Log bleed-out timer every 30 seconds
		// Corrected to use Math.Mod for Enfusion Script
		float remainder = Math.Mod(m_fUnconsciousTimer, 30.0);
		if (remainder < CHECK_INTERVAL && remainder >= 0) // Check if we are at or just passed a 30s mark (remainder will be 0 if it's an exact multiple)
		{
			PrintFormat("[NoInstantDeath] %1: Bleed-out time remaining: %2s / %3s", GetPlayerOrEntityNameStr(owner), m_fBleedOutTime - m_fUnconsciousTimer, m_fBleedOutTime);
		}

		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			PrintFormat("[NoInstantDeath] %1: Bleed-out timer expired. Character died.", GetPlayerOrEntityNameStr(owner));
			KillCharacter(owner); 
		}
		else
		{
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false); 
		}
	}

	void MakeUnconscious(IEntity owner)
	{
		if (m_bIsUnconscious || !m_CachedDmgManager) return;

		PrintFormat("[NoInstantDeath] %1: Entering unconscious state (bleed-out).", GetPlayerOrEntityNameStr(owner));
		m_bIsUnconscious = true;
		m_fUnconsciousTimer = 0.0; 
		m_bIsInitiatingKill = false;

		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();
		// if (!m_LastKnownInstigator) PrintFormat("[NoInstantDeath] %1: Warning: Could not get last known instigator for MakeUnconscious.", GetPlayerOrEntityNameStr(owner)); // Production: Verbose
		
		m_CachedDmgManager.ForceUnconsciousness(0.1); 

		if (Replication.IsServer())
		{
			Replication.BumpMe();
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
			// PrintFormat("[NoInstantDeath] %1: Started unconscious timer.", GetPlayerOrEntityNameStr(owner)); // Production: Verbose, implied by "Entering unconscious state"
		}
	}
	

	void KillCharacter(IEntity owner)
	{
		if (!m_bIsUnconscious || !m_CachedDmgManager) 
		{
			// PrintFormat("[NoInstantDeath] %1: KillCharacter called but no longer appropriately (IsUnconscious=%2, HasDmgManager=%3). Aborting kill.", GetPlayerOrEntityNameStr(owner), m_bIsUnconscious, m_CachedDmgManager != null); // Production: Verbose
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			m_fUnconsciousTimer = 0.0;
			return; 
		}
		
		// Message printed by UpdateUnconsciousTimer is sufficient: "Bleed-out timer expired. Character died."
		// PrintFormat("[NoInstantDeath] %1: Forcing character death (timer expired)." , GetPlayerOrEntityNameStr(owner)); // Production: Redundant
		
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		m_fUnconsciousTimer = 0.0;

		m_bIsInitiatingKill = true;
		// PrintFormat("[NoInstantDeath] %1: Set IsInitiatingKill flag.", GetPlayerOrEntityNameStr(owner)); // Production: Verbose

		Instigator instigatorToUse = m_LastKnownInstigator;
		if (!instigatorToUse) 
		{
			PrintFormat("[NoInstantDeath] %1: Warning: Stored instigator is null for KillCharacter. Attempting health-set fallback.", GetPlayerOrEntityNameStr(owner));
			HitZone defaultHZ = m_CachedDmgManager.GetDefaultHitZone(); 
			if (defaultHZ) { defaultHZ.SetHealth(0); } 
			else { PrintFormat("[NoInstantDeath] %1: CRITICAL ERROR: Cannot finalize Kill via HZ fallback.", GetPlayerOrEntityNameStr(owner)); }
			ResetInitiatingKillFlagInternal(owner);
			return;
		}
		
		// PrintFormat("[NoInstantDeath] %1: Calling DamageManager.Kill() with stored instigator: %2", GetPlayerOrEntityNameStr(owner), instigatorToUse); // Production: Verbose
		m_CachedDmgManager.Kill(instigatorToUse); 
	}

	protected void HandleDamageStateChange(EDamageState newDamageState)
	{
		IEntity owner = GetOwner();
		if (!owner || !Replication.IsServer()) return;

		bool wasUnconscious = m_bIsUnconscious;
		bool isNowConscious = (newDamageState == EDamageState.UNDAMAGED || newDamageState == EDamageState.INTERMEDIARY);

		// PrintFormat("[NoInstantDeath] %1: HandleDamageStateChange: NewState=%2, WasUnconscious=%3, IsNowConscious=%4", GetPlayerOrEntityNameStr(owner), typename.EnumToString(EDamageState, newDamageState), wasUnconscious, isNowConscious); // Production: Verbose

		if (wasUnconscious && isNowConscious)
		{
			PrintFormat("[NoInstantDeath] %1: Revived or recovered from unconsciousness. (New state: %2). Stopping bleed-out timer.", GetPlayerOrEntityNameStr(owner), typename.EnumToString(EDamageState, newDamageState));
			m_bIsUnconscious = false;
			m_fUnconsciousTimer = 0.0;
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			Replication.BumpMe();
		}
	}

	bool IsInitiatingKill() { return m_bIsInitiatingKill; }
	
	void ResetInitiatingKillFlagInternal(IEntity owner)
	{
		m_bIsInitiatingKill = false;
		// PrintFormat("[NoInstantDeath] %1: Reset IsInitiatingKill flag.", GetPlayerOrEntityNameStr(owner)); // Production: Verbose
	}

    void ResetInitiatingKillFlag()
    {
        ResetInitiatingKillFlagInternal(GetOwner());
    }

	void OnRep_IsUnconscious() 
	{ 
		IEntity owner = GetOwner(); 
		if (!owner) return; 
		// PrintFormat("[NoInstantDeath] CLIENT %1: Replicated unconscious state: %2", GetPlayerOrEntityNameStr(owner), m_bIsUnconscious); // Production: Verbose
	}

	bool IsUnconscious() { return m_bIsUnconscious; }
	
	override void OnDelete(IEntity owner) 
	{
		if (m_CachedDmgManager)
		{
			m_CachedDmgManager.GetOnDamageStateChanged().Remove(HandleDamageStateChange);
			// PrintFormat("[NoInstantDeath] %1: Unsubscribed from DamageStateChanged.", GetPlayerOrEntityNameStr(owner)); // Production: Verbose
		}
		
		if (Replication.IsServer()) 
		{
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		}

		super.OnDelete(owner);
	}
}
