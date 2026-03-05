//! Pneumothorax (collapsed lung) injury component
//! Triggers on chest damage with configurable chance, progresses through two stages

enum IRRU_EPneumothoraxStage
{
	NONE,
	SIMPLE,
	TENSION
}

[ComponentEditorProps(category: "Health",
	description: "Pneumothorax (collapsed lung) injury from chest trauma")]
class IRRU_PneumothoraxComponentClass : ScriptComponentClass
{
}

//! Manages pneumothorax state, progression, and effects
class IRRU_PneumothoraxComponent : ScriptComponent
{
	protected const float UPDATE_INTERVAL = 1.0;
	protected const float REPLICATION_BUMP_INTERVAL = 2.0;

	[RplProp(onRplName: "OnPneumothoraxStateChanged")]
	protected int m_iPneumothoraxStage = 0;

	[RplProp()]
	protected float m_fProgressionTimer = 0.0;

	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	protected CharacterStaminaComponent m_CachedStamina;
	protected SCR_CharacterResilienceHitZone m_CachedResilienceHZ;
	protected RplComponent m_Rpl;
	protected float m_fTimeSinceLastBump = 0.0;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(
			owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_CachedStamina = CharacterStaminaComponent.Cast(
			owner.FindComponent(CharacterStaminaComponent));

		if (m_CachedDmgManager)
			m_CachedResilienceHZ = m_CachedDmgManager.GetResilienceHitZone();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (Replication.IsServer())
			GetGame().GetCallqueue().Remove(UpdateProgression);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Attempt to trigger pneumothorax from chest damage
	//! Called from SCR_CharacterDamageManagerComponent.OnDamage when chest is hit
	//! \return true if pneumothorax was triggered
	bool TryTriggerPneumothorax()
	{
		if (!Replication.IsServer() && Replication.IsRunning())
			return false;

		if (HasPneumothorax())
			return false;

		float chance = IRRU_PneumothoraxSettings.GetTriggerChance();
		float roll = Math.RandomFloat(0.0, 1.0);

		if (roll > chance)
		{
			if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			{
				Print(string.Format("[Pneumothorax] %1: chest hit, roll %2 > %3, no pneumothorax",
					GetNameStr(), roll, chance));
			}
			return false;
		}

		m_iPneumothoraxStage = IRRU_EPneumothoraxStage.SIMPLE;
		m_fProgressionTimer = 0.0;
		m_fTimeSinceLastBump = 0.0;

		if (m_Rpl)
			Replication.BumpMe();

		GetGame().GetCallqueue().Remove(UpdateProgression);
		GetGame().GetCallqueue().CallLater(UpdateProgression, UPDATE_INTERVAL * 1000, false);

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
		{
			Print(string.Format("[Pneumothorax] %1: TRIGGERED (roll %2 <= %3) - Stage 1 started, progression in %4s",
				GetNameStr(), roll, chance, IRRU_PneumothoraxSettings.GetProgressionTime()));
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side update loop for progression and effects
	protected void UpdateProgression()
	{
		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		if (!HasPneumothorax())
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		// Check if character is dead - stop processing
		SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(
			owner.FindComponent(SCR_CharacterControllerComponent));
		if (ctrl && ctrl.GetLifeState() == ECharacterLifeState.DEAD)
		{
			ClearPneumothorax();
			return;
		}

		bool isConscious = true;
		if (ctrl)
			isConscious = (ctrl.GetLifeState() == ECharacterLifeState.ALIVE);

		int currentStage = m_iPneumothoraxStage;

		// Stage 1: progress timer toward Stage 2
		if (currentStage == IRRU_EPneumothoraxStage.SIMPLE)
		{
			m_fProgressionTimer += UPDATE_INTERVAL;

			float progressionTime = IRRU_PneumothoraxSettings.GetProgressionTime();
			if (m_fProgressionTimer >= progressionTime)
			{
				m_iPneumothoraxStage = IRRU_EPneumothoraxStage.TENSION;

				if (m_Rpl)
					Replication.BumpMe();

				if (IRRU_PneumothoraxSettings.IsDebugEnabled())
				{
					Print(string.Format("[Pneumothorax] %1: progressed to TENSION (Stage 2) after %2s",
						GetNameStr(), m_fProgressionTimer));
				}
			}
		}

		// Apply effects based on current stage
		if (isConscious)
			ApplyConsciousEffects(currentStage);

		// Periodic replication bump
		m_fTimeSinceLastBump += UPDATE_INTERVAL;
		if (m_fTimeSinceLastBump >= REPLICATION_BUMP_INTERVAL)
		{
			m_fTimeSinceLastBump = 0.0;
			if (m_Rpl)
				Replication.BumpMe();
		}

		// Reschedule
		GetGame().GetCallqueue().CallLater(UpdateProgression, UPDATE_INTERVAL * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Apply effects while the character is conscious
	protected void ApplyConsciousEffects(int stage)
	{
		// Stamina drain (both stages, worse in Stage 2)
		if (m_CachedStamina)
		{
			float staminaDrain;
			if (stage == IRRU_EPneumothoraxStage.TENSION)
				staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage2();
			else
				staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage1();

			m_CachedStamina.AddStamina(-staminaDrain);
		}

		// Resilience drain (Stage 2 only)
		if (stage == IRRU_EPneumothoraxStage.TENSION && m_CachedResilienceHZ)
		{
			float drainRate = IRRU_PneumothoraxSettings.GetResilienceDrainRate();
			m_CachedResilienceHZ.HandleDamage(drainRate * UPDATE_INTERVAL, EDamageType.TRUE, null);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Treat pneumothorax - called from needle decompression action
	void Treat()
	{
		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		if (!HasPneumothorax())
			return;

		int previousStage = m_iPneumothoraxStage;
		ClearPneumothorax();

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
		{
			string stageStr;
			if (previousStage == IRRU_EPneumothoraxStage.TENSION)
				stageStr = "TENSION";
			else
				stageStr = "SIMPLE";

			Print(string.Format("[Pneumothorax] %1: TREATED (was %2)", GetNameStr(), stageStr));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Clear all pneumothorax state
	protected void ClearPneumothorax()
	{
		m_iPneumothoraxStage = IRRU_EPneumothoraxStage.NONE;
		m_fProgressionTimer = 0.0;
		m_fTimeSinceLastBump = 0.0;

		GetGame().GetCallqueue().Remove(UpdateProgression);

		if (m_Rpl)
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Callback when pneumothorax state replicates to clients
	protected void OnPneumothoraxStateChanged()
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Whether this character currently has pneumothorax
	bool HasPneumothorax()
	{
		return m_iPneumothoraxStage != IRRU_EPneumothoraxStage.NONE;
	}

	//------------------------------------------------------------------------------------------------
	//! Get current pneumothorax stage
	int GetStage()
	{
		return m_iPneumothoraxStage;
	}

	//------------------------------------------------------------------------------------------------
	//! Get time elapsed in current stage
	float GetProgressionTimer()
	{
		return m_fProgressionTimer;
	}

	//------------------------------------------------------------------------------------------------
	//! Get progression percentage (0-100) from Stage 1 to Stage 2
	float GetProgressionPercentage()
	{
		if (m_iPneumothoraxStage != IRRU_EPneumothoraxStage.SIMPLE)
			return 0.0;

		float progressionTime = IRRU_PneumothoraxSettings.GetProgressionTime();
		if (progressionTime <= 0)
			return 100.0;

		return (m_fProgressionTimer / progressionTime) * 100.0;
	}

	//------------------------------------------------------------------------------------------------
	//! Get player/entity name for debug logging
	protected string GetNameStr()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return "UnknownEntity";

		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(
			owner.FindComponent(IRRU_NoInstantDeathComponent));
		if (nid)
			return nid.GetNameStr(owner);

		return owner.ToString();
	}
}
