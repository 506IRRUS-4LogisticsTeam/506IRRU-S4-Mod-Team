//! Extended damage manager with percentage display and no-instant-death

modded class SCR_CharacterDamageManagerComponent
    : SCR_CharacterDamageManagerComponent
{
	protected const float ARMOR_HIT_PAIN_SCALE = 3.0;

	protected ACE_Medical_PainHitZone m_pPainHitZone;

	//------------------------------------------------------------------------------------------------
	//! Get exact health percentage (0-100)
	//! Returns TRUE health hitzone value (not the "default" hitzone)
	float GetHealthPercentage()
	{
		// Find the actual Health hitzone, not the "default" hitzone
		array<HitZone> hitZones = {};
		GetAllHitZones(hitZones);

		foreach (HitZone hz : hitZones)
		{
			SCR_CharacterHealthHitZone healthHZ = SCR_CharacterHealthHitZone.Cast(hz);
			if (healthHZ)
			{
				float currentHealth = healthHZ.GetHealth();
				float maxHealth = healthHZ.GetMaxHealth();

				if (maxHealth <= 0)
					return 100.0;

				return (currentHealth / maxHealth) * 100.0;
			}
		}

		return 100.0;
	}

	//! Get exact blood percentage (0-100)
	float GetBloodPercentage()
	{
		SCR_CharacterBloodHitZone bloodHZ = GetBloodHitZone();
		if (!bloodHZ)
			return 100.0;
		
		float currentBlood = bloodHZ.GetHealth();
		float maxBlood = bloodHZ.GetMaxHealth();
		
		if (maxBlood <= 0)
			return 100.0;
			
		return (currentBlood / maxBlood) * 100.0;
	}

	//------------------------------------------------------------------------------------------------
	//! Get exact resilience percentage (0-100)
	float GetResiliencePercentage()
	{
		SCR_CharacterResilienceHitZone resilienceHZ = GetResilienceHitZone();
		if (!resilienceHZ)
			return -1.0;
		
		float currentResilience = resilienceHZ.GetHealth();
		float maxResilience = resilienceHZ.GetMaxHealth();
		
		if (maxResilience <= 0)
			return 100.0;
			
		return (currentResilience / maxResilience) * 100.0;
	}

	//------------------------------------------------------------------------------------------------
	//! Check if character has resilience system
	bool HasResilienceSystem()
	{
		return GetResilienceHitZone() != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Get bleeding rate in ml/s (ACE Medical now handles all bleeding calculations)
	float GetBleedingRateMLPerSecond()
	{
		SCR_CharacterBloodHitZone bloodHZ = GetBloodHitZone();
		if (!bloodHZ)
			return 0.0;

		return bloodHZ.GetTotalBleedingAmount();
	}

	//------------------------------------------------------------------------------------------------
	//! Get bleedout timer info
	void GetBleedoutTimerInfo(out float timeRemaining, out float totalTime, out bool isBleedingOut)
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
			totalTime = IRRU_NoInstantDeathSettings.GetBleedoutTime(); // Get from settings
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void ACE_Medical_SetPainHitZone(ACE_Medical_PainHitZone hz)
	{
		m_pPainHitZone = hz;
	}
	
	//------------------------------------------------------------------------------------------------
	override ACE_Medical_PainHitZone ACE_Medical_GetPainHitZone()
	{
		return m_pPainHitZone;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool ACE_Medical_IsInPain()
	{
		if (!m_pPainHitZone)
			return false;
		
		return m_pPainHitZone.GetDamageState() != EDamageState.UNDAMAGED;
	}
	
	//------------------------------------------------------------------------------------------------
	override float ACE_Medical_GetPainIntensity()
	{
		if (!m_pPainHitZone)
			return 0.0;

		float painHealth = m_pPainHitZone.GetHealthScaled();
		return 1.0 - painHealth;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Override to prevent Second Chance deactivation while our bleedout timer is running
	//! Returns false to keep Second Chance active during entire bleedout period
	override bool ACE_Medical_ShouldDeactivateSecondChance()
	{
		IEntity owner = GetOwner();
		if (owner)
		{
			IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(
				owner.FindComponent(IRRU_NoInstantDeathComponent));

			// Keep Second Chance active while our bleedout timer is running
			if (nid && nid.IsUnconscious())
				return false;
		}

		// If not using our bleedout system, fall back to ACE Medical's default behavior
		// (ACE Medical will deactivate Second Chance after 1 second)
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void ACE_Medical_OnSecondChanceGranted()
	{
		super.ACE_Medical_OnSecondChanceGranted();

		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
		{
			SCR_CharacterResilienceHitZone resilienceHZ = GetResilienceHitZone();
			HitZone healthHZ = GetDefaultHitZone();
			float currentHealth = 0;
			float currentResilience = 0;

			if (healthHZ)
				currentHealth = healthHZ.GetHealth();
			if (resilienceHZ)
				currentResilience = resilienceHZ.GetHealthScaled();

			Print(string.Format("[NoInstantDeath] SecondChance triggered - Health: %1, Resilience: %2%% -> 0%%",
			                   currentHealth, currentResilience * 100.0));

			string stackTrace;
			Debug.DumpStack(stackTrace);
			Print(stackTrace);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void ArmorHitEventDamage(EDamageType type, float damage, IEntity instigator)
	{
		super.ArmorHitEventDamage(type, damage, instigator);

		if (m_pPainHitZone)
			m_pPainHitZone.HandleDamage(damage * ARMOR_HIT_PAIN_SCALE, type, instigator);
	}

	//------------------------------------------------------------------------------------------------
	//! Get detailed medical status for inspection
	void GetDetailedMedicalStatus(out float healthPercent, out float bloodPercent,
								  out float resiliencePercent, out bool hasResilience,
								  out float bleedingRateMLs, out bool isUnconscious,
								  out float bleedoutTimeRemaining, out bool isBleedingOut)
	{
		healthPercent = GetHealthPercentage();
		bloodPercent = GetBloodPercentage();
		resiliencePercent = GetResiliencePercentage();
		hasResilience = HasResilienceSystem();
		bleedingRateMLs = GetBleedingRateMLPerSecond();

		IEntity owner = GetOwner();
		IRRU_NoInstantDeathComponent nid = null;
		if (owner)
			nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));

		isUnconscious = (nid && nid.IsUnconscious());

		float totalTime;
		GetBleedoutTimerInfo(bleedoutTimeRemaining, totalTime, isBleedingOut);
	}

	//! Get color code for health percentage (for UI display)
	string GetHealthColorCode(float percentage)
	{
		if (percentage >= 75)
			return "00FF00"; // Green
		else if (percentage >= 50)
			return "FFFF00"; // Yellow
		else if (percentage >= 25)
			return "FFA500"; // Orange
		else
			return "FF0000"; // Red
	}

	//! Get specific limb health percentage
	float GetLimbHealthPercentage(ECharacterHitZoneGroup limb)
	{
		float limbHealth = GetGroupHealthScaled(limb);
		return limbHealth * 100.0;
	}

	//! Get all limb health percentages at once
	void GetAllLimbHealthPercentages(out float head, out float chest, out float abdomen,
									  out float leftArm, out float rightArm, 
									  out float leftLeg, out float rightLeg)
	{
		head = GetLimbHealthPercentage(ECharacterHitZoneGroup.HEAD);
		chest = GetLimbHealthPercentage(ECharacterHitZoneGroup.UPPERTORSO);
		abdomen = GetLimbHealthPercentage(ECharacterHitZoneGroup.LOWERTORSO);
		leftArm = GetLimbHealthPercentage(ECharacterHitZoneGroup.LEFTARM);
		rightArm = GetLimbHealthPercentage(ECharacterHitZoneGroup.RIGHTARM);
		leftLeg = GetLimbHealthPercentage(ECharacterHitZoneGroup.LEFTLEG);
		rightLeg = GetLimbHealthPercentage(ECharacterHitZoneGroup.RIGHTLEG);
	}

	//------------------------------------------------------------------------------------------------
	override void Kill(notnull Instigator instigator)
	{
		IEntity owner = GetOwner();
		IRRU_NoInstantDeathComponent nid = null;
		if (owner)
			nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));

		if (!nid || !nid.IsInitialized())
		{
			super.Kill(instigator);
			return;
		}

		if (nid.IsInitiatingKill())
		{
			super.Kill(instigator);
			nid.ResetInitiatingKillFlag();
			return;
		}

		if (nid.IsUnconscious())
			return;

		nid.MakeUnconscious(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Fixed version of HealHitZonesInParallel that prevents infinite loops from floating point errors
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

			float healthToDistributeHitZone = healthToDistribute / damagedHitZones.Count();
			foreach (HitZone hitZone : damagedHitZones)
			{
				if (healthToDistribute <= 0.01)
					break;

				float healthToAdd = (hitZone.GetMaxHealth() * maxHealThresholdScaled) - hitZone.GetHealth();
				if (healthToAdd <= 0.01)
					continue;

				if (healthToDistributeHitZone > healthToAdd)
				{
					hitZone.HandleDamage(-healthToAdd, EDamageType.HEALING, null);
					healthToDistribute -= healthToAdd;
					continue;
				}
				else
				{
					hitZone.HandleDamage(-healthToDistributeHitZone, EDamageType.HEALING, null);
					healthToDistribute -= healthToDistributeHitZone;
					continue;
				}
			}

			damagedHitZones.Clear();
		}

		if (iteration >= maxIterations && IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print("[NoInstantDeath] HealHitZonesInParallel hit max iterations, prevented infinite loop", LogLevel.WARNING);

		return healthToDistribute;
	}
}