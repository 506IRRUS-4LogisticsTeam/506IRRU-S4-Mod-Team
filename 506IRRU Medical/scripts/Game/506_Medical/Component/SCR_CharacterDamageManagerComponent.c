modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	protected const float ARMOR_HIT_PAIN_SCALE = 3.0;
	protected ACE_Medical_PainHitZone m_pPainHitZone;

	float IRRU_GetHealthPercentage()
	{
		array<HitZone> hitZones = {};
		GetAllHitZones(hitZones);
		foreach (HitZone hz : hitZones)
		{
			SCR_CharacterHealthHitZone healthHZ = SCR_CharacterHealthHitZone.Cast(hz);
			if (healthHZ)
			{
				float maxHealth = healthHZ.GetMaxHealth();
				if (maxHealth <= 0)
					return 100.0;
				return (healthHZ.GetHealth() / maxHealth) * 100.0;
			}
		}
		return 100.0;
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

	string IRRU_GetHealthColorCode(float percentage)
	{
		if (percentage >= 75) return "00FF00";
		else if (percentage >= 50) return "FFFF00";
		else if (percentage >= 25) return "FFA500";
		else return "FF0000";
	}

	float IRRU_GetLimbHealthPercentage(ECharacterHitZoneGroup limb)
	{
		return GetGroupHealthScaled(limb) * 100.0;
	}

	void IRRU_GetAllLimbHealthPercentages(out float head, out float chest, out float abdomen,
		out float leftArm, out float rightArm, out float leftLeg, out float rightLeg)
	{
		head = IRRU_GetLimbHealthPercentage(ECharacterHitZoneGroup.HEAD);
		chest = IRRU_GetLimbHealthPercentage(ECharacterHitZoneGroup.UPPERTORSO);
		abdomen = IRRU_GetLimbHealthPercentage(ECharacterHitZoneGroup.LOWERTORSO);
		leftArm = IRRU_GetLimbHealthPercentage(ECharacterHitZoneGroup.LEFTARM);
		rightArm = IRRU_GetLimbHealthPercentage(ECharacterHitZoneGroup.RIGHTARM);
		leftLeg = IRRU_GetLimbHealthPercentage(ECharacterHitZoneGroup.LEFTLEG);
		rightLeg = IRRU_GetLimbHealthPercentage(ECharacterHitZoneGroup.RIGHTLEG);
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
