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
			return false;

		m_iPneumothoraxStage = IRRU_EPneumothoraxStage.SIMPLE;
		m_fProgressionTimer = 0.0;
		m_fTimeSinceLastBump = 0.0;

		if (m_Rpl)
			Replication.BumpMe();

		GetGame().GetCallqueue().Remove(UpdateProgression);
		GetGame().GetCallqueue().CallLater(UpdateProgression, UPDATE_INTERVAL * 1000, false);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateProgression()
	{
		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		if (!HasPneumothorax())
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

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

		if (currentStage == IRRU_EPneumothoraxStage.SIMPLE)
		{
			m_fProgressionTimer += UPDATE_INTERVAL;

			float progressionTime = IRRU_PneumothoraxSettings.GetProgressionTime();
			if (m_fProgressionTimer >= progressionTime)
			{
				m_iPneumothoraxStage = IRRU_EPneumothoraxStage.TENSION;

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
		if (m_CachedStamina)
		{
			float staminaDrain;
			if (stage == IRRU_EPneumothoraxStage.TENSION)
				staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage2();
			else
				staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage1();

			m_CachedStamina.AddStamina(-staminaDrain);
		}

		if (stage == IRRU_EPneumothoraxStage.TENSION && m_CachedResilienceHZ)
		{
			float drainRate = IRRU_PneumothoraxSettings.GetResilienceDrainRate();
			float drainAmount = drainRate * UPDATE_INTERVAL;
			m_CachedResilienceHZ.HandleDamage(drainAmount, EDamageType.TRUE, null);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyUnconsciousEffects(int stage)
	{
		if (!m_CachedResilienceHZ)
			return;

		float drainRate = IRRU_PneumothoraxSettings.GetResilienceDrainRate() * 0.5;
		float drainAmount = drainRate * UPDATE_INTERVAL;
		m_CachedResilienceHZ.HandleDamage(drainAmount, EDamageType.TRUE, null);
	}

	//------------------------------------------------------------------------------------------------
	void Treat()
	{
		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		if (!HasPneumothorax())
			return;

		ClearPneumothorax();
	}

	//------------------------------------------------------------------------------------------------
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
	protected void OnPneumothoraxStateChanged()
	{
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
