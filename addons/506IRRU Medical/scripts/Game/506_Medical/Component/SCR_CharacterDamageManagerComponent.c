modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	protected const float ARMOR_HIT_PAIN_SCALE = 3.0;
	protected const float IRRU_KILL_BLOCK_LOG_INTERVAL_MS = 5000;
	protected ACE_Medical_PainHitZone m_pPainHitZone;
	protected SCR_CharacterHealthHitZone m_IRRU_HealthHitZone;
	protected IRRU_NoInstantDeathComponent m_IRRU_NID;
	protected int m_iIRRU_ResilienceTextState = -1;
	protected float m_fIRRU_NextKillBlockLogTimeMS;

	//------------------------------------------------------------------------------------------------
	//! Under ACE the default hit zone is not the health hit zone, so scan once and cache
	SCR_CharacterHealthHitZone IRRU_GetHealthHitZone()
	{
		if (m_IRRU_HealthHitZone)
			return m_IRRU_HealthHitZone;

		array<HitZone> hitZones = {};
		GetAllHitZones(hitZones);
		foreach (HitZone hz : hitZones)
		{
			m_IRRU_HealthHitZone = SCR_CharacterHealthHitZone.Cast(hz);
			if (m_IRRU_HealthHitZone)
				break;
		}

		return m_IRRU_HealthHitZone;
	}

	//------------------------------------------------------------------------------------------------
	protected IRRU_NoInstantDeathComponent IRRU_GetNID()
	{
		if (!m_IRRU_NID)
			m_IRRU_NID = IRRU_NoInstantDeathComponent.Cast(GetOwner().FindComponent(IRRU_NoInstantDeathComponent));

		return m_IRRU_NID;
	}

	//------------------------------------------------------------------------------------------------
	float IRRU_GetHealthPercentage()
	{
		SCR_CharacterHealthHitZone healthHZ = IRRU_GetHealthHitZone();
		if (!healthHZ || healthHZ.GetMaxHealth() <= 0)
			return 100.0;

		return healthHZ.GetHealth() / healthHZ.GetMaxHealth() * 100.0;
	}

	//------------------------------------------------------------------------------------------------
	float IRRU_GetBloodPercentage()
	{
		SCR_CharacterBloodHitZone bloodHZ = GetBloodHitZone();
		if (!bloodHZ || bloodHZ.GetMaxHealth() <= 0)
			return 100.0;

		return bloodHZ.GetHealth() / bloodHZ.GetMaxHealth() * 100.0;
	}

	//------------------------------------------------------------------------------------------------
	//! \return -1 when the character has no resilience hit zone
	float IRRU_GetResiliencePercentage()
	{
		SCR_CharacterResilienceHitZone resilienceHZ = GetResilienceHitZone();
		if (!resilienceHZ)
			return -1.0;
		if (resilienceHZ.GetMaxHealth() <= 0)
			return 100.0;

		return resilienceHZ.GetHealth() / resilienceHZ.GetMaxHealth() * 100.0;
	}

	//------------------------------------------------------------------------------------------------
	float IRRU_GetBleedingRateMLPerSecond()
	{
		SCR_CharacterBloodHitZone bloodHZ = GetBloodHitZone();
		if (!bloodHZ)
			return 0.0;

		return bloodHZ.GetTotalBleedingAmount();
	}

	//------------------------------------------------------------------------------------------------
	override void SetResilienceHitZone(HitZone hitZone)
	{
		super.SetResilienceHitZone(hitZone);

		SCR_CharacterResilienceHitZone resilienceHZ = GetResilienceHitZone();
		if (resilienceHZ)
			resilienceHZ.GetOnHealthChanged().Insert(IRRU_RefreshResilienceReplication);
	}

	//------------------------------------------------------------------------------------------------
	//! Push a replication update whenever the resilience crosses an inspection-text threshold
	protected void IRRU_RefreshResilienceReplication()
	{
		if (!Replication.IsServer())
			return;

		int textState = IRRU_GetResilienceTextState();
		if (textState == m_iIRRU_ResilienceTextState)
			return;

		m_iIRRU_ResilienceTextState = textState;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected int IRRU_GetResilienceTextState()
	{
		float resiliencePercent = IRRU_GetResiliencePercentage();
		if (resiliencePercent < 33)
			return 0;
		if (resiliencePercent <= 59)
			return 1;
		if (resiliencePercent <= 99)
			return 2;
		return 3;
	}

	//------------------------------------------------------------------------------------------------
	void ACE_Medical_SetPainHitZone(ACE_Medical_PainHitZone hz) { m_pPainHitZone = hz; }
	override ACE_Medical_PainHitZone ACE_Medical_GetPainHitZone() { return m_pPainHitZone; }

	override bool ACE_Medical_IsInPain()
	{
		return m_pPainHitZone && m_pPainHitZone.GetDamageState() != EDamageState.UNDAMAGED;
	}

	override float ACE_Medical_GetPainIntensity()
	{
		if (!m_pPainHitZone)
			return 0.0;

		return 1.0 - m_pPainHitZone.GetHealthScaled();
	}

	//------------------------------------------------------------------------------------------------
	//! Keep ACE from disabling Second Chance while our bleedout timer owns the character
	override bool ACE_Medical_ShouldDeactivateSecondChance()
	{
		IRRU_NoInstantDeathComponent nid = IRRU_GetNID();
		if (nid)
		{
			if (!nid.IsInitialized())
				nid.Initialize();

			if (nid.IsUnconscious())
				return false;
		}

		return super.ACE_Medical_ShouldDeactivateSecondChance();
	}

	//------------------------------------------------------------------------------------------------
	//! Backstop: nothing may kill an unconscious player except the bleedout timer
	override void Kill(notnull Instigator instigator)
	{
		if (Replication.IsServer() || !Replication.IsRunning())
		{
			IRRU_NoInstantDeathComponent nid = IRRU_GetNID();
			IEntity owner = GetOwner();
			if (nid && nid.IsUnconscious() && GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner) > 0)
			{
				float nowMS = owner.GetWorld().GetWorldTime();
				if (nowMS >= m_fIRRU_NextKillBlockLogTimeMS)
				{
					m_fIRRU_NextKillBlockLogTimeMS = nowMS + IRRU_KILL_BLOCK_LOG_INTERVAL_MS;
					Print(string.Format("[NoInstantDeath] BLOCKED Kill() on %1 during bleedout - Blood: %2%%, Health: %3%%, timer remaining: %4s, instigatorPlayerId: %5",
						nid.GetNameStr(owner), IRRU_GetBloodPercentage(), IRRU_GetHealthPercentage(), nid.GetBleedoutTimeRemaining(), instigator.GetInstigatorPlayerID()), LogLevel.WARNING);
				}

				return;
			}
		}

		super.Kill(instigator);
	}

	//------------------------------------------------------------------------------------------------
	override void ACE_Medical_OnSecondChanceGranted()
	{
		super.ACE_Medical_OnSecondChanceGranted();

		IRRU_NoInstantDeathComponent nid = IRRU_GetNID();
		if (!nid)
			return;

		if (!nid.IsInitialized())
			nid.Initialize();
		if (!nid.IsUnconscious())
			nid.MakeUnconscious(GetOwner());

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print(string.Format("[NoInstantDeath] SecondChance triggered on %1 - Health: %2%%, Resilience: %3%%", nid.GetNameStr(GetOwner()), IRRU_GetHealthPercentage(), IRRU_GetResiliencePercentage()));
	}

	//------------------------------------------------------------------------------------------------
	override void FullHeal(bool ignoreHealingDOT = true)
	{
		super.FullHeal(ignoreHealingDOT);
		RemoveAllBleedingParticles();
	}

	//------------------------------------------------------------------------------------------------
	override void ArmorHitEventDamage(EDamageType type, float damage, IEntity instigator)
	{
		super.ArmorHitEventDamage(type, damage, instigator);
		if (m_pPainHitZone)
			m_pPainHitZone.HandleDamage(damage * ARMOR_HIT_PAIN_SCALE, type, instigator);
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla distribution with an iteration cap so a zone that refuses healing cannot spin forever
	override protected float HealHitZonesInParallel(float healthToDistribute, float maxHealThresholdScaled, array<HitZone> targetHitZones)
	{
		array<HitZone> damagedHitZones = {};
		int maxIterations = 100;
		int iteration = 0;

		while (healthToDistribute > 0.01 && iteration < maxIterations)
		{
			iteration++;

			foreach (HitZone hitZone : targetHitZones)
			{
				if (hitZone.GetHealth() < (hitZone.GetMaxHealth() * maxHealThresholdScaled))
					damagedHitZones.Insert(hitZone);
			}

			if (damagedHitZones.IsEmpty())
				break;

			float healthPerZone = healthToDistribute / damagedHitZones.Count();
			foreach (HitZone hitZone : damagedHitZones)
			{
				if (healthToDistribute <= 0.01)
					break;

				float healthToAdd = (hitZone.GetMaxHealth() * maxHealThresholdScaled) - hitZone.GetHealth();
				if (healthToAdd <= 0.01)
					continue;

				if (healthPerZone > healthToAdd)
				{
					hitZone.HandleDamage(-healthToAdd, EDamageType.HEALING, null);
					healthToDistribute -= healthToAdd;
				}
				else
				{
					hitZone.HandleDamage(-healthPerZone, EDamageType.HEALING, null);
					healthToDistribute -= healthPerZone;
				}
			}

			damagedHitZones.Clear();
		}

		if (iteration >= maxIterations && IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print("[NoInstantDeath] HealHitZonesInParallel hit max iterations", LogLevel.WARNING);

		return healthToDistribute;
	}
}
