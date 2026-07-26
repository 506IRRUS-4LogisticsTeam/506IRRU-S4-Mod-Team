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

	protected IRRU_CPRHelperCompartment m_pActiveCPRHelper;

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
		if (newLifeState == ECharacterLifeState.INCAPACITATED && previousLifeState == ECharacterLifeState.ALIVE && (Replication.IsServer() || !Replication.IsRunning()))
			IRRU_StowWeaponOnUnconscious();

		if (newLifeState != ECharacterLifeState.INCAPACITATED || previousLifeState != ECharacterLifeState.ALIVE || !m_bNID_Initialized || m_bIsUnconscious)
			return;

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled() && m_CachedDmgManager)
		{
			Print(string.Format("[NoInstantDeath] %1: unconscious - Health: %2%%, Blood: %3%%, Resilience: %4%%",
				GetNameStr(GetOwner()), m_CachedDmgManager.IRRU_GetHealthPercentage(), m_CachedDmgManager.IRRU_GetBloodPercentage(), m_CachedDmgManager.IRRU_GetResiliencePercentage()));
		}

		MakeUnconscious(GetOwner());
	}

	//------------------------------------------------------------------------------------------------
	//! Weapon stow on unconsciousness adapted from "Keep Gun When Uncon" by ceo_of_bacon (published
	//! on the Workshop originally as bacon8008), original implementation by R34P3R. Used and modified under the
	//! Arma Public License (APL), with ceo_of_bacon's knowledge (2026-07-19).
	//! https://reforger.armaplatform.com/workshop/6088A3044B7ECBFD-KeepGunWhenUncon
	//! https://www.bohemia.net/community/licenses/arma-public-license
	protected void IRRU_StowWeaponOnUnconscious()
	{
		if (!m_Ctrl)
			return;

		BaseWeaponManagerComponent weaponManager = m_Ctrl.GetWeaponManagerComponent();
		if (!weaponManager)
			return;

		BaseWeaponComponent currentWeapon = weaponManager.GetCurrentWeapon();
		if (!currentWeapon || !currentWeapon.GetOwner())
			return;

		m_Ctrl.TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedContextual, true);
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

		if (Replication.IsServer() || !Replication.IsRunning())
			IRRU_LogBlueOnBlue(owner);

		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			IRRU_BumpReplication();
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
		}

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] %1: entering unconscious state (%2s timer)", GetNameStr(owner), IRRU_NoInstantDeathSettings.GetBleedoutTime()));
	}

	protected void IRRU_LogBlueOnBlue(IEntity owner)
	{
		if (!m_LastKnownInstigator)
		{
			IRRU_LogBlueOnBlueSkip(owner, "no instigator recorded");
			return;
		}

		SCR_ECharacterControlType victimType = SCR_CharacterHelper.GetCharacterControlType(owner);
		if (victimType == SCR_ECharacterControlType.AI || victimType == SCR_ECharacterControlType.POSSESSED_AI || victimType == SCR_ECharacterControlType.UNKNOWN)
		{
			IRRU_LogBlueOnBlueSkip(owner, string.Format("victim is not a player (control type %1)", typename.EnumToString(SCR_ECharacterControlType, victimType)));
			return;
		}

		int attackerPlayerId = m_LastKnownInstigator.GetInstigatorPlayerID();
		IEntity attacker = m_LastKnownInstigator.GetInstigatorEntity();

		if (attackerPlayerId <= 0 && !attacker)
		{
			IRRU_LogBlueOnBlueSkip(owner, "world damage");
			return;
		}

		if (attacker && attacker == owner)
		{
			IRRU_LogBlueOnBlueSkip(owner, "self-inflicted");
			return;
		}

		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm && attackerPlayerId > 0 && attackerPlayerId == pm.GetPlayerIdFromControlledEntity(owner))
		{
			IRRU_LogBlueOnBlueSkip(owner, "self-inflicted via indirect instigator");
			return;
		}

		IEntity controlTypeProbe = attacker;
		if (!controlTypeProbe && attackerPlayerId > 0 && pm)
			controlTypeProbe = pm.GetPlayerControlledEntity(attackerPlayerId);
		if (controlTypeProbe)
		{
			SCR_ECharacterControlType attackerType = SCR_CharacterHelper.GetCharacterControlType(controlTypeProbe);
			if (attackerType == SCR_ECharacterControlType.POSSESSED_AI || attackerType == SCR_ECharacterControlType.UNLIMITED_EDITOR)
			{
				IRRU_LogBlueOnBlueSkip(owner, string.Format("attacker is GM-driven (control type %1)", typename.EnumToString(SCR_ECharacterControlType, attackerType)));
				return;
			}
		}

		string victimFaction = IRRU_GetFactionKey(owner);
		string attackerFaction = IRRU_GetAttackerFactionKey(attackerPlayerId, attacker);
		if (victimFaction.IsEmpty() || attackerFaction.IsEmpty() || victimFaction != attackerFaction)
		{
			IRRU_LogBlueOnBlueSkip(owner, string.Format("factions differ or unknown (victim=%1, attacker=%2)", victimFaction, attackerFaction));
			return;
		}

		Print(string.Format("[NoInstantDeath] BLUE-ON-BLUE: %1 knocked unconscious by friendly %2 (faction=%3)",
			GetNameStr(owner), IRRU_GetAttackerDesc(attackerPlayerId, attacker), victimFaction), LogLevel.WARNING);
	}

	protected void IRRU_LogBlueOnBlueSkip(IEntity owner, string reason)
	{
		if (!IRRU_NoInstantDeathSettings.IsDebugEnabled())
			return;

		Print(string.Format("[NoInstantDeath] %1: blue-on-blue check skipped - %2", GetNameStr(owner), reason));
	}

	protected string IRRU_GetAttackerFactionKey(int attackerPlayerId, IEntity attackerEntity)
	{
		if (attackerPlayerId > 0)
		{
			Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(attackerPlayerId);
			if (playerFaction)
				return playerFaction.GetFactionKey();
		}

		return IRRU_GetFactionKey(attackerEntity);
	}

	protected string IRRU_GetAttackerDesc(int attackerPlayerId, IEntity attackerEntity)
	{
		if (attackerPlayerId > 0)
		{
			string playerName = "";
			PlayerManager pm = GetGame().GetPlayerManager();
			if (pm)
				playerName = pm.GetPlayerName(attackerPlayerId);
			if (playerName.IsEmpty())
				playerName = "Player";
			return string.Format("%1#%2", playerName, attackerPlayerId);
		}

		return GetNameStr(attackerEntity);
	}

	protected string IRRU_GetFactionKey(IEntity e)
	{
		if (!e)
			return "";

		SCR_FactionAffiliationComponent fac = SCR_FactionAffiliationComponent.Cast(e.FindComponent(SCR_FactionAffiliationComponent));
		if (!fac)
			return "";

		Faction faction = fac.GetAffiliatedFaction();
		if (!faction)
			return "";

		return faction.GetFactionKey();
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
		if (m_CachedDmgManager)
		{
			healthStable = (m_CachedDmgManager.IRRU_GetHealthPercentage() > 33.0 && m_CachedDmgManager.IRRU_GetBloodPercentage() > 33.0);
		}

		if (!m_bReceivingCPR && !healthStable)
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

			Print(string.Format("[NoInstantDeath] %1: bleedout %2/%3s remaining%4 - Health: %5%%, Blood: %6%%, Resilience: %7%%, Bleeding: %8 ml/s",
				GetNameStr(owner), (bleedoutTime - m_fUnconsciousTimer), bleedoutTime, pauseReason,
				m_CachedDmgManager.IRRU_GetHealthPercentage(), m_CachedDmgManager.IRRU_GetBloodPercentage(), m_CachedDmgManager.IRRU_GetResiliencePercentage(),
				m_CachedDmgManager.IRRU_GetBleedingRateMLPerSecond()));
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

	void IRRU_SetActiveCPRHelper(IRRU_CPRHelperCompartment helper)
	{
		m_pActiveCPRHelper = helper;
	}

	IRRU_CPRHelperCompartment IRRU_GetActiveCPRHelper()
	{
		return m_pActiveCPRHelper;
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
