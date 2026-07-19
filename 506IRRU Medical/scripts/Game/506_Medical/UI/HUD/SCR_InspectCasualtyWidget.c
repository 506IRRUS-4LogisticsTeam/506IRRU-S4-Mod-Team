modded class SCR_InspectCasualtyWidget : SCR_InfoDisplayExtended
{
	const ResourceName INSPECT_CASUALTY_LAYOUT = "{11AC7D61FD4CF3F6}UI/layouts/Damage/InspectCasualtyMenu.layout";

	protected SCR_CharacterDamageManagerComponent m_CharDamageManager;
	protected Widget m_wCasualtyInspectWidget;
	IEntity m_Target;

	protected const float UPDATE_FREQ = 0.5;
	protected const float MAX_SHOW_DURATION = 5;
	protected const string TARGET_BONE = "Spine4";
	protected float m_fTimeTillUpdate;
	protected float m_fTimeTillClose;
	protected bool m_bShouldBeVisible;

	protected TextWidget m_wResilienceText;
	protected RichTextWidget m_wDetailedStatus;
	protected TextWidget m_wBleedoutTimerText;
	protected TextWidget m_wCPRStatusText;

	override void DisplayStartDraw(IEntity owner)
	{
		m_wCasualtyInspectWidget = GetRootWidget();
		if (m_wCasualtyInspectWidget)
		{
			m_wResilienceText = TextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("ResilienceText"));
			m_wDetailedStatus = RichTextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("DetailedStatus"));
			m_wBleedoutTimerText = TextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("BleedoutTimerText"));
			m_wCPRStatusText = TextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("CPRStatusText"));
		}
		DisableWidget();
	}

	override event void DisplayUpdate(IEntity owner, float timeSlice)
	{
		if (m_fTimeTillClose < 0)
			DisableWidget();
		else
			m_fTimeTillClose -= timeSlice;

		if (m_fTimeTillUpdate > 0)
			m_fTimeTillUpdate -= timeSlice;
		else
		{
			m_fTimeTillUpdate = UPDATE_FREQ;
			UpdateTarget();
		}
		UpdateWidget();
	}

	override bool ShowInspectCasualtyWidget(IEntity targetCharacter)
	{
		if (!m_wCasualtyInspectWidget)
			return false;

		ChimeraCharacter ch = ChimeraCharacter.Cast(targetCharacter);
		if (!ch)
			return false;

		if (!ch.GetCharacterController())
			return false;

		UpdateTarget();
		EnableWidget();
		return true;
	}

	override protected void UpdateTarget()
	{
		if (!m_Target)
		{
			DisableWidget();
			return;
		}

		ChimeraCharacter ch = ChimeraCharacter.Cast(m_Target);
		if (!ch)
			return;

		CharacterControllerComponent controller = ch.GetCharacterController();
		if (controller.GetLifeState() == ECharacterLifeState.DEAD)
		{
			DisableWidget();
			return;
		}
		UpdateWidgetData();
	}

	protected Color IRRU_GetHealthStatusColor(float healthPercent)
	{
		if (healthPercent >= 75)
			return Color.FromSRGBA(255, 255, 0, 255);
		else if (healthPercent >= 50)
			return Color.FromSRGBA(255, 200, 0, 255);
		else if (healthPercent >= 34)
			return Color.FromSRGBA(255, 150, 0, 255);
		return Color.FromSRGBA(255, 0, 0, 255);
	}

	protected Color IRRU_GetBloodStatusColor(float bloodPercent)
	{
		if (bloodPercent >= 60)
			return Color.FromSRGBA(255, 255, 0, 255);
		else if (bloodPercent >= 40)
			return Color.FromSRGBA(255, 200, 0, 255);
		else if (bloodPercent >= 33)
			return Color.FromSRGBA(255, 150, 0, 255);
		return Color.FromSRGBA(255, 0, 0, 255);
	}

	protected string IRRU_GetTriageLevel(float percentRemaining)
	{
		if (percentRemaining > 70) return "DELAYED";
		else if (percentRemaining > 50) return "PRIORITY";
		else if (percentRemaining > 30) return "URGENT";
		else if (percentRemaining > 15) return "CRITICAL";
		return "IMMEDIATE";
	}

	protected Color IRRU_GetTriageColor(float percentRemaining)
	{
		if (percentRemaining > 70)
			return Color.FromSRGBA(0, 255, 100, 255);
		else if (percentRemaining > 50)
			return Color.FromSRGBA(255, 255, 0, 255);
		else if (percentRemaining > 30)
			return Color.FromSRGBA(255, 200, 0, 255);
		else if (percentRemaining > 15)
			return Color.FromSRGBA(255, 150, 0, 255);
		return Color.FromSRGBA(255, 0, 0, 255);
	}

	protected string IRRU_GetTriageRGBA(float percentRemaining)
	{
		if (percentRemaining > 70) return "0,255,100,255";
		else if (percentRemaining > 50) return "255,255,0,255";
		else if (percentRemaining > 30) return "255,200,0,255";
		else if (percentRemaining > 15) return "255,150,0,255";
		return "255,0,0,255";
	}

	protected float IRRU_GetBleedoutPercentRemaining(IEntity character, float bleedoutTimeRemaining)
	{
		float totalBleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(character.FindComponent(IRRU_NoInstantDeathComponent));
		if (nid)
			totalBleedoutTime = nid.GetBleedoutTimeTotal();
		return (bleedoutTimeRemaining / totalBleedoutTime) * 100.0;
	}

	protected string IRRU_FormatBleedoutTime(float bleedoutTimeRemaining)
	{
		int minutes = Math.Floor(bleedoutTimeRemaining / 60);
		int seconds = Math.Floor(Math.Mod(bleedoutTimeRemaining, 60));
		return string.Format("%1:%2", minutes, seconds.ToString(2));
	}

	override protected void UpdateWidgetData()
	{
		if (!m_Target || !m_wCasualtyInspectWidget)
			return;

		string sName;
		GetCasualtyName(sName, m_Target);

		if (GetGame().GetPlatformService().GetLocalPlatformKind() == PlatformKind.PSN)
		{
			PlayerManager playerMgr = GetGame().GetPlayerManager();
			if (playerMgr && playerMgr.GetPlatformKind(playerMgr.GetPlayerIdFromControlledEntity(m_Target)) == PlatformKind.PSN)
				sName = string.Format("<color rgba=%1><image set='%2' name='%3' scale='%4'/></color>", UIColors.FormatColor(GUIColors.ENABLED), UIConstants.ICONS_IMAGE_SET, UIConstants.PLATFROM_PLAYSTATION_ICON_NAME, 2) + sName;
			else
				sName = string.Format("<color rgba=%1><image set='%2' name='%3' scale='%4'/></color>", UIColors.FormatColor(GUIColors.ENABLED), UIConstants.ICONS_IMAGE_SET, UIConstants.PLATFROM_GENERIC_ICON_NAME, 2) + sName;
		}

		ChimeraCharacter character = ChimeraCharacter.Cast(m_Target);
		if (!character)
			return;

		SCR_CharacterDamageManagerComponent damageMan = SCR_CharacterDamageManagerComponent.Cast(character.GetDamageManager());
		if (!damageMan)
			return;

		float healthPercent, bloodPercent, resiliencePercent, bleedingRateMLs, bleedoutTimeRemaining;
		bool hasResilience, isUnconscious, isBleedingOut;
		damageMan.IRRU_GetDetailedMedicalStatus(healthPercent, bloodPercent, resiliencePercent,
			hasResilience, bleedingRateMLs, isUnconscious, bleedoutTimeRemaining, isBleedingOut);

		string damageIntensityText;
		string damageIntensity = "";
		int damageIntensityLevel = 0;

		if (healthPercent >= 100)
		{
			damageIntensityText = "Healthy";
		}
		else if (healthPercent >= 75)
		{
			damageIntensityText = "Minor injuries";
			damageIntensityLevel = 1;
			damageIntensity = "Wound_1_UI";
		}
		else if (healthPercent >= 50)
		{
			damageIntensityText = "Wounded";
			damageIntensityLevel = 2;
			damageIntensity = "Wound_2_UI";
		}
		else if (healthPercent >= 34)
		{
			damageIntensityText = "Badly wounded";
			damageIntensityLevel = 3;
			damageIntensity = "Wound_3_UI";
		}
		else
		{
			damageIntensityText = "Severely wounded";
			damageIntensityLevel = 4;
			damageIntensity = "Wound_4_UI";
		}

		string bloodStatus;
		if (bloodPercent >= 60)
			bloodStatus = "Lost some blood";
		else if (bloodPercent >= 40)
			bloodStatus = "Lost a lot of blood";
		else if (bloodPercent >= 33)
			bloodStatus = "Massive blood loss";
		else
			bloodStatus = "Critical blood loss";

		string bleedingIntensityText;
		if (bleedingRateMLs > 0.1)
			bleedingIntensityText = string.Format("%1 (-%2 ml/s)", bloodStatus, Math.Round(bleedingRateMLs * 10) / 10);
		else
			bleedingIntensityText = bloodStatus;

		if (isUnconscious && isBleedingOut && bleedoutTimeRemaining > 0)
		{
			float percentRemaining = IRRU_GetBleedoutPercentRemaining(character, bleedoutTimeRemaining);

			string timeText;
			if (IRRU_NoInstantDeathSettings.IsDescriptiveTimerEnabled())
			{
				timeText = IRRU_GetTriageLevel(percentRemaining);
				IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(character.FindComponent(IRRU_NoInstantDeathComponent));
				if (nid && nid.IsReceivingCPR())
					timeText = timeText + " - CPR";
			}
			else
			{
				timeText = IRRU_FormatBleedoutTime(bleedoutTimeRemaining);
			}

			sName = string.Format("%1 [<color rgba='%2'>%3</color>]", sName, IRRU_GetTriageRGBA(percentRemaining), timeText);
		}
		else if (isUnconscious)
		{
			sName = sName + " [UNCONSCIOUS]";
		}

		bool isTourniquetted = false, isSalineBagged = false, isMorphined = false, regenerating = false;
		array<ECharacterHitZoneGroup> tourniquettedLimbs = {};

		array<ECharacterHitZoneGroup> limbGroups = {};
		damageMan.GetAllLimbs(limbGroups);

		foreach (ECharacterHitZoneGroup group : limbGroups)
		{
			if (damageMan.GetGroupTourniquetted(group))
			{
				isTourniquetted = true;
				tourniquettedLimbs.Insert(group);
			}
			if (!isSalineBagged)
				isSalineBagged = damageMan.GetGroupSalineBagged(group);
			if (!regenerating)
				regenerating = damageMan.GetGroupDamageOverTime(group, EDamageType.HEALING) != 0 ||
					damageMan.GetGroupDamageOverTime(group, EDamageType.REGENERATION) != 0;
		}

		array<ref SCR_PersistentDamageEffect> effects = damageMan.GetAllPersistentEffectsOfType(SCR_MorphineDamageEffect);
		isMorphined = !effects.IsEmpty();

		SCR_InventoryDamageInfoUI damageInfoUI = SCR_InventoryDamageInfoUI.Cast(m_wCasualtyInspectWidget.FindHandler(SCR_InventoryDamageInfoUI));
		if (damageInfoUI)
		{
			damageInfoUI.SetName(sName);
			damageInfoUI.SetDamageStateVisible(damageIntensityLevel, regenerating, damageIntensity, damageIntensityText);
			damageInfoUI.SetBleedingStateVisible(bleedingRateMLs > 0 || bloodPercent < 100, bleedingIntensityText);
			damageInfoUI.SetTourniquetStateVisible(isTourniquetted);
			damageInfoUI.SetSalineBagStateVisible(isSalineBagged);
			damageInfoUI.SetMorphineStateVisible(isMorphined);
			damageInfoUI.SetFractureStateVisible(0, 0);

			Widget damageTextWidget = m_wCasualtyInspectWidget.FindAnyWidget("DamageInfo_text");
			if (damageTextWidget && healthPercent < 100)
			{
				TextWidget damageText = TextWidget.Cast(damageTextWidget);
				if (damageText)
					damageText.SetColor(IRRU_GetHealthStatusColor(healthPercent));
			}

			Widget bleedingTextWidget = m_wCasualtyInspectWidget.FindAnyWidget("BleedingInfo_text");
			if (bleedingTextWidget && bloodPercent < 100)
			{
				TextWidget bleedingText = TextWidget.Cast(bleedingTextWidget);
				if (bleedingText)
					bleedingText.SetColor(IRRU_GetBloodStatusColor(bloodPercent));
			}
		}

		if (m_wResilienceText && hasResilience)
		{
			string resilienceText;
			if (resiliencePercent < 33)
				resilienceText = "Unconscious";
			else if (resiliencePercent <= 59)
				resilienceText = "Fading";
			else if (resiliencePercent <= 99)
				resilienceText = "Dazed";
			else
				resilienceText = "Fully responsive";

			m_wResilienceText.SetText(resilienceText);

			Color resilienceColor;
			if (resiliencePercent >= 75)
				resilienceColor = Color.FromSRGBA(0, 255, 0, 255);
			else if (resiliencePercent >= 50)
				resilienceColor = Color.FromSRGBA(255, 255, 0, 255);
			else if (resiliencePercent >= 25)
				resilienceColor = Color.FromSRGBA(255, 165, 0, 255);
			else
				resilienceColor = Color.FromSRGBA(255, 0, 0, 255);

			m_wResilienceText.SetColor(resilienceColor);
			m_wResilienceText.SetVisible(true);
		}
		else if (m_wResilienceText)
		{
			m_wResilienceText.SetVisible(false);
		}

		if (m_wBleedoutTimerText)
		{
			if (isBleedingOut && bleedoutTimeRemaining > 0)
			{
				float percentRemaining = IRRU_GetBleedoutPercentRemaining(character, bleedoutTimeRemaining);

				string timeText;
				if (IRRU_NoInstantDeathSettings.IsDescriptiveTimerEnabled())
					timeText = string.Format("Condition: %1", IRRU_GetTriageLevel(percentRemaining));
				else
					timeText = string.Format("Bleedout: %1", IRRU_FormatBleedoutTime(bleedoutTimeRemaining));

				m_wBleedoutTimerText.SetText(timeText);
				m_wBleedoutTimerText.SetColor(IRRU_GetTriageColor(percentRemaining));
				m_wBleedoutTimerText.SetVisible(true);
			}
			else
			{
				m_wBleedoutTimerText.SetVisible(false);
			}
		}

		if (m_wDetailedStatus && isTourniquetted)
		{
			string tqText = "TQ: ";
			foreach (ECharacterHitZoneGroup limb : tourniquettedLimbs)
			{
				switch (limb)
				{
					case ECharacterHitZoneGroup.LEFTARM: tqText += "L.Arm "; break;
					case ECharacterHitZoneGroup.RIGHTARM: tqText += "R.Arm "; break;
					case ECharacterHitZoneGroup.LEFTLEG: tqText += "L.Leg "; break;
					case ECharacterHitZoneGroup.RIGHTLEG: tqText += "R.Leg "; break;
				}
			}
			m_wDetailedStatus.SetText(tqText);
			m_wDetailedStatus.SetVisible(true);
		}
		else if (m_wDetailedStatus)
		{
			m_wDetailedStatus.SetVisible(false);
		}

		if (m_wCPRStatusText)
		{
			IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(character.FindComponent(IRRU_NoInstantDeathComponent));
			if (nid && nid.IsReceivingCPR())
			{
				m_wCPRStatusText.SetText("CPR IN PROGRESS");
				m_wCPRStatusText.SetColor(Color.FromSRGBA(200, 100, 255, 255));
				m_wCPRStatusText.SetVisible(true);
			}
			else if (nid && nid.IsUnconscious())
			{
				m_wCPRStatusText.SetText("No CPR");
				m_wCPRStatusText.SetColor(Color.FromSRGBA(128, 128, 128, 255));
				m_wCPRStatusText.SetVisible(true);
			}
			else
			{
				m_wCPRStatusText.SetVisible(false);
			}
		}
	}

	override protected void UpdateWidget()
	{
		if (!m_Target || !m_wCasualtyInspectWidget || !m_bIsEnabled)
			return;

		vector boneVector[4];
		m_Target.GetAnimation().GetBoneMatrix(m_Target.GetAnimation().GetBoneIndex(TARGET_BONE), boneVector);

		vector WPPos = boneVector[3] + m_Target.GetOrigin();
		vector pos = GetGame().GetWorkspace().ProjWorldToScreen(WPPos, GetGame().GetWorld());

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int winX = workspace.GetWidth();
		int winY = workspace.GetHeight();
		int posX = workspace.DPIScale(pos[0]);
		int posY = workspace.DPIScale(pos[1]);

		if (posX < 0 || posX > winX || posY > winY || posY < 0)
		{
			DisableWidget();
			return;
		}

		FrameSlot.SetPos(m_wCasualtyInspectWidget.GetChildren(), pos[0], pos[1]);

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc || !pc.GetControlledEntity())
		{
			DisableWidget();
			return;
		}

		float dist = vector.Distance(pc.GetControlledEntity().GetOrigin(), WPPos);
		if (dist >= 4)
		{
			DisableWidget();
			return;
		}

		float distanceOpacityReduction = 1;
		if (dist <= 3)
			distanceOpacityReduction = 0;
		else
			distanceOpacityReduction = Math.InverseLerp(3, 4, dist);

		m_wCasualtyInspectWidget.SetOpacity(1 - distanceOpacityReduction);
	}

	override protected void GetCasualtyName(inout string sName, IEntity targetCharacter)
	{
		int playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(targetCharacter);
		if (playerID > 0)
		{
			PlayerManager playerMgr = GetGame().GetPlayerManager();
			if (playerMgr)
				sName = SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerID);
		}
		else
		{
			string sFormat, sAlias, sSurname;
			SCR_CharacterIdentityComponent scrCharIdentity = SCR_CharacterIdentityComponent.Cast(targetCharacter.FindComponent(SCR_CharacterIdentityComponent));
			if (scrCharIdentity)
			{
				scrCharIdentity.GetFormattedFullName(sFormat, sName, sAlias, sSurname);
				sName = sName + " " + sSurname;
			}
			else
			{
				CharacterIdentityComponent charIdentity = CharacterIdentityComponent.Cast(targetCharacter.FindComponent(CharacterIdentityComponent));
				if (charIdentity && charIdentity.GetIdentity())
					sName = charIdentity.GetIdentity().GetName();
			}
		}
	}

	override protected void DisplayOnSuspended() { DisableWidget(); }
	override void SetTarget(IEntity target) { m_Target = target; }

	override bool IsActive()
	{
		return m_Target && m_wCasualtyInspectWidget && m_wCasualtyInspectWidget.GetOpacity() != 0;
	}

	override protected void DisableWidget()
	{
		if (m_wCasualtyInspectWidget)
			m_wCasualtyInspectWidget.SetVisible(false);
		m_Target = null;
		SetEnabled(false);
		m_bShouldBeVisible = false;
		m_fTimeTillClose = MAX_SHOW_DURATION;
	}

	override protected void EnableWidget()
	{
		if (m_wCasualtyInspectWidget)
			m_wCasualtyInspectWidget.SetVisible(true);
		SetEnabled(true);
		m_bShouldBeVisible = true;
	}

	override void DisplayOnResumed()
	{
		if (!m_bShouldBeVisible && m_wCasualtyInspectWidget)
			m_wCasualtyInspectWidget.SetVisible(false);
	}
}
