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
	protected const float STAMINA_UPDATE_INTERVAL = 0.1;
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
	protected bool m_bClientDrainActive = false;
	protected bool m_bIsLocalPlayer = false;
	protected bool m_bOwnershipChecked = false;

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

		if (Replication.IsRunning())
			GetGame().GetCallqueue().CallLater(IRRU_CheckLocalOwnership, 500, false);
		else
		{
			m_bIsLocalPlayer = true;
			m_bOwnershipChecked = true;
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(UpdateProgression);
		GetGame().GetCallqueue().Remove(IRRU_UpdateClientStaminaDrain);
		GetGame().GetCallqueue().Remove(IRRU_CheckLocalOwnership);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_CheckLocalOwnership()
	{
		if (m_bOwnershipChecked)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			// Retry once if player controller not ready yet
			GetGame().GetCallqueue().CallLater(IRRU_CheckLocalOwnership, 500, false);
			return;
		}

		int localPlayerId = pc.GetPlayerId();
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		int ownerPlayerId = pm.GetPlayerIdFromControlledEntity(owner);

		if (ownerPlayerId <= 0)
		{
			// Entity not yet assigned to a player, retry once
			GetGame().GetCallqueue().CallLater(IRRU_CheckLocalOwnership, 500, false);
			return;
		}

		m_bOwnershipChecked = true;
		m_bIsLocalPlayer = (localPlayerId == ownerPlayerId);

		// If pneumothorax is already active when we confirm ownership (late join/replication), start client drain
		if (m_bIsLocalPlayer && HasPneumothorax())
		{
			if (!m_bClientDrainActive)
				IRRU_StartClientStaminaDrain();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Try to trigger pneumothorax based on chest damage ratio (0-1 of hitzone max health)
	//! Damage below the minimum threshold is ignored.
	//! Chance scales linearly from MinTriggerChance at threshold to MaxTriggerChance at 100% damage.
	bool TryTriggerPneumothorax(float damageRatio = 0.0)
	{
		if (!Replication.IsServer() && Replication.IsRunning())
			return false;

		if (HasPneumothorax())
			return false;

		// Gate: damage must exceed minimum threshold
		float minThreshold = IRRU_PneumothoraxSettings.GetMinDamageThreshold();
		if (damageRatio < minThreshold)
			return false;

		// Scale chance linearly from min to max based on damage ratio
		float minChance = IRRU_PneumothoraxSettings.GetMinTriggerChance();
		float maxChance = IRRU_PneumothoraxSettings.GetMaxTriggerChance();

		float scaleFactor = 0.0;
		if (1.0 - minThreshold > 0)
			scaleFactor = (damageRatio - minThreshold) / (1.0 - minThreshold);
		if (scaleFactor > 1.0)
			scaleFactor = 1.0;

		float chance = minChance + scaleFactor * (maxChance - minChance);

		float roll = Math.RandomFloat(0.0, 1.0);

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print(string.Format("[Pneumothorax] DamageRatio: %1, Chance: %2, Roll: %3", damageRatio, chance, roll));

		if (roll > chance)
			return false;

		m_iPneumothoraxStage = IRRU_EPneumothoraxStage.SIMPLE;
		m_fProgressionTimer = 0.0;
		m_fTimeSinceLastBump = 0.0;

		if (m_Rpl)
			Replication.BumpMe();

		GetGame().GetCallqueue().Remove(UpdateProgression);
		GetGame().GetCallqueue().CallLater(UpdateProgression, UPDATE_INTERVAL * 1000, false);

		if (!Replication.IsRunning() && !m_bClientDrainActive)
			IRRU_StartClientStaminaDrain();

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
		if (stage == IRRU_EPneumothoraxStage.TENSION)
		{
			if (m_CachedResilienceHZ)
			{
				float drainRate = IRRU_PneumothoraxSettings.GetResilienceDrainRate();
				float drainAmount = drainRate * UPDATE_INTERVAL;
				float healthBefore = m_CachedResilienceHZ.GetHealth();
				float newHealth = healthBefore - drainAmount;
				if (newHealth < 0)
					newHealth = 0;
				m_CachedResilienceHZ.SetHealth(newHealth);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyUnconsciousEffects(int stage)
	{
		if (!m_CachedResilienceHZ)
			return;

		float drainRate = IRRU_PneumothoraxSettings.GetResilienceDrainRate() * 0.5;
		float drainAmount = drainRate * UPDATE_INTERVAL;
		float healthBefore = m_CachedResilienceHZ.GetHealth();
		float newHealth = healthBefore - drainAmount;
		if (newHealth < 0)
			newHealth = 0;
		m_CachedResilienceHZ.SetHealth(newHealth);
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
		IRRU_StopClientStaminaDrain();

		if (m_Rpl)
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPneumothoraxStateChanged()
	{
		if (!m_bIsLocalPlayer || !Replication.IsRunning())
			return;

		if (HasPneumothorax())
		{
			if (!m_bClientDrainActive)
				IRRU_StartClientStaminaDrain();
		}
		else
		{
			IRRU_StopClientStaminaDrain();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_StartClientStaminaDrain()
	{
		if (m_bClientDrainActive)
			return;

		m_bClientDrainActive = true;
		GetGame().GetCallqueue().Remove(IRRU_UpdateClientStaminaDrain);
		GetGame().GetCallqueue().CallLater(IRRU_UpdateClientStaminaDrain, STAMINA_UPDATE_INTERVAL * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_StopClientStaminaDrain()
	{
		m_bClientDrainActive = false;
		GetGame().GetCallqueue().Remove(IRRU_UpdateClientStaminaDrain);
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_UpdateClientStaminaDrain()
	{
		if (!m_bClientDrainActive || !m_bIsLocalPlayer)
			return;

		if (!HasPneumothorax())
		{
			IRRU_StopClientStaminaDrain();
			return;
		}

		IEntity owner = GetOwner();
		if (!owner)
			return;

		// Skip stamina drain if dead or unconscious
		SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(
			owner.FindComponent(SCR_CharacterControllerComponent));
		if (ctrl)
		{
			int lifeState = ctrl.GetLifeState();
			if (lifeState == ECharacterLifeState.DEAD)
			{
				IRRU_StopClientStaminaDrain();
				return;
			}
			if (lifeState != ECharacterLifeState.ALIVE)
			{
				GetGame().GetCallqueue().CallLater(IRRU_UpdateClientStaminaDrain, STAMINA_UPDATE_INTERVAL * 1000, false);
				return;
			}
		}

		if (m_CachedStamina)
		{
			float staminaDrain;
			if (m_iPneumothoraxStage == IRRU_EPneumothoraxStage.TENSION)
				staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage2();
			else
				staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage1();

			m_CachedStamina.AddStamina(-staminaDrain * STAMINA_UPDATE_INTERVAL);
		}

		GetGame().GetCallqueue().CallLater(IRRU_UpdateClientStaminaDrain, STAMINA_UPDATE_INTERVAL * 1000, false);
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
