// ============================================================================
//  SCR_InspectCasualtyWidget.c - Modified for 506thIRRUMedical
//  Shows exact health, blood, and resilience percentages
// ============================================================================

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
	
	// NEW: Additional widgets for enhanced display
	protected TextWidget m_wResilienceText;
	protected RichTextWidget m_wDetailedStatus;

	//------------------------------------------------------------------------------------------------
	override void DisplayStartDraw(IEntity owner)
	{
		m_wCasualtyInspectWidget = GetRootWidget();
		
		// NEW: Find additional widgets if they exist in the layout
		if (m_wCasualtyInspectWidget)
		{
			m_wResilienceText = TextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("ResilienceText"));
			m_wDetailedStatus = RichTextWidget.Cast(m_wCasualtyInspectWidget.FindAnyWidget("DetailedStatus"));
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
	//! Gather and update data of target character into widget - MODIFIED FOR PERCENTAGES
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

		// NEW: Get all percentage data using our new methods
		float healthPercent, bloodPercent, resiliencePercent, bleedingRateMLs;
		bool hasResilience, isUnconscious;
		
		damageMan.GetDetailedMedicalStatus(healthPercent, bloodPercent, resiliencePercent, 
		                                   hasResilience, bleedingRateMLs, isUnconscious);
		
		// Format the display texts with exact percentages
		string damageIntensityText = string.Format("Health: %1%%", Math.Round(healthPercent));
		string damageIntensity = ""; // Icon name
		
		// Determine icon based on health
		int damageIntensityLevel = 0;
		if (healthPercent < 10)
		{
			damageIntensityLevel = 4;
			damageIntensity = "Wound_4_UI";
		}
		else if (healthPercent < 25)
		{
			damageIntensityLevel = 3;
			damageIntensity = "Wound_3_UI";
		}
		else if (healthPercent < 50)
		{
			damageIntensityLevel = 2;
			damageIntensity = "Wound_2_UI";
		}
		else if (healthPercent < 75)
		{
			damageIntensityLevel = 1;
			damageIntensity = "Wound_1_UI";
		}
		
		// Format bleeding text with exact percentages
		string bleedingIntensityText;
		if (bleedingRateMLs > 0.1)
		{
			bleedingIntensityText = string.Format("Blood: %1%% (-%2 ml/s)", 
			                                      Math.Round(bloodPercent), 
			                                      Math.Round(bleedingRateMLs * 10) / 10);
		}
		else
		{
			bleedingIntensityText = string.Format("Blood: %1%%", Math.Round(bloodPercent));
		}
		
		// Add unconscious state to name if applicable
		if (isUnconscious)
		{
			sName = sName + " [UNCONSCIOUS]";
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
		}
		
		// NEW: Update resilience text widget if it exists
		if (m_wResilienceText && hasResilience)
		{
			string resilienceText = string.Format("Resilience: %1%%", Math.Round(resiliencePercent));
			m_wResilienceText.SetText(resilienceText);
			m_wResilienceText.SetVisible(true);
		}
		else if (m_wResilienceText)
		{
			m_wResilienceText.SetVisible(false);
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