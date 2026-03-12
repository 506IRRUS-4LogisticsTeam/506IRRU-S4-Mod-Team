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

		Print(string.Format("[Pneumothorax] EOnInit - Owner: %1 | Rpl: %2 | DmgMgr: %3 | Stamina: %4 | ResilienceHZ: %5 | IsServer: %6",
			owner,
			m_Rpl,
			m_CachedDmgManager,
			m_CachedStamina,
			m_CachedResilienceHZ,
			Replication.IsServer()));
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (Replication.IsServer())
			GetGame().GetCallqueue().Remove(UpdateProgression);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
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
			Print(string.Format("[Pneumothorax] TryTrigger FAILED roll - Roll: %1, Chance: %2", roll, chance));
			return false;
		}

		m_iPneumothoraxStage = IRRU_EPneumothoraxStage.SIMPLE;
		m_fProgressionTimer = 0.0;
		m_fTimeSinceLastBump = 0.0;

		Print(string.Format("[Pneumothorax] TryTrigger SUCCESS - Stage set to SIMPLE | Owner: %1 | Rpl: %2", GetOwner(), m_Rpl));

		if (m_Rpl)
			Replication.BumpMe();

		GetGame().GetCallqueue().Remove(UpdateProgression);
		GetGame().GetCallqueue().CallLater(UpdateProgression, UPDATE_INTERVAL * 1000, false);

		Print("[Pneumothorax] TryTrigger - UpdateProgression scheduled");

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateProgression()
	{
		Print(string.Format("[Pneumothorax] UpdateProgression CALLED - IsServer: %1 | IsRunning: %2 | Stage: %3 | Timer: %4",
			Replication.IsServer(), Replication.IsRunning(), m_iPneumothoraxStage, m_fProgressionTimer));

		if (!Replication.IsServer() && Replication.IsRunning())
		{
			Print("[Pneumothorax] UpdateProgression SKIPPED - Not server in MP");
			return;
		}

		if (!HasPneumothorax())
		{
			Print("[Pneumothorax] UpdateProgression SKIPPED - No pneumothorax");
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
		{
			Print("[Pneumothorax] UpdateProgression SKIPPED - No owner");
			return;
		}

		SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(
			owner.FindComponent(SCR_CharacterControllerComponent));
		if (ctrl && ctrl.GetLifeState() == ECharacterLifeState.DEAD)
		{
			Print("[Pneumothorax] UpdateProgression - Character DEAD, clearing");
			ClearPneumothorax();
			return;
		}

		bool isConscious = true;
		if (ctrl)
			isConscious = (ctrl.GetLifeState() == ECharacterLifeState.ALIVE);

		int currentStage = m_iPneumothoraxStage;

		Print(string.Format("[Pneumothorax] UpdateProgression - Stage: %1 | Conscious: %2 | LifeState: %3",
			currentStage, isConscious, ctrl.GetLifeState()));

		if (currentStage == IRRU_EPneumothoraxStage.SIMPLE)
		{
			m_fProgressionTimer += UPDATE_INTERVAL;

			float progressionTime = IRRU_PneumothoraxSettings.GetProgressionTime();
			if (m_fProgressionTimer >= progressionTime)
			{
				m_iPneumothoraxStage = IRRU_EPneumothoraxStage.TENSION;
				Print(string.Format("[Pneumothorax] PROGRESSED to TENSION at %1s (threshold: %2s)", m_fProgressionTimer, progressionTime));

				if (m_Rpl)
					Replication.BumpMe();
			}
		}

		if (isConscious)
			ApplyConsciousEffects(currentStage);
		else
			ApplyUnconsciousEffects(currentStage);

		m_fTimeSinceLastBump += UPDATE_INTERVAL;
		if (m_fTimeSinceLastBump >= REPLICATION_BUMP_INTERVAL)
		{
			m_fTimeSinceLastBump = 0.0;
			if (m_Rpl)
				Replication.BumpMe();
		}

		GetGame().GetCallqueue().CallLater(UpdateProgression, UPDATE_INTERVAL * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyConsciousEffects(int stage)
	{
		Print(string.Format("[Pneumothorax] ApplyConsciousEffects - Stage: %1 | HasStamina: %2 | HasResilienceHZ: %3",
			stage, m_CachedStamina != null, m_CachedResilienceHZ != null));

		if (m_CachedStamina)
		{
			float staminaDrain;
			if (stage == IRRU_EPneumothoraxStage.TENSION)
				staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage2();
			else
				staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage1();

			Print(string.Format("[Pneumothorax] Stamina BEFORE drain: (draining %1)", staminaDrain));
			m_CachedStamina.AddStamina(-staminaDrain);
			Print("[Pneumothorax] Stamina drain applied");
		}
		else
		{
			Print("[Pneumothorax] WARNING: m_CachedStamina is NULL - cannot drain stamina!");
		}

		if (stage == IRRU_EPneumothoraxStage.TENSION)
		{
			if (m_CachedResilienceHZ)
			{
				float drainRate = IRRU_PneumothoraxSettings.GetResilienceDrainRate();
				float drainAmount = drainRate * UPDATE_INTERVAL;
				float healthBefore = m_CachedResilienceHZ.GetHealth();
				m_CachedResilienceHZ.HandleDamage(drainAmount, EDamageType.TRUE, null);
				float healthAfter = m_CachedResilienceHZ.GetHealth();
				Print(string.Format("[Pneumothorax] Resilience drain - Rate: %1 | Amount: %2 | Before: %3 | After: %4",
					drainRate, drainAmount, healthBefore, healthAfter));
			}
			else
			{
				Print("[Pneumothorax] WARNING: m_CachedResilienceHZ is NULL - cannot drain resilience!");
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyUnconsciousEffects(int stage)
	{
		Print(string.Format("[Pneumothorax] ApplyUnconsciousEffects - Stage: %1 | HasResilienceHZ: %2", stage, m_CachedResilienceHZ != null));

		if (!m_CachedResilienceHZ)
		{
			Print("[Pneumothorax] WARNING: m_CachedResilienceHZ is NULL in unconscious effects!");
			return;
		}

		float drainRate = IRRU_PneumothoraxSettings.GetResilienceDrainRate() * 0.5;
		float drainAmount = drainRate * UPDATE_INTERVAL;
		float healthBefore = m_CachedResilienceHZ.GetHealth();
		m_CachedResilienceHZ.HandleDamage(drainAmount, EDamageType.TRUE, null);
		float healthAfter = m_CachedResilienceHZ.GetHealth();
		Print(string.Format("[Pneumothorax] Unconscious resilience drain - Amount: %1 | Before: %2 | After: %3",
			drainAmount, healthBefore, healthAfter));
	}

	//------------------------------------------------------------------------------------------------
	void Treat()
	{
		Print(string.Format("[Pneumothorax] Treat() called - IsServer: %1 | IsRunning: %2 | HasPneumo: %3",
			Replication.IsServer(), Replication.IsRunning(), HasPneumothorax()));

		if (!Replication.IsServer() && Replication.IsRunning())
		{
			Print("[Pneumothorax] Treat() SKIPPED - Not server in MP");
			return;
		}

		if (!HasPneumothorax())
		{
			Print("[Pneumothorax] Treat() SKIPPED - No pneumothorax to treat");
			return;
		}

		Print("[Pneumothorax] Treat() - Clearing pneumothorax");
		ClearPneumothorax();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearPneumothorax()
	{
		Print(string.Format("[Pneumothorax] ClearPneumothorax - Was stage: %1", m_iPneumothoraxStage));

		m_iPneumothoraxStage = IRRU_EPneumothoraxStage.NONE;
		m_fProgressionTimer = 0.0;
		m_fTimeSinceLastBump = 0.0;

		GetGame().GetCallqueue().Remove(UpdateProgression);

		if (m_Rpl)
			Replication.BumpMe();

		Print("[Pneumothorax] ClearPneumothorax - DONE, stage reset to NONE");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPneumothoraxStateChanged()
	{
		Print(string.Format("[Pneumothorax] OnPneumothoraxStateChanged (CLIENT) - New stage: %1 | Timer: %2",
			m_iPneumothoraxStage, m_fProgressionTimer));
	}

	//------------------------------------------------------------------------------------------------
	bool HasPneumothorax()
	{
		return m_iPneumothoraxStage != IRRU_EPneumothoraxStage.NONE;
	}

	//------------------------------------------------------------------------------------------------
	int GetStage()
	{
		return m_iPneumothoraxStage;
	}

	//------------------------------------------------------------------------------------------------
	float GetProgressionTimer()
	{
		return m_fProgressionTimer;
	}

	//------------------------------------------------------------------------------------------------
	float GetProgressionPercentage()
	{
		if (m_iPneumothoraxStage != IRRU_EPneumothoraxStage.SIMPLE)
			return 0.0;

		float progressionTime = IRRU_PneumothoraxSettings.GetProgressionTime();
		if (progressionTime <= 0)
			return 100.0;

		return (m_fProgressionTimer / progressionTime) * 100.0;
	}

}
