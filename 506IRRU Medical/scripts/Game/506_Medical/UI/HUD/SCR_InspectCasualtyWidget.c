//! Enhanced casualty inspection widget with detailed medical status

modded class SCR_InspectCasualtyWidget : SCR_InfoDisplayExtended
{
	// Keep all original member variables
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

	//------------------------------------------------------------------------------------------------
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

	//------------------------------------------------------------------------------------------------
	override event void DisplayUpdate(IEntity owner, float timeSlice)
	{
		if (m_fTimeTillClose < 0)
			DisableWidget();
		else
			m_fTimeTillClose -= timeSlice;

		if (m_fTimeTillUpdate > 0)
		{
			m_fTimeTillUpdate -= timeSlice;
		}
		else
		{
			m_fTimeTillUpdate = UPDATE_FREQ;
			UpdateTarget();
		}

		UpdateWidget();
	}

	//------------------------------------------------------------------------------------------------
	//! Start showing the widget
	override bool ShowInspectCasualtyWidget(IEntity targetCharacter)
	{
		if (!m_wCasualtyInspectWidget)
			return false;

		ChimeraCharacter char = ChimeraCharacter.Cast(targetCharacter);
		if (!char)
			return false;

		CharacterControllerComponent targetController = char.GetCharacterController();
		if (!targetController)
			return false;

		UpdateTarget();
		EnableWidget();

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Check if target is still alive and update widget if so
	override protected void UpdateTarget()
	{
		if (!m_Target)
		{
			DisableWidget();
			return;
		}

		ChimeraCharacter char = ChimeraCharacter.Cast(m_Target);
		if (!char)
			return;

		CharacterControllerComponent controller = char.GetCharacterController();
		if (controller.GetLifeState() == ECharacterLifeState.DEAD)
		{
			DisableWidget();
			return;
		}

		UpdateWidgetData();
	}

	//------------------------------------------------------------------------------------------------
	//! Helper method to get health status color based on percentage
	protected Color GetHealthStatusColor(float healthPercent)
	{
		if (healthPercent >= 75)
			return Color.FromSRGBA(255, 255, 0, 255); // Yellow
		else if (healthPercent >= 50)
			return Color.FromSRGBA(255, 200, 0, 255); // Yellow-orange
		else if (healthPercent >= 34)
			return Color.FromSRGBA(255, 150, 0, 255); // Orange
		else
			return Color.FromSRGBA(255, 0, 0, 255); // Red
	}

	//------------------------------------------------------------------------------------------------
	//! Helper method to get blood status color based on percentage
	protected Color GetBloodStatusColor(float bloodPercent)
	{
		if (bloodPercent >= 60)
			return Color.FromSRGBA(255, 255, 0, 255); // Yellow
		else if (bloodPercent >= 40)
			return Color.FromSRGBA(255, 200, 0, 255); // Yellow-orange
		else if (bloodPercent >= 34)
			return Color.FromSRGBA(255, 150, 0, 255); // Orange
		else
			return Color.FromSRGBA(255, 0, 0, 255); // Red
	}

	//------------------------------------------------------------------------------------------------
	//! Gather and update data of target character into widget - MODIFIED FOR PERCENTAGES AND TIMER
	override protected void UpdateWidgetData()
	{
		if (!m_Target || !m_wCasualtyInspectWidget)
			return;

		string sName;
		GetCasualtyName(sName, m_Target);

		// Platform-specific formatting
		if (GetGame().GetPlatformService().GetLocalPlatformKind() == PlatformKind.PSN)
		{
			PlayerManager playerMgr = GetGame().GetPlayerManager();
			if (playerMgr && playerMgr.GetPlatformKind(playerMgr.GetPlayerIdFromControlledEntity(m_Target)) == PlatformKind.PSN)
				sName = string.Format("<color rgba=%1><image set='%2' name='%3' scale='%4'/></color>", UIColors.FormatColor(GUIColors.ENABLED), UIConstants.ICONS_IMAGE_SET, UIConstants.PLATFROM_PLAYSTATION_ICON_NAME, 2) + sName;
			else
				sName = string.Format("<color rgba=%1><image set='%2' name='%3' scale='%4'/></color>", UIColors.FormatColor(GUIColors.ENABLED), UIConstants.ICONS_IMAGE_SET, UIConstants.PLATFROM_GENERIC_ICON_NAME, 2) + sName;
		}

		// Get damage manager
		ChimeraCharacter character = ChimeraCharacter.Cast(m_Target);
		if (!character)
			return;
			
		SCR_CharacterDamageManagerComponent damageMan = SCR_CharacterDamageManagerComponent.Cast(character.GetDamageManager());
		if (!damageMan)
			return;

		// NEW: Get all percentage data including timer using our new methods
		float healthPercent, bloodPercent, resiliencePercent, bleedingRateMLs;
		float bleedoutTimeRemaining;
		bool hasResilience, isUnconscious, isBleedingOut;
		
		damageMan.GetDetailedMedicalStatus(healthPercent, bloodPercent, resiliencePercent, 
		                                   hasResilience, bleedingRateMLs, isUnconscious,
		                                   bleedoutTimeRemaining, isBleedingOut);
		
		// Format the display texts with descriptive health status
		string damageIntensityText;
		string damageIntensity = ""; // Icon name

		// Determine health status description and icon (5 levels)
		int damageIntensityLevel = 0;
		if (healthPercent >= 100)
		{
			damageIntensityText = "Healthy";
			damageIntensityLevel = 0;
			// No icon for healthy state
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
		
		// Format bleeding text with descriptive blood loss status (4 levels)
		string bleedingIntensityText;
		string bloodStatus;

		if (bloodPercent >= 60)
		{
			bloodStatus = "Lost some blood";
		}
		else if (bloodPercent >= 40)
		{
			bloodStatus = "Lost a lot of blood";
		}
		else if (bloodPercent >= 34)
		{
			bloodStatus = "Massive blood loss";
		}
		else
		{
			bloodStatus = "Critical blood loss";
		}

		if (bleedingRateMLs > 0.1)
		{
			bleedingIntensityText = string.Format("%1 (-%2 ml/s)",
			                                      bloodStatus,
			                                      Math.Round(bleedingRateMLs * 10) / 10);
		}
		else
		{
			bleedingIntensityText = bloodStatus;
		}
		
		// Add unconscious state and timer to name if applicable
		if (isUnconscious)
		{
			if (isBleedingOut && bleedoutTimeRemaining > 0)
			{
				string timeText;
				
				// Get total time for percentage calculation
				float totalBleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
				IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(character.FindComponent(IRRU_NoInstantDeathComponent));
				if (nid)
					totalBleedoutTime = nid.GetBleedoutTimeTotal();
				float percentRemaining = (bleedoutTimeRemaining / totalBleedoutTime) * 100.0;
				
				// Check if we should use descriptive or exact timer
				if (IRRU_NoInstantDeathSettings.IsDescriptiveTimerEnabled())
				{
					// Generate descriptive text based on time remaining - 5 level system
					if (percentRemaining > 70)
						timeText = "DELAYED";
					else if (percentRemaining > 50)
						timeText = "PRIORITY";
					else if (percentRemaining > 30)
						timeText = "URGENT";
					else if (percentRemaining > 15)
						timeText = "CRITICAL";
					else
						timeText = "IMMEDIATE";

					// Add CPR indicator if receiving CPR
					if (nid && nid.IsReceivingCPR())
						timeText = timeText + " - CPR";
				}
				else
				{
					// Use exact timer format MM:SS
					int minutes = Math.Floor(bleedoutTimeRemaining / 60);
					int seconds = Math.Floor(Math.Mod(bleedoutTimeRemaining, 60));
					timeText = string.Format("%1:%2", minutes, seconds.ToString(2));
				}
				
				// Color code based on time remaining - 5 level system
				if (percentRemaining > 70)
				{
					// Green for DELAYED (>70%)
					sName = string.Format("%1 [<color rgba='0,255,100,255'>%2</color>]", sName, timeText);
				}
				else if (percentRemaining > 50)
				{
					// Yellow for PRIORITY (50-70%)
					sName = string.Format("%1 [<color rgba='255,255,0,255'>%2</color>]", sName, timeText);
				}
				else if (percentRemaining > 30)
				{
					// Yellow-orange for URGENT (30-50%)
					sName = string.Format("%1 [<color rgba='255,200,0,255'>%2</color>]", sName, timeText);
				}
				else if (percentRemaining > 15)
				{
					// Orange for CRITICAL (15-30%)
					sName = string.Format("%1 [<color rgba='255,150,0,255'>%2</color>]", sName, timeText);
				}
				else
				{
					// Red for IMMEDIATE (<15%)
					sName = string.Format("%1 [<color rgba='255,0,0,255'>%2</color>]", sName, timeText);
				}
			}
			else
			{
				sName = sName + " [UNCONSCIOUS]";
			}
		}
		
		// Get medical treatment states
		bool isTourniquetted = false, isSalineBagged = false, isMorphined = false, regenerating = false;
		array<ECharacterHitZoneGroup> tourniquettedLimbs = {};
		
		// Check each limb for treatments
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
		
		// Check morphine
		array<ref SCR_PersistentDamageEffect> effects = damageMan.GetAllPersistentEffectsOfType(SCR_MorphineDamageEffect);
		isMorphined = !effects.IsEmpty();
		
		// Update the main UI
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

			// Apply colors directly to the text widgets
			Widget damageTextWidget = m_wCasualtyInspectWidget.FindAnyWidget("DamageInfo_text");
			if (damageTextWidget && healthPercent < 100)
			{
				TextWidget damageText = TextWidget.Cast(damageTextWidget);
				if (damageText)
				{
					Color healthColor = GetHealthStatusColor(healthPercent);
					damageText.SetColor(healthColor);
				}
			}

			Widget bleedingTextWidget = m_wCasualtyInspectWidget.FindAnyWidget("BleedingInfo_text");
			if (bleedingTextWidget && bloodPercent < 100)
			{
				TextWidget bleedingText = TextWidget.Cast(bleedingTextWidget);
				if (bleedingText)
				{
					Color bloodColor = GetBloodStatusColor(bloodPercent);
					bleedingText.SetColor(bloodColor);
				}
			}
		}
		
		// NEW: Update resilience text widget if it exists
		if (m_wResilienceText && hasResilience)
		{
			string resilienceText;

			// Determine resilience status description (4 levels)
			if (resiliencePercent < 33)
			{
				resilienceText = "Unconscious";
			}
			else if (resiliencePercent <= 59)
			{
				resilienceText = "Fading";
			}
			else if (resiliencePercent <= 99)
			{
				resilienceText = "Dazed";
			}
			else
			{
				resilienceText = "Fully responsive";
			}

			m_wResilienceText.SetText(resilienceText);

			// Set color based on percentage
			Color resilienceDisplayColor;
			if (resiliencePercent >= 75)
				resilienceDisplayColor = Color.FromSRGBA(0, 255, 0, 255); // Green
			else if (resiliencePercent >= 50)
				resilienceDisplayColor = Color.FromSRGBA(255, 255, 0, 255); // Yellow
			else if (resiliencePercent >= 25)
				resilienceDisplayColor = Color.FromSRGBA(255, 165, 0, 255); // Orange
			else
				resilienceDisplayColor = Color.FromSRGBA(255, 0, 0, 255); // Red

			m_wResilienceText.SetColor(resilienceDisplayColor);
			m_wResilienceText.SetVisible(true);
		}
		else if (m_wResilienceText)
		{
			m_wResilienceText.SetVisible(false);
		}
		
		// NEW: Update separate bleedout timer widget if it exists
		if (m_wBleedoutTimerText)
		{
			if (isBleedingOut && bleedoutTimeRemaining > 0)
			{
				string timeText;
				
				// Get total time for percentage calculation
				float totalBleedoutTime = IRRU_NoInstantDeathSettings.GetBleedoutTime();
				IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(character.FindComponent(IRRU_NoInstantDeathComponent));
				if (nid)
					totalBleedoutTime = nid.GetBleedoutTimeTotal();
				float percentRemaining = (bleedoutTimeRemaining / totalBleedoutTime) * 100.0;
				
				// Check if we should use descriptive or exact timer
				if (IRRU_NoInstantDeathSettings.IsDescriptiveTimerEnabled())
				{
					// Generate descriptive text based on time remaining - 5 level system
					if (percentRemaining > 70)
						timeText = "Condition: DELAYED";
					else if (percentRemaining > 50)
						timeText = "Condition: PRIORITY";
					else if (percentRemaining > 30)
						timeText = "Condition: URGENT";
					else if (percentRemaining > 15)
						timeText = "Condition: CRITICAL";
					else
						timeText = "Condition: IMMEDIATE";
				}
				else
				{
					// Use exact timer format
					int minutes = Math.Floor(bleedoutTimeRemaining / 60);
					int seconds = Math.Floor(Math.Mod(bleedoutTimeRemaining, 60));
					timeText = string.Format("Bleedout: %1:%2", minutes, seconds.ToString(2));
				}
				
				// Color based on time - 5 level system
				Color timerColor;
				if (percentRemaining > 70)
					timerColor = Color.FromSRGBA(0, 255, 100, 255); // Green for DELAYED
				else if (percentRemaining > 50)
					timerColor = Color.FromSRGBA(255, 255, 0, 255); // Yellow for PRIORITY
				else if (percentRemaining > 30)
					timerColor = Color.FromSRGBA(255, 200, 0, 255); // Yellow-orange for URGENT
				else if (percentRemaining > 15)
					timerColor = Color.FromSRGBA(255, 150, 0, 255); // Orange for CRITICAL
				else
					timerColor = Color.FromSRGBA(255, 0, 0, 255); // Red for IMMEDIATE
					
				m_wBleedoutTimerText.SetText(timeText);
				m_wBleedoutTimerText.SetColor(timerColor);
				m_wBleedoutTimerText.SetVisible(true);
			}
			else
			{
				m_wBleedoutTimerText.SetVisible(false);
			}
		}
		
		// NEW: Show which limbs have tourniquets if detail widget exists
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
		
		// NEW: Show CPR status
		if (m_wCPRStatusText)
		{
			IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(character.FindComponent(IRRU_NoInstantDeathComponent));
			if (nid && nid.IsReceivingCPR())
			{
				m_wCPRStatusText.SetText("CPR IN PROGRESS");
				m_wCPRStatusText.SetColor(Color.FromSRGBA(200, 100, 255, 255)); // Purple
				m_wCPRStatusText.SetVisible(true);
			}
			else if (nid && nid.IsUnconscious())
			{
				m_wCPRStatusText.SetText("No CPR");
				m_wCPRStatusText.SetColor(Color.FromSRGBA(128, 128, 128, 255)); // Gray
				m_wCPRStatusText.SetVisible(true);
			}
			else
			{
				m_wCPRStatusText.SetVisible(false);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Update widget position and opacity
	override protected void UpdateWidget()
	{
		if (!m_Target || !m_wCasualtyInspectWidget || !m_bIsEnabled)
			return;

		vector boneVector[4];
		m_Target.GetAnimation().GetBoneMatrix(m_Target.GetAnimation().GetBoneIndex(TARGET_BONE), boneVector);

		vector WPPos = boneVector[3] + m_Target.GetOrigin();
		vector pos = GetGame().GetWorkspace().ProjWorldToScreen(WPPos, GetGame().GetWorld());

		// Handle off-screen coords
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int winX = workspace.GetWidth();
		int winY = workspace.GetHeight();
		int posX = workspace.DPIScale(pos[0]);
		int posY = workspace.DPIScale(pos[1]);

		// If widget off screen, remove widget
		if (posX < 0 || posX > winX || posY > winY || posY < 0)
		{
			DisableWidget();
			return;
		}

		FrameSlot.SetPos(m_wCasualtyInspectWidget.GetChildren(), pos[0], pos[1]);

		float dist = vector.Distance(GetGame().GetPlayerController().GetControlledEntity().GetOrigin(), WPPos);
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

	//------------------------------------------------------------------------------------------------
	// REMOVED old GetDamageInfo method - now using damage manager's percentage methods directly

	//------------------------------------------------------------------------------------------------
	override protected void GetCasualtyName(inout string sName, IEntity targetCharacter)
	{
		string sFormat, sAlias, sSurname;
		int playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(targetCharacter);
		if (playerID > 0)
		{
			PlayerManager playerMgr = GetGame().GetPlayerManager();
			if (playerMgr)
				sName = SCR_PlayerNamesFilterCache.GetInstance().GetPlayerDisplayName(playerID);
		}
		else
		{
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

	//------------------------------------------------------------------------------------------------
	override protected void DisplayOnSuspended()
	{
		DisableWidget();
	}

	//------------------------------------------------------------------------------------------------
	override void SetTarget(IEntity target)
	{
		m_Target = target;
	}

	//------------------------------------------------------------------------------------------------
	override bool IsActive()
	{
		return m_Target && m_wCasualtyInspectWidget && m_wCasualtyInspectWidget.GetOpacity() != 0;
	}

	//------------------------------------------------------------------------------------------------
	override protected void DisableWidget()
	{
		if (m_wCasualtyInspectWidget)
			m_wCasualtyInspectWidget.SetVisible(false);

		m_Target = null;
		SetEnabled(false);
		m_bShouldBeVisible = false;
		m_fTimeTillClose = MAX_SHOW_DURATION;
	}

	//------------------------------------------------------------------------------------------------
	override protected void EnableWidget()
	{
		if (m_wCasualtyInspectWidget)
			m_wCasualtyInspectWidget.SetVisible(true);

		SetEnabled(true);
		m_bShouldBeVisible = true;
	}

	//------------------------------------------------------------------------------------------------
	override void DisplayOnResumed()
	{
		if (!m_bShouldBeVisible && m_wCasualtyInspectWidget)
			m_wCasualtyInspectWidget.SetVisible(false);
	}
}