//! Replaces the vanilla casualty readout with percentage-based wound/blood text, a triage tag
//! with the bleedout countdown, and resilience/CPR lines (widgets added in the modded layout).
modded class SCR_InspectCasualtyWidget : SCR_InfoDisplayExtended
{
	protected TextWidget m_wResilienceText;
	protected TextWidget m_wCPRStatusText;
	protected TextWidget m_wDamageText;
	protected TextWidget m_wBleedingText;
	protected SCR_InventoryDamageInfoUI m_DamageInfoUI;

	//------------------------------------------------------------------------------------------------
	override void DisplayStartDraw(IEntity owner)
	{
		super.DisplayStartDraw(owner);

		if (!m_wCasualtyInspectWidget)
			return;

		m_wResilienceText = TextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("ResilienceText"));
		m_wCPRStatusText = TextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("CPRStatusText"));
		m_wDamageText = TextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("DamageInfo_text"));
		m_wBleedingText = TextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("BleedingInfo_text"));
		m_DamageInfoUI = SCR_InventoryDamageInfoUI.Cast(m_wCasualtyInspectWidget.FindHandler(SCR_InventoryDamageInfoUI));
	}

	//------------------------------------------------------------------------------------------------
	protected string IRRU_GetTriageLevel(float percentRemaining)
	{
		if (percentRemaining > 70)
			return "DELAYED";
		if (percentRemaining > 50)
			return "PRIORITY";
		if (percentRemaining > 30)
			return "URGENT";
		if (percentRemaining > 15)
			return "CRITICAL";
		return "IMMEDIATE";
	}

	//------------------------------------------------------------------------------------------------
	protected string IRRU_GetTriageRGBA(float percentRemaining)
	{
		if (percentRemaining > 70)
			return "0,255,100,255";
		if (percentRemaining > 50)
			return "255,255,0,255";
		if (percentRemaining > 30)
			return "255,200,0,255";
		if (percentRemaining > 15)
			return "255,150,0,255";
		return "255,0,0,255";
	}

	//------------------------------------------------------------------------------------------------
	protected string IRRU_FormatBleedoutTime(float bleedoutTimeRemaining)
	{
		int minutes = Math.Floor(bleedoutTimeRemaining / 60);
		int seconds = Math.Floor(Math.Mod(bleedoutTimeRemaining, 60));
		return string.Format("%1:%2", minutes, seconds.ToString(2));
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateWidgetData()
	{
		if (!m_Target || !m_wCasualtyInspectWidget)
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(m_Target);
		if (!character)
			return;

		SCR_CharacterDamageManagerComponent damageMan = SCR_CharacterDamageManagerComponent.Cast(character.GetDamageManager());
		if (!damageMan)
			return;

		string sName;
		GetCasualtyName(sName, m_Target);

		if (GetGame().GetPlatformService().GetLocalPlatformKind() == PlatformKind.PSN)
		{
			PlayerManager playerMgr = GetGame().GetPlayerManager();
			string icon = UIConstants.PLATFROM_GENERIC_ICON_NAME;
			if (playerMgr.GetPlatformKind(playerMgr.GetPlayerIdFromControlledEntity(m_Target)) == PlatformKind.PSN)
				icon = UIConstants.PLATFROM_PLAYSTATION_ICON_NAME;
			sName = string.Format("<color rgba=%1><image set='%2' name='%3' scale='%4'/></color>", UIColors.FormatColor(GUIColors.ENABLED), UIConstants.ICONS_IMAGE_SET, icon, 2) + sName;
		}

		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(character.FindComponent(IRRU_NoInstantDeathComponent));
		bool isUnconscious = nid && nid.IsUnconscious();
		bool receivingCPR = nid && nid.IsReceivingCPR();
		float healthPercent = damageMan.IRRU_GetHealthPercentage();
		float bloodPercent = damageMan.IRRU_GetBloodPercentage();
		float resiliencePercent = damageMan.IRRU_GetResiliencePercentage();
		float bleedingRateMLs = damageMan.IRRU_GetBleedingRateMLPerSecond();

		string damageIntensityText = "Healthy";
		int damageIntensityLevel = 0;
		Color damageColor = Color.FromSRGBA(255, 255, 0, 255);
		if (healthPercent < 34)
		{
			damageIntensityText = "Severely wounded";
			damageIntensityLevel = 4;
			damageColor = Color.FromSRGBA(255, 0, 0, 255);
		}
		else if (healthPercent < 50)
		{
			damageIntensityText = "Badly wounded";
			damageIntensityLevel = 3;
			damageColor = Color.FromSRGBA(255, 150, 0, 255);
		}
		else if (healthPercent < 75)
		{
			damageIntensityText = "Wounded";
			damageIntensityLevel = 2;
			damageColor = Color.FromSRGBA(255, 200, 0, 255);
		}
		else if (healthPercent < 100)
		{
			damageIntensityText = "Minor injuries";
			damageIntensityLevel = 1;
		}

		string damageIntensity = "";
		if (damageIntensityLevel > 0)
			damageIntensity = string.Format("Wound_%1_UI", damageIntensityLevel);

		string bloodStatus = "Lost some blood";
		Color bloodColor = Color.FromSRGBA(255, 255, 0, 255);
		if (bloodPercent < 33)
		{
			bloodStatus = "Critical blood loss";
			bloodColor = Color.FromSRGBA(255, 0, 0, 255);
		}
		else if (bloodPercent < 40)
		{
			bloodStatus = "Massive blood loss";
			bloodColor = Color.FromSRGBA(255, 150, 0, 255);
		}
		else if (bloodPercent < 60)
		{
			bloodStatus = "Lost a lot of blood";
			bloodColor = Color.FromSRGBA(255, 200, 0, 255);
		}

		string bleedingIntensityText = bloodStatus;
		if (bleedingRateMLs > 0.1)
			bleedingIntensityText = string.Format("%1 (-%2 ml/s)", bloodStatus, Math.Round(bleedingRateMLs * 10) / 10);

		if (isUnconscious)
		{
			float bleedoutTimeRemaining = nid.GetBleedoutTimeRemaining();
			if (bleedoutTimeRemaining > 0)
			{
				float percentRemaining = bleedoutTimeRemaining / IRRU_NoInstantDeathSettings.GetBleedoutTime() * 100.0;
				string timeText;
				if (IRRU_NoInstantDeathSettings.IsDescriptiveTimerEnabled())
				{
					timeText = IRRU_GetTriageLevel(percentRemaining);
					if (receivingCPR)
						timeText = timeText + " - CPR";
				}
				else
				{
					timeText = IRRU_FormatBleedoutTime(bleedoutTimeRemaining);
				}

				sName = string.Format("%1 [<color rgba='%2'>%3</color>]", sName, IRRU_GetTriageRGBA(percentRemaining), timeText);
			}
			else
			{
				sName = sName + " [UNCONSCIOUS]";
			}
		}

		bool isTourniquetted = false;
		bool isSalineBagged = false;
		bool regenerating = false;
		array<ECharacterHitZoneGroup> limbGroups = {};
		damageMan.GetAllLimbs(limbGroups);
		foreach (ECharacterHitZoneGroup group : limbGroups)
		{
			if (damageMan.GetGroupTourniquetted(group))
				isTourniquetted = true;
			if (damageMan.GetGroupSalineBagged(group))
				isSalineBagged = true;
			if (damageMan.GetGroupDamageOverTime(group, EDamageType.HEALING) != 0 || damageMan.GetGroupDamageOverTime(group, EDamageType.REGENERATION) != 0)
				regenerating = true;
		}
		bool isMorphined = !damageMan.GetAllPersistentEffectsOfType(SCR_MorphineDamageEffect).IsEmpty();

		if (m_DamageInfoUI)
		{
			m_DamageInfoUI.SetName(sName);
			m_DamageInfoUI.SetDamageStateVisible(damageIntensityLevel, regenerating, damageIntensity, damageIntensityText);
			m_DamageInfoUI.SetBleedingStateVisible(bleedingRateMLs > 0 || bloodPercent < 100, bleedingIntensityText);
			m_DamageInfoUI.SetTourniquetStateVisible(isTourniquetted);
			m_DamageInfoUI.SetSalineBagStateVisible(isSalineBagged);
			m_DamageInfoUI.SetMorphineStateVisible(isMorphined);
			m_DamageInfoUI.SetFractureStateVisible(0, 0);
		}

		if (m_wDamageText && damageIntensityLevel > 0)
			m_wDamageText.SetColor(damageColor);
		if (m_wBleedingText && bloodPercent < 100)
			m_wBleedingText.SetColor(bloodColor);

		if (m_wResilienceText)
		{
			m_wResilienceText.SetVisible(resiliencePercent >= 0);
			if (resiliencePercent >= 0)
			{
				string resilienceText = "Fully responsive";
				if (resiliencePercent < 33)
					resilienceText = "Unconscious";
				else if (resiliencePercent <= 59)
					resilienceText = "Fading";
				else if (resiliencePercent <= 99)
					resilienceText = "Dazed";

				Color resilienceColor = Color.FromSRGBA(0, 255, 0, 255);
				if (resiliencePercent < 25)
					resilienceColor = Color.FromSRGBA(255, 0, 0, 255);
				else if (resiliencePercent < 50)
					resilienceColor = Color.FromSRGBA(255, 165, 0, 255);
				else if (resiliencePercent < 75)
					resilienceColor = Color.FromSRGBA(255, 255, 0, 255);

				m_wResilienceText.SetText(resilienceText);
				m_wResilienceText.SetColor(resilienceColor);
			}
		}

		if (m_wCPRStatusText)
		{
			m_wCPRStatusText.SetVisible(isUnconscious || receivingCPR);
			if (receivingCPR)
			{
				m_wCPRStatusText.SetText("CPR IN PROGRESS");
				m_wCPRStatusText.SetColor(Color.FromSRGBA(200, 100, 255, 255));
			}
			else if (isUnconscious)
			{
				m_wCPRStatusText.SetText("No CPR");
				m_wCPRStatusText.SetColor(Color.FromSRGBA(128, 128, 128, 255));
			}
		}
	}
}
