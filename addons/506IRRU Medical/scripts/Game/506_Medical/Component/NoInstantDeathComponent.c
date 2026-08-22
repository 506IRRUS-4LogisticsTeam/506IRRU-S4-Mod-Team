[ComponentEditorProps(category: "Health", description: "Overrides death to force player bleed-out")]
class IRRU_NoInstantDeathComponentClass : ScriptComponentClass
{
}

class IRRU_NoInstantDeathComponent : ScriptComponent
{
	protected const float CHECK_INTERVAL = 1.0;

	protected bool m_bNID_Initialized = false;
	[RplProp()]
	protected bool m_bIsUnconscious = false;
	[RplProp()]
	protected bool m_bReceivingCPR = false;
	[RplProp()]
	protected float m_fCPRCooldownTimer = 0.0;
	[RplProp()]
	protected float m_fUnconsciousTimer = 0.0;

	protected IRRU_CPRHelperCompartment m_pActiveCPRHelper;

	//! Captured on unconsciousness; read by the Ticket System's modded hook
	protected Instigator m_LastKnownInstigator;
	protected SCR_CharacterDamageManagerComponent m_CachedDmgManager;
	protected SCR_CharacterControllerComponent m_Ctrl;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_CachedDmgManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_Ctrl = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));

		if (m_Ctrl)
			m_Ctrl.m_OnLifeStateChanged.Insert(OnLifeStateChanged);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (m_Ctrl)
			m_Ctrl.m_OnLifeStateChanged.Remove(OnLifeStateChanged);

		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		GetGame().GetCallqueue().Remove(UpdateCPRCooldownTimer);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	bool IsInitialized()
	{
		return m_bNID_Initialized;
	}

	//------------------------------------------------------------------------------------------------
	//! Arms bleedout handling; called once a player takes control of the character
	void Initialize()
	{
		if (m_bNID_Initialized || !m_CachedDmgManager)
			return;

		m_bNID_Initialized = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		if (newLifeState != ECharacterLifeState.INCAPACITATED || previousLifeState != ECharacterLifeState.ALIVE)
			return;

		if (Replication.IsServer() || !Replication.IsRunning())
			IRRU_StowWeaponOnUnconscious();

		if (m_bNID_Initialized && !m_bIsUnconscious)
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

	//------------------------------------------------------------------------------------------------
	void MakeUnconscious(IEntity owner)
	{
		if (!m_bNID_Initialized || m_bIsUnconscious || !m_CachedDmgManager)
			return;

		m_bIsUnconscious = true;
		m_fUnconsciousTimer = 0.0;
		m_LastKnownInstigator = m_CachedDmgManager.GetInstigator();
		m_CachedDmgManager.ForceUnconsciousness();

		if (Replication.IsServer() || !Replication.IsRunning())
			IRRU_LogBlueOnBlue(owner);

		if (Replication.IsServer())
		{
			Replication.BumpMe();
			GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
			GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
		}

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
		{
			Print(string.Format("[NoInstantDeath] %1: unconscious, %2s bleedout - Health: %3%%, Blood: %4%%, Resilience: %5%%",
				GetNameStr(owner), IRRU_NoInstantDeathSettings.GetBleedoutTime(), m_CachedDmgManager.IRRU_GetHealthPercentage(), m_CachedDmgManager.IRRU_GetBloodPercentage(), m_CachedDmgManager.IRRU_GetResiliencePercentage()));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_LogBlueOnBlue(IEntity owner)
	{
		SCR_ECharacterControlType victimType = SCR_CharacterHelper.GetCharacterControlType(owner);
		if (victimType == SCR_ECharacterControlType.AI || victimType == SCR_ECharacterControlType.POSSESSED_AI || victimType == SCR_ECharacterControlType.UNKNOWN)
			return;

		if (!m_LastKnownInstigator)
		{
			IRRU_LogBlueOnBlueSkip(owner, "no instigator recorded");
			return;
		}

		int attackerPlayerId = m_LastKnownInstigator.GetInstigatorPlayerID();
		IEntity attacker = m_LastKnownInstigator.GetInstigatorEntity();

		if (attackerPlayerId <= 0 && !attacker)
		{
			IRRU_LogBlueOnBlueSkip(owner, "world damage");
			return;
		}

		if (attacker == owner)
		{
			IRRU_LogBlueOnBlueSkip(owner, "self-inflicted");
			return;
		}

		PlayerManager pm = GetGame().GetPlayerManager();
		int victimPlayerId = pm.GetPlayerIdFromControlledEntity(owner);
		if (attackerPlayerId > 0 && attackerPlayerId == victimPlayerId)
		{
			IRRU_LogBlueOnBlueSkip(owner, "self-inflicted via indirect instigator");
			return;
		}

		IEntity controlTypeProbe = attacker;
		if (!controlTypeProbe && attackerPlayerId > 0)
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

		string victimFaction = IRRU_ResolveFactionKey(victimPlayerId, owner);
		string attackerFaction = IRRU_ResolveFactionKey(attackerPlayerId, attacker);
		if (victimFaction.IsEmpty() || victimFaction != attackerFaction)
		{
			IRRU_LogBlueOnBlueSkip(owner, string.Format("factions differ or unknown (victim=%1, attacker=%2)", victimFaction, attackerFaction));
			return;
		}

		Print(string.Format("[NoInstantDeath] BLUE-ON-BLUE: %1 knocked unconscious by friendly %2 (faction=%3)",
			GetNameStr(owner), IRRU_GetAttackerDesc(attackerPlayerId, attacker), victimFaction), LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_LogBlueOnBlueSkip(IEntity owner, string reason)
	{
		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] %1: blue-on-blue check skipped - %2", GetNameStr(owner), reason));
	}

	//------------------------------------------------------------------------------------------------
	//! Player record first: GM-placed and quick-deploy bodies carry no faction affiliation
	protected string IRRU_ResolveFactionKey(int playerId, IEntity entity)
	{
		if (playerId > 0)
		{
			Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(playerId);
			if (playerFaction)
				return playerFaction.GetFactionKey();
		}

		if (!entity)
			return "";

		SCR_FactionAffiliationComponent fac = SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent));
		if (!fac)
			return "";

		Faction faction = fac.GetAffiliatedFaction();
		if (!faction)
			faction = fac.GetDefaultAffiliatedFaction();
		if (!faction)
			return "";

		return faction.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	//! Custom roster name from the name tags addon, platform name when none is set
	protected string IRRU_GetPlayerDisplayName(int playerId)
	{
		return IRRU_PlayerNameHelper.GetPlayerDisplayName(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected string IRRU_GetAttackerDesc(int attackerPlayerId, IEntity attackerEntity)
	{
		if (attackerPlayerId <= 0)
			return GetNameStr(attackerEntity);

		string playerName = IRRU_GetPlayerDisplayName(attackerPlayerId);
		if (playerName.IsEmpty())
			playerName = "Player";

		return string.Format("%1#%2", playerName, attackerPlayerId);
	}

	//------------------------------------------------------------------------------------------------
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
			// Something killed the character past the Kill() backstop; pin them back
			// to unconscious and give up if that does not take.
			m_CachedDmgManager.ForceUnconsciousness();
			if (m_Ctrl.GetLifeState() == ECharacterLifeState.DEAD)
			{
				Print(string.Format("[NoInstantDeath] %1 reached DEAD state before timer expiry", GetNameStr(owner)), LogLevel.WARNING);
				StopBleedoutTimer("forced death");
				return;
			}
		}

		bool healthStable = m_CachedDmgManager.IRRU_GetHealthPercentage() > 33.0 && m_CachedDmgManager.IRRU_GetBloodPercentage() > 33.0;
		if (!m_bReceivingCPR && !healthStable)
		{
			m_fUnconsciousTimer += CHECK_INTERVAL;
			Replication.BumpMe();
		}

		if (m_fUnconsciousTimer >= IRRU_NoInstantDeathSettings.GetBleedoutTime())
		{
			if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
				Print(string.Format("[NoInstantDeath] %1: bleedout timer expired", GetNameStr(owner)));

			KillCharacter(owner);
			return;
		}

		GetGame().GetCallqueue().CallLater(UpdateUnconsciousTimer, CHECK_INTERVAL * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	void KillCharacter(IEntity owner)
	{
		if (!m_bIsUnconscious || !m_CachedDmgManager)
			return;

		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);

		HitZone hz = m_CachedDmgManager.GetDefaultHitZone();
		if (hz)
			hz.SetHealth(0);

		m_bIsUnconscious = false;
		m_fUnconsciousTimer = 0.0;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void StopBleedoutTimer(string reason)
	{
		if (!m_bIsUnconscious)
			return;

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] %1: bleedout cancelled (%2)", GetNameStr(GetOwner()), reason));

		m_bIsUnconscious = false;
		m_fUnconsciousTimer = 0.0;
		GetGame().GetCallqueue().Remove(UpdateUnconsciousTimer);
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	string GetNameStr(IEntity e)
	{
		if (!e)
			return "UnknownEntity";

		int pid = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(e);
		if (pid > 0)
		{
			string n = IRRU_GetPlayerDisplayName(pid);
			if (!n.IsEmpty())
				return n;
		}

		return e.ToString();
	}

	//------------------------------------------------------------------------------------------------
	float GetBleedoutTimeRemaining()
	{
		if (!m_bIsUnconscious)
			return -1.0;

		return IRRU_NoInstantDeathSettings.GetBleedoutTime() - m_fUnconsciousTimer;
	}

	bool IsUnconscious() { return m_bIsUnconscious; }
	bool IsReceivingCPR() { return m_bReceivingCPR; }
	bool IsOnCPRCooldown() { return m_fCPRCooldownTimer > 0; }
	float GetCPRCooldownRemaining() { return m_fCPRCooldownTimer; }
	IRRU_CPRHelperCompartment IRRU_GetActiveCPRHelper() { return m_pActiveCPRHelper; }
	void IRRU_SetActiveCPRHelper(IRRU_CPRHelperCompartment helper) { m_pActiveCPRHelper = helper; }

	//------------------------------------------------------------------------------------------------
	void SetReceivingCPR(bool receiving)
	{
		if (m_bReceivingCPR == receiving)
			return;

		m_bReceivingCPR = receiving;
		Replication.BumpMe();

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] %1: CPR receiving=%2", GetNameStr(GetOwner()), receiving));
	}

	//------------------------------------------------------------------------------------------------
	void SetCPRCooldown(float cooldownDuration)
	{
		m_fCPRCooldownTimer = cooldownDuration;
		Replication.BumpMe();

		if (cooldownDuration > 0 && Replication.IsServer())
		{
			GetGame().GetCallqueue().Remove(UpdateCPRCooldownTimer);
			GetGame().GetCallqueue().CallLater(UpdateCPRCooldownTimer, 1000, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCPRCooldownTimer()
	{
		m_fCPRCooldownTimer -= 1.0;

		if (m_fCPRCooldownTimer <= 0)
		{
			m_fCPRCooldownTimer = 0;
			GetGame().GetCallqueue().Remove(UpdateCPRCooldownTimer);
		}

		Replication.BumpMe();
	}
}
