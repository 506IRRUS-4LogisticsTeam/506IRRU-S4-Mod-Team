[ComponentEditorProps(category: "Health", description: "Overrides death to force player bleed-out")]
class IRRU_NoInstantDeathComponentClass : ScriptComponentClass
{
}

class IRRU_NoInstantDeathComponent : ScriptComponent
{
	protected const float CHECK_INTERVAL = 1.0;
	protected const float PERIODIC_LOG_INTERVAL = 30.0;

	protected bool m_bNID_Initialized = false;
	[RplProp(onRplName: "OnUnconsciousStateChanged")]
	protected bool m_bIsUnconscious = false;
	[RplProp(onRplName: "OnCPRStateChanged")]
	protected bool m_bReceivingCPR = false;
	[RplProp()]
	protected float m_fCPRCooldownTimer = 0.0;
	[RplProp()]
	protected float m_fUnconsciousTimer = 0.0;

	protected bool m_bDeadBlockPrinted = false;
	protected bool m_bDeadWarned = false;

	//! Captured at unconscious time and held through the bleedout window. Read cross-module by the
	//! IRRU Ticket System hook (modded IRRU_NoInstantDeathComponent) to attribute casualties. Do not remove.
	protected Instigator m_LastKnownInstigator;
	protected RplComponent m_Rpl;
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	protected SCR_CharacterControllerComponent m_Ctrl;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_Ctrl = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));

		if (m_Ctrl)
			m_Ctrl.m_OnLifeStateChanged.Insert(OnLifeStateChanged);

		static bool s_bVersionLogged = false;
		if (!s_bVersionLogged && IRRU_NoInstantDeathSettings.IsDebugEnabled())
		{
			Print(string.Format("[NoInstantDeath] Medical Mod v%1 loaded", IRRU_NoInstantDeathSettings.MOD_VERSION));
			s_bVersionLogged = true;
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (m_Ctrl)
			m_Ctrl.m_OnLifeStateChanged.Remove(OnLifeStateChanged);

		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			GetGame().GetCallqueue().Remove(UpdateCPRCooldownTimer);
		}

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Replicate this component's RplProps to clients (no-op when not networked).
	protected void IRRU_BumpReplication()
	{
		if (m_Rpl)
			Replication.BumpMe();
	}

	bool IsInitialized()
	{
		return m_bNID_Initialized;
	}

	void Initialize()
	{
		if (m_bNID_Initialized)
			return;

		if (!m_CachedDmgManager)
		{
			Print("[NoInstantDeath] Initialize failed - no damage manager", LogLevel.ERROR);
			return;
		}

		m_bNID_Initialized = true;

		IRRU_BumpReplication();

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] %1: initialized with %2s bleedout timer", GetNameStr(GetOwner()), IRRU_NoInstantDeathSettings.GetBleedoutTime()));
	}

	protected void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		if (newLifeState != ECharacterLifeState.INCAPACITATED || previousLifeState != ECharacterLifeState.ALIVE || !m_bNID_Initialized || m_bIsUnconscious)
			return;

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled() && m_CachedDmgManager)
		{
			Print(string.Format("[NoInstantDeath] %1: unconscious - Health: %2%%, Blood: %3%%, Resilience: %4%%",
				GetNameStr(GetOwner()), m_CachedDmgManager.IRRU_GetHealthPercentage(), m_CachedDmgManager.IRRU_GetBloodPercentage(), m_CachedDmgManager.IRRU_GetResiliencePercentage()));
		}

		MakeUnconscious(GetOwner());
	}

	void MakeUnconscious(IEntity owner)
	{
		if (!m_bNID_Initialized || m_bIsUnconscious || !m_CachedDmgManager)
			return;

		m_bIsUnconscious = true;
		m_bDeadBlockPrinted = false;
		m_bDeadWarned = false;
		m_fUnconsciousTimer = 0.0;
		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();
		m_CachedDmgManager.ForceUnconsciousness();

		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			IRRU_BumpReplication();
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
		}

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] %1: entering unconscious state (%2s timer)", GetNameStr(owner), IRRU_NoInstantDeathSettings.GetBleedoutTime()));
	}

	protected void UpdateUnconsciousTimer()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_bIsUnconscious || !Replication.IsServer())
			return;

		if (m_Ctrl && m_Ctrl.GetLifeState() == ECharacterLifeState.ALIVE)
		{
			StopBleedoutTimer("revived");
			return;
		}

		if (m_Ctrl && m_Ctrl.GetLifeState() == ECharacterLifeState.DEAD)
		{
			if (!m_bDeadBlockPrinted)
			{
				if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
					Print(string.Format("[NoInstantDeath] %1: preventing premature DEAD state during bleedout", GetNameStr(owner)));
				m_bDeadBlockPrinted = true;
			}

			m_CachedDmgManager.ForceUnconsciousness();

			if (m_Ctrl.GetLifeState() == ECharacterLifeState.DEAD && !m_bDeadWarned)
			{
				Print(string.Format("[NoInstantDeath] WARNING: %1 reached DEAD state before timer expiry", GetNameStr(owner)), LogLevel.WARNING);
				m_bDeadWarned = true;
				StopBleedoutTimer("forced death");
				return;
			}
		}

		bool healthStable = false;
		// bool hasEpinephrine = false;
		if (m_CachedDmgManager)
		{
			healthStable = (m_CachedDmgManager.IRRU_GetHealthPercentage() > 33.0 && m_CachedDmgManager.IRRU_GetBloodPercentage() > 33.0);
			// HitZone resilienceHZ = m_CachedDmgManager.GetResilienceHitZone();
			// if (resilienceHZ)
			// 	hasEpinephrine = (m_CachedDmgManager.FindDamageEffectOnHitZone(ACE_Medical_EpinephrineDamageEffect, resilienceHZ) != null);
		}

		if (!m_bReceivingCPR && !healthStable /* && !hasEpinephrine */)
			m_fUnconsciousTimer += CHECK_INTERVAL;

		IRRU_BumpReplication();

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled() && Math.Mod(m_fUnconsciousTimer, PERIODIC_LOG_INTERVAL) < CHECK_INTERVAL)
		{
			float bleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
			string pauseReason = "";
			if (m_bReceivingCPR)
				pauseReason = " (paused: CPR)";
			else if (healthStable)
				pauseReason = " (paused: vitals stable)";
			// else if (hasEpinephrine)
			// 	pauseReason = " (paused: epinephrine)";

			Print(string.Format("[NoInstantDeath] %1: bleedout %2/%3s remaining%4 - Health: %5%%, Blood: %6%%, Resilience: %7%%",
				GetNameStr(owner), (bleedoutTime - m_fUnconsciousTimer), bleedoutTime, pauseReason,
				m_CachedDmgManager.IRRU_GetHealthPercentage(), m_CachedDmgManager.IRRU_GetBloodPercentage(), m_CachedDmgManager.IRRU_GetResiliencePercentage()));
		}

		float bleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
		if (m_fUnconsciousTimer >= bleedoutTime)
		{
			if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
				Print(string.Format("[NoInstantDeath] %1: bleedout timer expired", GetNameStr(owner)));
			KillCharacter(owner);
			return;
		}

		GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
	}

	void KillCharacter(IEntity owner)
	{
		if (!m_bIsUnconscious || !m_CachedDmgManager)
			return;

		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

		HitZone hz = m_CachedDmgManager.GetDefaultHitZone();
		if (hz)
			hz.SetHealth(0);
	}

	protected void StopBleedoutTimer(string reason)
	{
		if (!m_bIsUnconscious)
			return;

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] %1: bleedout cancelled (%2)", GetNameStr(GetOwner()), reason));

		m_bIsUnconscious = false;
		m_fUnconsciousTimer = 0.0;
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

		if (Replication.IsServer())
			IRRU_BumpReplication();
	}

	protected void OnUnconsciousStateChanged() {}
	protected void OnCPRStateChanged() {}

	string GetNameStr(IEntity e)
	{
		if (!e)
			return "UnknownEntity";

		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(e);
			if (ch)
			{
				int pid = pm.GetPlayerIdFromControlledEntity(ch);
				if (pid > 0)
				{
					string n = pm.GetPlayerName(pid);
					if (!n.IsEmpty())
						return n;
				}
			}
		}
		return e.ToString();
	}

	float GetBleedoutTimeRemaining()
	{
		if (!m_bIsUnconscious)
			return -1.0;
		return IRRU_NoInstantDeathSettings.GetBleedoutTime() - m_fUnconsciousTimer;
	}

	float GetBleedoutTimeTotal()
	{
		return IRRU_NoInstantDeathSettings.GetBleedoutTime();
	}

	bool IsUnconscious() { return m_bIsUnconscious; }
	bool IsReceivingCPR() { return m_bReceivingCPR; }
	bool IsOnCPRCooldown() { return m_fCPRCooldownTimer > 0; }
	float GetCPRCooldownRemaining() { return m_fCPRCooldownTimer; }

	void SetReceivingCPR(bool receiving)
	{
		if (m_bReceivingCPR == receiving)
			return;

		m_bReceivingCPR = receiving;

		IRRU_BumpReplication();

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
		{
			string state = "stopped";
			if (receiving)
				state = "started";
			Print(string.Format("[NoInstantDeath] %1: CPR %2", GetNameStr(GetOwner()), state));
		}
	}

	void SetCPRCooldown(float cooldownDuration)
	{
		if (cooldownDuration < 0)
		{
			Print(string.Format("[NoInstantDeath] Invalid CPR cooldown: %1, clamping to 0", cooldownDuration), LogLevel.WARNING);
			cooldownDuration = 0;
		}

		m_fCPRCooldownTimer = cooldownDuration;

		IRRU_BumpReplication();

		if (cooldownDuration > 0 && Replication.IsServer())
		{
			GetGame().GetCallqueue().Remove(UpdateCPRCooldownTimer);
			GetGame().GetCallqueue().CallLater(UpdateCPRCooldownTimer, 1000, true);
		}
	}

	protected void UpdateCPRCooldownTimer()
	{
		m_fCPRCooldownTimer -= 1.0;

		if (m_fCPRCooldownTimer <= 0)
		{
			m_fCPRCooldownTimer = 0;
			GetGame().GetCallqueue().Remove(UpdateCPRCooldownTimer);
		}

		IRRU_BumpReplication();
	}
}
