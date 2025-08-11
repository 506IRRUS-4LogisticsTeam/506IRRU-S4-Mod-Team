// ============================================================================
//  SCR_CharacterDamageManagerComponent.c  
//  506th IRRU Medical Mod v2.0.5
//  – Prevents any vital hit‑zone from falling below 1 HP
//  – Restores OnCustomDamageTaken invoker
//  – Methods to return exact percentages for inspection
//  – Bleedout timer support with dynamic settings
// ============================================================================

modded class SCR_CharacterDamageManagerComponent
    : SCR_CharacterDamageManagerComponent
{
	// Public invoker for other scripts
	ref ScriptInvoker OnCustomDamageTaken = new ScriptInvoker();

	// ─── debug ───────────────────────────────────────────────────────────
	protected void NID_DebugPrint(string msg)
	{
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print("[NoInstantDeath][DMG] " + msg);
	}

	protected string GetPlayerOrEntityNameStr(IEntity entity)
	{
		if (!entity) return "UnknownEntity(null)";

		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			SCR_ChimeraCharacter chr = SCR_ChimeraCharacter.Cast(entity);
			if (chr)
			{
				int pid = pm.GetPlayerIdFromControlledEntity(chr);
				if (pid > 0)
				{
					string n = pm.GetPlayerName(pid);
					if (!n.IsEmpty()) return n;
				}
			}
		}
		return entity.ToString();
	}

	// ══════════════════════════════════════════════════════════════════════
	// NEW PERCENTAGE METHODS FOR INSPECTION
	// ══════════════════════════════════════════════════════════════════════

	//! Get exact health percentage (0-100)
	float GetHealthPercentage()
	{
		HitZone defaultHZ = GetDefaultHitZone();
		if (!defaultHZ)
			return 100.0;
		
		// For No-Instant-Death compatibility, check if unconscious
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
		
		// If unconscious and NID is active, show very low health but not 0
		if (nid && nid.IsUnconscious())
		{
			float currentHealth = defaultHZ.GetHealth();
			float maxHealth = defaultHZ.GetMaxHealth();
			
			// Ensure we show at least 1% when unconscious
			if (maxHealth > 0)
				return Math.Max(1.0, (currentHealth / maxHealth) * 100.0);
			else
				return 1.0;
		}
		
		// Normal health calculation
		return GetHealthScaled() * 100.0;
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

	//! Get exact resilience percentage (0-100) 
	float GetResiliencePercentage()
	{
		SCR_CharacterResilienceHitZone resilienceHZ = GetResilienceHitZone();
		if (!resilienceHZ)
			return -1.0; // Return -1 to indicate no resilience system
		
		float currentResilience = resilienceHZ.GetHealth();
		float maxResilience = resilienceHZ.GetMaxHealth();
		
		if (maxResilience <= 0)
			return 100.0;
			
		return (currentResilience / maxResilience) * 100.0;
	}

	//! Check if character has resilience system
	bool HasResilienceSystem()
	{
		return GetResilienceHitZone() != null;
	}

	//! Get bleeding rate in ml/s (more intuitive than the raw value)
	float GetBleedingRateMLPerSecond()
	{
		SCR_CharacterBloodHitZone bloodHZ = GetBloodHitZone();
		if (!bloodHZ)
			return 0.0;
			
		// Return raw bleeding rate value
		return bloodHZ.GetTotalBleedingAmount() * GetBleedingScale(); 
	}

	//! Get bleedout timer info
	void GetBleedoutTimerInfo(out float timeRemaining, out float totalTime, out bool isBleedingOut)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
			
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
			totalTime = NoInstantDeath_Settings.GetBleedoutTime(); // Get from settings
		}
	}

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
		
		// Check unconscious state using your NID system
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
			
		isUnconscious = (nid && nid.IsUnconscious());
		
		// Get bleedout timer
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

	// ─────────────────────────────────────────────────────────────────────
	// ORIGINAL NO-INSTANT-DEATH METHODS
	// ─────────────────────────────────────────────────────────────────────
	override void OnDamage(notnull BaseDamageContext damageContext)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(
				owner.FindComponent(NoInstantDeathComponent));

		// —— 1) AI or un‑initialised pawn → vanilla path ——
		if (!nid || !nid.NID_IsInitialized())
		{
			super.OnDamage(damageContext);
			vector zv = vector.Zero;
			OnCustomDamageTaken.Invoke(owner, damageContext.damageValue,
			                           damageContext.instigator, zv,
			                           damageContext.struckHitZone);
			return;
		}

		// —— 2) First lethal hit while conscious → knock‑out ——
		if (!nid.IsUnconscious())
		{
			float projected = GetHealth() - damageContext.damageValue;
			if (projected <= 0.1)
			{
				nid.MakeUnconscious(owner);

				// Apply 5‑HP buffer but ALSO guarantee ≥1 HP everywhere
				EnforceMinHealth(GetDefaultHitZone(),     5.0);
				EnforceMinHealth(GetHitZoneByName("Head"),5.0);
				EnforceMinHealth(GetHitZoneByName("Torso"),5.0);

				NID_DebugPrint(string.Format(
					"%1 – lethal hit intercepted (knock‑out)",
					GetPlayerOrEntityNameStr(owner)));
				return;
			}
		}

		// —— 3) Damage while unconscious (non‑healing) ——
		if (nid.IsUnconscious() &&
			damageContext.damageType != EDamageType.HEALING &&
			!nid.IsInitiatingKill())   // ← skip clamp for bleed-out kill
		{
			// Keep both struck zone and core ≥1 HP
			EnforceMinHealth(damageContext.struckHitZone, 1.0);
			EnforceMinHealth(GetDefaultHitZone(),         1.0);
			return;
		}

		// —— 4) Normal pass‑through ——
		super.OnDamage(damageContext);
		vector zv2 = vector.Zero;
		OnCustomDamageTaken.Invoke(owner, damageContext.damageValue,
		                           damageContext.instigator, zv2,
		                           damageContext.struckHitZone);
	}

	// Utility: clamp zone health
	protected void EnforceMinHealth(HitZone hz, float minHP)
	{
		if (hz && hz.GetHealth() < minHP)
			hz.SetHealth(minHP);
	}

	// ─────────────────────────────────────────────────────────────────────
	override void OnDamageStateChanged(EDamageState state)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (!nid || !nid.NID_IsInitialized())
		{
			super.OnDamageStateChanged(state);
			return;
		}

		if (nid.IsUnconscious() && !nid.IsInitiatingKill()
		    && state == EDamageState.DESTROYED)
		{
			NID_DebugPrint(string.Format(
				"%1 – DESTROYED state intercepted", GetPlayerOrEntityNameStr(owner)));
			return;
		}
		super.OnDamageStateChanged(state);
	}

	// ─────────────────────────────────────────────────────────────────────
	override void Kill(notnull Instigator instigator)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (!nid || !nid.NID_IsInitialized())
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
		{
			NID_DebugPrint(string.Format(
				"%1 – Kill() ignored while unconscious",
				GetPlayerOrEntityNameStr(owner)));
			return;
		}

		nid.MakeUnconscious(owner);
		EnforceMinHealth(GetDefaultHitZone(), 1.0);

		NID_DebugPrint(string.Format(
			"%1 – Kill() intercepted, converted to knock‑out",
			GetPlayerOrEntityNameStr(owner)));
	}
}