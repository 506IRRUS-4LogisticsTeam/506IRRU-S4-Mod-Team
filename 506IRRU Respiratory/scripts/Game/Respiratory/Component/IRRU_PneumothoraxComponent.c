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

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
		{
			Print(string.Format("[Pneumothorax] EOnInit - Owner: %1 | Rpl: %2 | DmgMgr: %3 | Stamina: %4 | ResilienceHZ: %5 | IsServer: %6",
				owner,
				m_Rpl,
				m_CachedDmgManager,
				m_CachedStamina,
				m_CachedResilienceHZ,
				Replication.IsServer()));
		}

		// Schedule ownership check - can't determine at init time in MP
		if (Replication.IsRunning())
			GetGame().GetCallqueue().CallLater(IRRU_CheckLocalOwnership, 500, false);
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

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
		{
			Print(string.Format("[Pneumothorax] Ownership check - LocalId: %1 | OwnerId: %2 | IsLocal: %3",
				localPlayerId, ownerPlayerId, m_bIsLocalPlayer));
		}

		// If pneumothorax is already active when we confirm ownership (late join/replication), start client drain
		if (m_bIsLocalPlayer && HasPneumothorax())
		{
			if (!m_bClientDrainActive)
				IRRU_StartClientStaminaDrain();
		}
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
			if (IRRU_PneumothoraxSettings.IsDebugEnabled())
				Print(string.Format("[Pneumothorax] TryTrigger FAILED roll - Roll: %1, Chance: %2", roll, chance));
			return false;
		}

		m_iPneumothoraxStage = IRRU_EPneumothoraxStage.SIMPLE;
		m_fProgressionTimer = 0.0;
		m_fTimeSinceLastBump = 0.0;

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print(string.Format("[Pneumothorax] TryTrigger SUCCESS - Stage set to SIMPLE | Owner: %1", GetOwner()));

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

				if (IRRU_PneumothoraxSettings.IsDebugEnabled())
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
		// Stamina drain: client-side in MP, server-side in SP
		if (!Replication.IsRunning())
		{
			// Singleplayer: server IS the client, drain stamina directly
			if (m_CachedStamina)
			{
				float staminaDrain;
				if (stage == IRRU_EPneumothoraxStage.TENSION)
					staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage2();
				else
					staminaDrain = IRRU_PneumothoraxSettings.GetStaminaDrainStage1();

				m_CachedStamina.AddStamina(-staminaDrain);

				if (IRRU_PneumothoraxSettings.IsDebugEnabled())
					Print(string.Format("[Pneumothorax] SP Stamina drain: %1 | Stamina: %2", staminaDrain, m_CachedStamina.GetStamina()));
			}
		}

		// Resilience drain: always server-side (replicates via hitzone system)
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

				if (IRRU_PneumothoraxSettings.IsDebugEnabled())
					Print(string.Format("[Pneumothorax] Resilience drain - Amount: %1 | Before: %2 | After: %3", drainAmount, healthBefore, m_CachedResilienceHZ.GetHealth()));
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

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print(string.Format("[Pneumothorax] Unconscious resilience drain - Amount: %1 | Before: %2 | After: %3", drainAmount, healthBefore, m_CachedResilienceHZ.GetHealth()));
	}

	//------------------------------------------------------------------------------------------------
	void Treat()
	{
		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		if (!HasPneumothorax())
			return;

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print("[Pneumothorax] Treat() - Clearing pneumothorax");

		ClearPneumothorax();
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearPneumothorax()
	{
		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print(string.Format("[Pneumothorax] ClearPneumothorax - Was stage: %1", m_iPneumothoraxStage));

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
		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print(string.Format("[Pneumothorax] OnPneumothoraxStateChanged (CLIENT) - New stage: %1 | IsLocal: %2", m_iPneumothoraxStage, m_bIsLocalPlayer));

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
		GetGame().GetCallqueue().CallLater(IRRU_UpdateClientStaminaDrain, UPDATE_INTERVAL * 1000, false);

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print("[Pneumothorax] Client stamina drain STARTED");
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_StopClientStaminaDrain()
	{
		m_bClientDrainActive = false;
		GetGame().GetCallqueue().Remove(IRRU_UpdateClientStaminaDrain);

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print("[Pneumothorax] Client stamina drain STOPPED");
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
				// Unconscious - skip stamina drain but keep loop running
				GetGame().GetCallqueue().CallLater(IRRU_UpdateClientStaminaDrain, UPDATE_INTERVAL * 1000, false);
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

			m_CachedStamina.AddStamina(-staminaDrain);

			if (IRRU_PneumothoraxSettings.IsDebugEnabled())
				Print(string.Format("[Pneumothorax] Client stamina drain - Stage: %1 | Drain: %2 | Stamina: %3", m_iPneumothoraxStage, staminaDrain, m_CachedStamina.GetStamina()));
		}

		GetGame().GetCallqueue().CallLater(IRRU_UpdateClientStaminaDrain, UPDATE_INTERVAL * 1000, false);
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
