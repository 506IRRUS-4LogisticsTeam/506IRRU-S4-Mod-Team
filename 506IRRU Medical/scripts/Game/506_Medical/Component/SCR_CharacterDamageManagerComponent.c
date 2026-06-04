modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	protected const float ARMOR_HIT_PAIN_SCALE = 3.0;
	protected ACE_Medical_PainHitZone m_pPainHitZone;
	protected SCR_CharacterHealthHitZone m_IRRU_HealthHitZone;

	//! Lazily resolves and caches the character's overall health hitzone by TYPE.
	//! IMPORTANT: do NOT use GetDefaultHitZone() here. Under ACE Medical the default hitzone is a
	//! separate plain SCR_HitZone ("ACE_Medical_Default", HZDefault 1) while the SCR_CharacterHealthHitZone
	//! ("Health") is HZDefault 0 - so GetDefaultHitZone() would not return the health zone. A type-based
	//! lookup is correct under both vanilla and ACE; caching it avoids the per-call array allocation + scan
	//! (this is read every casualty-UI tick and every bleedout tick). The hitzone instance is stable for
	//! the character's lifetime, matching how the base game caches blood/resilience/head hitzones.
	SCR_CharacterHealthHitZone IRRU_GetHealthHitZone()
	{
		if (m_IRRU_HealthHitZone)
			return m_IRRU_HealthHitZone;

		array<HitZone> hitZones = {};
		GetAllHitZones(hitZones);
		foreach (HitZone hz : hitZones)
		{
			SCR_CharacterHealthHitZone healthHZ = SCR_CharacterHealthHitZone.Cast(hz);
			if (healthHZ)
			{
				m_IRRU_HealthHitZone = healthHZ;
				return m_IRRU_HealthHitZone;
			}
		}
		return null;
	}

	float IRRU_GetHealthPercentage()
	{
		SCR_CharacterHealthHitZone healthHZ = IRRU_GetHealthHitZone();
		if (!healthHZ)
			return 100.0;

		float maxHealth = healthHZ.GetMaxHealth();
		if (maxHealth <= 0)
			return 100.0;

		return (healthHZ.GetHealth() / maxHealth) * 100.0;
	}

	float IRRU_GetBloodPercentage()
	{
		SCR_CharacterBloodHitZone bloodHZ = GetBloodHitZone();
		if (!bloodHZ)
			return 100.0;
		float maxBlood = bloodHZ.GetMaxHealth();
		if (maxBlood <= 0)
			return 100.0;
		return (bloodHZ.GetHealth() / maxBlood) * 100.0;
	}

	float IRRU_GetResiliencePercentage()
	{
		SCR_CharacterResilienceHitZone resilienceHZ = GetResilienceHitZone();
		if (!resilienceHZ)
			return -1.0;
		float maxResilience = resilienceHZ.GetMaxHealth();
		if (maxResilience <= 0)
			return 100.0;
		return (resilienceHZ.GetHealth() / maxResilience) * 100.0;
	}

	bool IRRU_HasResilienceSystem()
	{
		return GetResilienceHitZone() != null;
	}

	float IRRU_GetBleedingRateMLPerSecond()
	{
		SCR_CharacterBloodHitZone bloodHZ = GetBloodHitZone();
		if (!bloodHZ)
			return 0.0;
		return bloodHZ.GetTotalBleedingAmount();
	}

	void IRRU_GetBleedoutTimerInfo(out float timeRemaining, out float totalTime, out bool isBleedingOut)
	{
		IEntity owner = GetOwner();
		IRRU_NoInstantDeathComponent nid = null;
		if (owner)
			nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));

		if (nid && nid.IsUnconscious())
		{
			isBleedingOut = true;
			timeRemaining = nid.GetBleedoutTimeRemaining();
			totalTime = nid.GetBleedoutTimeTotal();
		}
		else
		{
			isBleedingOut = false;
			timeRemaining = -1.0;
			totalTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
		}
	}

	void ACE_Medical_SetPainHitZone(ACE_Medical_PainHitZone hz) { m_pPainHitZone = hz; }
	override ACE_Medical_PainHitZone ACE_Medical_GetPainHitZone() { return m_pPainHitZone; }

	override bool ACE_Medical_IsInPain()
	{
		if (!m_pPainHitZone)
			return false;
		return m_pPainHitZone.GetDamageState() != EDamageState.UNDAMAGED;
	}

	override float ACE_Medical_GetPainIntensity()
	{
		if (!m_pPainHitZone)
			return 0.0;
		return 1.0 - m_pPainHitZone.GetHealthScaled();
	}

	override bool ACE_Medical_ShouldDeactivateSecondChance()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return super.ACE_Medical_ShouldDeactivateSecondChance();

		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));
		if (!nid)
			return super.ACE_Medical_ShouldDeactivateSecondChance();

		if (!nid.IsInitialized())
			nid.Initialize();

		if (nid.IsUnconscious())
			return false;

		return super.ACE_Medical_ShouldDeactivateSecondChance();
	}

	override void ACE_Medical_OnSecondChanceGranted()
	{
		super.ACE_Medical_OnSecondChanceGranted();

		IEntity owner = GetOwner();
		if (owner)
		{
			IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));
			if (nid)
			{
				if (!nid.IsInitialized())
					nid.Initialize();
				if (!nid.IsUnconscious())
					nid.MakeUnconscious(owner);
			}
		}

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
		{
			string characterName = "Unknown";
			if (owner)
			{
				IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));
				if (nid)
					characterName = nid.GetNameStr(owner);
			}

			SCR_CharacterResilienceHitZone resilienceHZ = GetResilienceHitZone();
			HitZone healthHZ = GetDefaultHitZone();
			float currentHealth = 0;
			float currentResilience = 0;
			if (healthHZ)
				currentHealth = healthHZ.GetHealth();
			if (resilienceHZ)
				currentResilience = resilienceHZ.GetHealthScaled();

			Print(string.Format("[NoInstantDeath] SecondChance triggered on %1 - Health: %2, Resilience: %3%%", characterName, currentHealth, currentResilience * 100.0));
		}
	}

	override void ArmorHitEventDamage(EDamageType type, float damage, IEntity instigator)
	{
		super.ArmorHitEventDamage(type, damage, instigator);
		if (m_pPainHitZone)
			m_pPainHitZone.HandleDamage(damage * ARMOR_HIT_PAIN_SCALE, type, instigator);
	}

	void IRRU_GetDetailedMedicalStatus(out float healthPercent, out float bloodPercent,
		out float resiliencePercent, out bool hasResilience,
		out float bleedingRateMLs, out bool isUnconscious,
		out float bleedoutTimeRemaining, out bool isBleedingOut)
	{
		healthPercent = IRRU_GetHealthPercentage();
		bloodPercent = IRRU_GetBloodPercentage();
		resiliencePercent = IRRU_GetResiliencePercentage();
		hasResilience = IRRU_HasResilienceSystem();
		bleedingRateMLs = IRRU_GetBleedingRateMLPerSecond();

		IEntity owner = GetOwner();
		IRRU_NoInstantDeathComponent nid = null;
		if (owner)
			nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));
		isUnconscious = (nid && nid.IsUnconscious());

		float totalTime;
		IRRU_GetBleedoutTimerInfo(bleedoutTimeRemaining, totalTime, isBleedingOut);
	}

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
			Print(string.Format("[NoInstantDeath] HealHitZonesInParallel hit max iterations"), LogLevel.WARNING);

		return healthToDistribute;
	}
}
