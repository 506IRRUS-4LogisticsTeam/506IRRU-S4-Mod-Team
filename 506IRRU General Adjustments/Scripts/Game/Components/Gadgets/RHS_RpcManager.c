[EntityEditorProps(category: "GameScripted/Character", description: "Proxy radio rpc handling", color: "0 0 255 255")]
class RHS_RpcManagerClass : ScriptComponentClass
{
}
class RHS_RpcManager : ScriptComponent
{
	protected RHS_NightVisionComponent m_OwnedNVComponent;
	protected bool m_bIsCurrentNVNotAttachedToMe = false; //fe. true when using vehicle nv
	
	static RHS_RpcManager GetLocalControllableRpcManager()
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character)
			return null;
		return character.GetRHSRpcManager();
	}
	
	void AskAuthorityToSyncPresetFreq(notnull RHS_RadioComponent pRadioComp, int pPresetNumber, int pNewFrequency)
	{
		RplId id = Replication.FindId(pRadioComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_SyncPresetFreq, id, pPresetNumber, pNewFrequency);
		}
	}

	void AskAuthorityToSyncKnobState(notnull RHS_RadioComponent pRadioComp, int pNewKnobState)
	{
		RplId id = Replication.FindId(pRadioComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_SyncKnobState, id, pNewKnobState);
		}
	}

	void AskAuthorityToAddNewPreset(notnull RHS_RadioComponent pRadioComp, int pPresetNumber, int pNewFrequency)
	{
		RplId id = Replication.FindId(pRadioComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_AddNewPreset, id, pPresetNumber, pNewFrequency);
		}
	}

	void AskAuthorityToPlaySound(notnull RHS_RadioComponent pRadioComp, string pEventName, string pSoundType)
	{
		RplId id = Replication.FindId(pRadioComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_PlayRadioSound, id, pEventName, pSoundType);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_PlayRadioSound(RplId pRplID, string pEventName, string pSoundType)
	{
		if (!pRplID || !pRplID.IsValid())
			return;

		RHS_RadioComponent enhancedRadioDevice = RHS_RadioComponent.Cast(Replication.FindItem(pRplID));
		if (!enhancedRadioDevice)
			return;

		enhancedRadioDevice.AuthorityPlaySound(pEventName, pSoundType);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncPresetFreq(RplId pRplID, int pPresetNumber, int pNewFrequency)
	{
		if (!pRplID || !pRplID.IsValid())
			return;

		RHS_RadioComponent enhancedRadioDevice = RHS_RadioComponent.Cast(Replication.FindItem(pRplID));
		if (!enhancedRadioDevice)
			return;

		enhancedRadioDevice.AuthoritySyncPresetNewFrequency(pPresetNumber, pNewFrequency);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncKnobState(RplId pRplID, int pNewKnobState)
	{
		if (!pRplID || !pRplID.IsValid())
			return;

		RHS_RadioComponent enhancedRadioDevice = RHS_RadioComponent.Cast(Replication.FindItem(pRplID));
		if (!enhancedRadioDevice)
			return;

		enhancedRadioDevice.AuthoritySyncNewKnobState(pNewKnobState);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_AddNewPreset(RplId pRplID, int pPresetNumber, int pNewFrequency)
	{
		if (!pRplID || !pRplID.IsValid())
			return;

		RHS_RadioComponent enhancedRadioDevice = RHS_RadioComponent.Cast(Replication.FindItem(pRplID));
		if (!enhancedRadioDevice)
			return;

		enhancedRadioDevice.AuthorityAddNewPreset(pPresetNumber, pNewFrequency);
	}

	///////////////////////////////////////////////////////////
	////////////////////////// N V G //////////////////////////
	///////////////////////////////////////////////////////////
	RHS_NightVisionComponent GetOwnedNVG(out bool isCurrentNVNotAttachedToMe = false)
	{
		isCurrentNVNotAttachedToMe = m_bIsCurrentNVNotAttachedToMe;
		SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(GetOwner());
		if (!gadgetManager)
			return null;

		array<SCR_GadgetComponent> nvgs = gadgetManager.GetGadgetsByType(EGadgetType.NIGHT_VISION);
		if (!nvgs || nvgs.IsEmpty())
			return null;

		foreach (SCR_GadgetComponent nvg : nvgs)
		{
			if (nvg.GetMode() == EGadgetMode.IN_SLOT)
				return RHS_NightVisionComponent.Cast(nvg);
		}

		return null;
	}

	void SetNotAttachedNVStatus(bool pNewState)
	{
		m_bIsCurrentNVNotAttachedToMe = pNewState;
	}

	void SetOwnedNVG(RHS_NightVisionComponent pNewOwnedNV)
	{
		//tmp fix for crash when loadout is saved
		//m_OwnedNVComponent = pNewOwnedNV;
		if (!pNewOwnedNV && m_bIsCurrentNVNotAttachedToMe)
		{
			m_bIsCurrentNVNotAttachedToMe = false;
			//m_OwnedNVComponent = FindCurrentlyWornNVComponent();
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(SCR_PlayerController.s_pLocalPlayerController);
		if (!pc)
			return;

		pc.RHS_SwitchNearbyIRLights();
	}

	RHS_NightVisionComponent FindCurrentlyWornNVComponent()
	{
		SCR_CharacterInventoryStorageComponent charInventoryStorageComp = SCR_CharacterInventoryStorageComponent.Cast(GetOwner().FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!charInventoryStorageComp)
			return null;

		IEntity helmetEntity = charInventoryStorageComp.GetClothFromArea(LoadoutHeadCoverArea);
		if (!helmetEntity)
			return null;

		RHS_ClothNodeStorageComponent nodeStorageComp = RHS_ClothNodeStorageComponent.Cast(helmetEntity.FindComponent(RHS_ClothNodeStorageComponent));
		if (!nodeStorageComp)
			return null;

		array<IEntity> ownedHelmetItems = {};
		nodeStorageComp.GetAll(ownedHelmetItems);
		if (!ownedHelmetItems || ownedHelmetItems && ownedHelmetItems.Count() == 0)
			return null;

		foreach (IEntity item : ownedHelmetItems)
		{
			RHS_NightVisionComponent nightVisionComp = RHS_NightVisionComponent.Cast(item.FindComponent(RHS_NightVisionComponent));
			if (nightVisionComp)
				return nightVisionComp;
		}

		return null;
	}

	///////////////////////////////////////////////////////////
	/////////////////////// L I G H T S ///////////////////////
	///////////////////////////////////////////////////////////
	void AskAuthorityToSyncLightDeviceState(RHS_LightDevice pLightDeviceComp, bool pNewState)
	{
		RplId id = Replication.FindId(pLightDeviceComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_SyncLightDeviceState, id, pNewState);
		}
	}

	void AskAuthorityToSyncLightType(RHS_LightDevice pLightDeviceComp, int pNewType)
	{
		RplId id = Replication.FindId(pLightDeviceComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_SyncLightType, id, pNewType);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncLightDeviceState(RplId id, bool pNewState)
	{
		RHS_LightDevice lightDevice = RHS_LightDevice.Cast(Replication.FindItem(id));
		if (!lightDevice)
			return;

		lightDevice.AuthoritySyncLightDeviceState(pNewState);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncLightType(RplId id, int pNewType)
	{
		RHS_LightDevice lightDevice = RHS_LightDevice.Cast(Replication.FindItem(id));
		if (!lightDevice)
			return;

		lightDevice.AuthoritySyncLightType(pNewType);
	}
	
	// LBT ADDED
	void AskAuthorityToSyncWLightSpillAngle(RHS_LightDevice pLightDeviceComp, float pNewAngle)
	{
		RplId id = Replication.FindId(pLightDeviceComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_SyncWLightSpillAngle, id, pNewAngle);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncWLightSpillAngle(RplId id, float pNewAngle)
	{
		RHS_LightDevice lightDevice = RHS_LightDevice.Cast(Replication.FindItem(id));
		if (!lightDevice)
			return;
		
		lightDevice.AuthoritySyncWLightSpillAngle(pNewAngle);
	}
	
	void AskAuthorityToSyncWLightSpotAngle(RHS_LightDevice pLightDeviceComp, float pNewAngle)
	{
		RplId id = Replication.FindId(pLightDeviceComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_SyncWLightSpotAngle, id, pNewAngle);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncWLightSpotAngle(RplId id, float pNewAngle)
	{
		RHS_LightDevice lightDevice = RHS_LightDevice.Cast(Replication.FindItem(id));
		if (!lightDevice)
			return;
		
		lightDevice.AuthoritySyncWLightSpotAngle(pNewAngle);
	}
	
	void AskAuthorityToSyncIRLowSpotAngle(RHS_LightDevice pLightDeviceComp, float pNewAngle)
	{
		RplId id = Replication.FindId(pLightDeviceComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_SyncIRLowSpotAngle, id, pNewAngle);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncIRLowSpotAngle(RplId id, float pNewAngle)
	{
		RHS_LightDevice lightDevice = RHS_LightDevice.Cast(Replication.FindItem(id));
		if (!lightDevice)
			return;
		
		lightDevice.AuthoritySyncIRLowSpotAngle(pNewAngle);
	}
	
	void AskAuthorityToSyncIRHighSpotAngle(RHS_LightDevice pLightDeviceComp, float pNewAngle)
	{
		RplId id = Replication.FindId(pLightDeviceComp);
		if (id && id.IsValid())
		{
			Rpc(RpcAsk_SyncIRHighSpotAngle, id, pNewAngle);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncIRHighSpotAngle(RplId id, float pNewAngle)
	{
		RHS_LightDevice lightDevice = RHS_LightDevice.Cast(Replication.FindItem(id));
		if (!lightDevice)
			return;
		
		lightDevice.AuthoritySyncIRHighSpotAngle(pNewAngle);
	}
	// END LBT ADDED
	
	///////////////////////////////////////////////////////////
	/////////////////////// B I P O D S ///////////////////////
	///////////////////////////////////////////////////////////
	void AskAuthorityToSyncBipodState(RHS_BipodComponent pBipod, bool pNewState)
	{
		RplId id = Replication.FindId(pBipod);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncBipodState, id, pNewState);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncBipodState(RplId id, bool pNewState)
	{
		RHS_BipodComponent bipod = RHS_BipodComponent.Cast(Replication.FindItem(id));
		if (!bipod)
			return;

		bipod.AuthoritySyncBipodState(pNewState);
	}

	///////////////////////////////////////////////////////////
	//////////////// S T A B I L I Z A T I O N ////////////////
	///////////////////////////////////////////////////////////
	void AskAuthorityToSyncStabilizationEnabledState(RHS_TurretStabilizationComponent turretStabilizationComponent, bool stabilizationEnabled)
	{
		RplId id = Replication.FindId(turretStabilizationComponent);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncStabilizationEnabledState, id, stabilizationEnabled);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncStabilizationEnabledState(RplId id, bool stabilizationEnabled)
	{
		RHS_TurretStabilizationComponent turretStabilizationComponent = RHS_TurretStabilizationComponent.Cast(Replication.FindItem(id));
		if (!turretStabilizationComponent)
		{
			Print(string.Format("RHS_RpcManager.RpcAsk_SyncStabilizationEnabledState => Wrong RplId (%1).", id), LogLevel.ERROR);
			return;
		}

		turretStabilizationComponent.SetStabilizationEnabled(stabilizationEnabled);
	}

	void AskAuthorityToSyncStabilizationTarget(RHS_TurretStabilizationComponent turretStabilizationComponent, IEntity target)
	{
		RplId id = Replication.FindId(turretStabilizationComponent);
		RplId entityId = Replication.FindId(target);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncStabilizationStabilizationTarget, id, entityId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncStabilizationStabilizationTarget(RplId id, RplId entityId)
	{
		RHS_TurretStabilizationComponent turretStabilizationComponent = RHS_TurretStabilizationComponent.Cast(Replication.FindItem(id));
		if (!turretStabilizationComponent)
		{
			Print(string.Format("RHS_RpcManager.RpcAsk_SyncStabilizationStabilizationTarget => Wrong RplId (%1).", id), LogLevel.ERROR);
			return;
		}

		turretStabilizationComponent.SetStabilizationTarget(entityId);
	}

	///////////////////////////////////////////////////////////
	////////////////////////// P M F //////////////////////////
	///////////////////////////////////////////////////////////
	void AskAuthorityToSyncPMFCameraRotation(RHS_VehiclePMFScreenComponent PMFScreen, float rotation)
	{
		RplId id = Replication.FindId(PMFScreen);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncPMFCameraRotation, id, rotation);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncPMFCameraRotation(RplId id, float rotation)
	{
		RHS_VehiclePMFScreenComponent PMFScreen = RHS_VehiclePMFScreenComponent.Cast(Replication.FindItem(id));
		if (!PMFScreen)
			return;

		PMFScreen.RotateCamera(rotation);
	}

	void AskAuthorityToSyncPMFCurrentSubMenu(RHS_VehiclePMFScreenComponent PMFScreen, int currentSubMenu)
	{
		RplId id = Replication.FindId(PMFScreen);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncPMFCurrentSubMenu, id, currentSubMenu);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncPMFCurrentSubMenu(RplId id, int currentSubMenu)
	{
		RHS_VehiclePMFScreenComponent PMFScreen = RHS_VehiclePMFScreenComponent.Cast(Replication.FindItem(id));
		if (!PMFScreen)
			return;

		PMFScreen.SetCurrentSubMenu(currentSubMenu);
	}

	void AskAuthorityToSyncPMFCurrentCameraZoom(RHS_VehiclePMFScreenComponent PMFScreen, float zoom)
	{
		RplId id = Replication.FindId(PMFScreen);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncPMFCurrentCameraZoom, id, zoom);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncPMFCurrentCameraZoom(RplId id, float zoom)
	{
		RHS_VehiclePMFScreenComponent PMFScreen = RHS_VehiclePMFScreenComponent.Cast(Replication.FindItem(id));
		if (!PMFScreen)
			return;

		PMFScreen.SetCurrentCameraZoom(zoom);
	}

	void AskAuthorityToSyncPMFCameraScanEnabled(RHS_VehiclePMFScreenComponent PMFScreen, bool cameraScanEnabled)
	{
		RplId id = Replication.FindId(PMFScreen);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncPMFCameraScanEnabled, id, cameraScanEnabled);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncPMFCameraScanEnabled(RplId id, bool cameraScanEnabled)
	{
		RHS_VehiclePMFScreenComponent PMFScreen = RHS_VehiclePMFScreenComponent.Cast(Replication.FindItem(id));
		if (!PMFScreen)
			return;

		PMFScreen.SetCameraScanEnabled(cameraScanEnabled);
	}

	void AskAuthorityToSyncPMFCursorPosition(RHS_VehiclePMFScreenComponent PMFScreen, vector cursorPosition)
	{
		RplId id = Replication.FindId(PMFScreen);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncPMFCursorPosition, id, cursorPosition);
	}
	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	void RpcAsk_SyncPMFCursorPosition(RplId id, vector cursorPosition)
	{
		RHS_VehiclePMFScreenComponent PMFScreen = RHS_VehiclePMFScreenComponent.Cast(Replication.FindItem(id));
		if (!PMFScreen)
			return;

		PMFScreen.SetInnerCursorPosition(cursorPosition);
	}

	void AskAuthorityToSyncPMFCurrentDrongOSMenu(RHS_VehiclePMFScreenComponent PMFScreen, int drongOSMenu)
	{
		RplId id = Replication.FindId(PMFScreen);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncPMFCurrentDrongOSMenu, id, drongOSMenu);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncPMFCurrentDrongOSMenu(RplId id, int drongOSMenu)
	{
		RHS_VehiclePMFScreenComponent PMFScreen = RHS_VehiclePMFScreenComponent.Cast(Replication.FindItem(id));
		if (!PMFScreen)
			return;

		PMFScreen.SetDrongOSMenu(drongOSMenu);
	}

	void AskAuthorityToSyncPMFCurrentMainSubMenu(RHS_VehiclePMFScreenComponent PMFScreen, int mainSubMenu)
	{
		RplId id = Replication.FindId(PMFScreen);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncPMFCurrentMainSubMenu, id, mainSubMenu);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncPMFCurrentMainSubMenu(RplId id, int mainSubMenu)
	{
		RHS_VehiclePMFScreenComponent PMFScreen = RHS_VehiclePMFScreenComponent.Cast(Replication.FindItem(id));
		if (!PMFScreen)
			return;

		PMFScreen.SetMainSubMenu(mainSubMenu);
	}

	void AskAuthorityToSyncTurretGuidingTarget(RHS_VehicleScreensControllerComponent screensController, RplId vehicleRplId)
	{
		RplId id = Replication.FindId(screensController);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncTurretGuidingTarget, id, vehicleRplId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncTurretGuidingTarget(RplId id, RplId vehicleRplId)
	{
		RHS_VehicleScreensControllerComponent screensController = RHS_VehicleScreensControllerComponent.Cast(Replication.FindItem(id));
		if (!screensController)
			return;

		screensController.SetGuidingTarget(vehicleRplId);
	}

	void AskAuthorityToSyncVehicleScreenPoweredState(RHS_VehicleScreenComponent screenComponent, bool powered)
	{
		RplId id = Replication.FindId(screenComponent);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncVehicleScreenPoweredState, id, powered);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncVehicleScreenPoweredState(RplId id, bool powered)
	{
		RHS_VehicleScreenComponent screenComponent = RHS_VehicleScreenComponent.Cast(Replication.FindItem(id));
		if (!screenComponent)
			return;

		screenComponent.SetPowered(powered);
	}

	void AskAuthorityToSyncVehicleCameraHDR(RHS_VehiclePiPScreenComponent screenComponent, int HDR)
	{
		RplId id = Replication.FindId(screenComponent);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncVehicleScreenCameraHDR, id, HDR);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncVehicleScreenCameraHDR(RplId id, int HDR)
	{
		RHS_VehiclePiPScreenComponent screenComponent = RHS_VehiclePiPScreenComponent.Cast(Replication.FindItem(id));
		if (!screenComponent)
			return;

		screenComponent.SetCameraHDR(HDR);
	}

	void AskAuthorityToSyncPMFCurrentMapOffset(RHS_VehiclePMFScreenComponent PMFScreen, vector mapOffset)
	{
		RplId id = Replication.FindId(PMFScreen);
		if (id && id.IsValid())
			Rpc(RpcAsk_SyncPMFCurrentMapOffset, id, mapOffset, SCR_PlayerController.GetLocalPlayerId());
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_SyncPMFCurrentMapOffset(RplId id, vector mapOffset, int userPlayerId)
	{
		RHS_VehiclePMFScreenComponent PMFScreen = RHS_VehiclePMFScreenComponent.Cast(Replication.FindItem(id));
		if (!PMFScreen)
			return;

		PMFScreen.SetCurrentMapOffset(mapOffset, userPlayerId);
	}

	///////////////////////////////////////////////////////////
	////////////////// CALL IN SUPPORT SYSTEM /////////////////
	///////////////////////////////////////////////////////////
	void AskForSupportCallIn(notnull ChimeraCharacter caller, SCR_ECallInSupportType callInType, vector desiredPos)
	{
		RplId callerID = Replication.FindId(caller);
		if (!callerID.IsValid())
			return;

		Rpc(RPC_AskForSupportCallIn, callerID, callInType, desiredPos);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_AskForSupportCallIn(RplId callerId, SCR_ECallInSupportType callInType, vector desiredPos)
	{
		if (!callerId.IsValid())
			return;

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		RHS_CallInSupportSystem callInSystem = RHS_CallInSupportSystem.Cast(world.FindSystem(RHS_CallInSupportSystem));
		if (!callInSystem)
			return;

		callInSystem.RequestCallIn(callerId, callInType, desiredPos);
	}

	void UpdateCallInDataForClient(SCR_ECallInSupportType callInType, int useLimit, int cooldownTime, int spawnDelay, bool canAfford)
	{
		Rpc(RPC_DoUpdateCallInDataForClient, callInType, useLimit, cooldownTime, spawnDelay, canAfford);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	void RPC_DoUpdateCallInDataForClient(SCR_ECallInSupportType callInType, int useLimit, int cooldownTime, int spawnDelay, bool canAfford)
	{
		
		SCR_Faction faction = SCR_Faction.Cast(SCR_FactionManager.SGetLocalPlayerFaction());
		if (!faction)
			return;

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		RHS_CallInSupportSystem callInSystem = RHS_CallInSupportSystem.Cast(world.FindSystem(RHS_CallInSupportSystem));
		if (!callInSystem)
			return;

		callInSystem.UpdateLocalCallInData(faction, callInType, useLimit, cooldownTime, spawnDelay, canAfford);
	}

	void AskForSupportCallInCancellation(notnull ChimeraCharacter caller, SCR_ECallInSupportType callInType)
	{
		RplId callerID = Replication.FindId(caller);
		if (!callerID.IsValid())
			return;

		Rpc(RPC_AskForSupportCallInCancellation, callerID, callInType);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_AskForSupportCallInCancellation(RplId callerId, SCR_ECallInSupportType callInType)
	{
		if (!callerId.IsValid())
			return;

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		RHS_CallInSupportSystem callInSystem = RHS_CallInSupportSystem.Cast(world.FindSystem(RHS_CallInSupportSystem));
		if (!callInSystem)
			return;

		callInSystem.RequestCallInCancellation(callerId, callInType);
	}

	///////////////////////////////////////////////////////////
	//////////////////////// GPS SYSTEM ///////////////////////
	///////////////////////////////////////////////////////////
	void RequestGPSMarkerData(RplId GPSContainerId)
	{
		Rpc(RPC_DoRequestGPSMarkerData, GPSContainerId);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_DoRequestGPSMarkerData(RplId GPSContainerId)
	{
		RHS_GPSMarkerContainerComponent GPSMarkerContainerComponent = RHS_GPSMarkerContainerComponent.Cast(Replication.FindItem(GPSContainerId));
		if (!GPSMarkerContainerComponent)
			return;
		GPSMarkerContainerComponent.ResponceMarkersToOwner(this);
	}
	
	void UpdateGPSMarkerData(RplId GPSContainerId, RHS_GPSMarker marker)
	{
		Rpc(RPC_DoUpdateGPSMarkerData, GPSContainerId, marker);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_DoUpdateGPSMarkerData(RplId GPSContainerId, RHS_GPSMarker marker)
	{
		RHS_GPSMarkerContainerComponent GPSMarkerContainerComponent = RHS_GPSMarkerContainerComponent.Cast(Replication.FindItem(GPSContainerId));
		if (!GPSMarkerContainerComponent)
			return;
		GPSMarkerContainerComponent.OnTransferMarker(marker, false);
	}
	
	void DeleteGPSMarkerData(RplId GPSContainerId, RHS_GPSMarker marker)
	{
		Rpc(RPC_DoDeleteGPSMarkerData, GPSContainerId, marker);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_DoDeleteGPSMarkerData(RplId GPSContainerId, RHS_GPSMarker marker)
	{
		RHS_GPSMarkerContainerComponent GPSMarkerContainerComponent = RHS_GPSMarkerContainerComponent.Cast(Replication.FindItem(GPSContainerId));
		if (!GPSMarkerContainerComponent)
			return;
		GPSMarkerContainerComponent.OnDeleteMarker(marker, false);
	}
	
	void CreateGPSMarkerOnServer(RplId GPSContainerId, RHS_GPSMarker marker, bool transfer)
	{
		Rpc(RPC_DoCreateGPSMarkerOnServer, GPSContainerId, marker, transfer);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_DoCreateGPSMarkerOnServer(RplId GPSContainerId, RHS_GPSMarker marker, bool transfer)
	{
		marker.UpdateId();
		RHS_GPSMarkerContainerComponent GPSMarkerContainerComponent = RHS_GPSMarkerContainerComponent.Cast(Replication.FindItem(GPSContainerId));
		if (!GPSMarkerContainerComponent)
			return;
		if (transfer)
			GPSMarkerContainerComponent.TransferMarker(marker);
		else
			GPSMarkerContainerComponent.OnTransferMarker(marker, true);
	}
	
	void UpdateGPSMarkerOnServer(RplId GPSContainerId, RHS_GPSMarker marker, bool transfer)
	{
		Rpc(RPC_DoUpdateGPSMarkerOnServer, GPSContainerId, marker, transfer);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_DoUpdateGPSMarkerOnServer(RplId GPSContainerId, RHS_GPSMarker marker, bool transfer)
	{
		RHS_GPSMarkerContainerComponent GPSMarkerContainerComponent = RHS_GPSMarkerContainerComponent.Cast(Replication.FindItem(GPSContainerId));
		if (!GPSMarkerContainerComponent)
			return;
		if (transfer)
			GPSMarkerContainerComponent.TransferMarker(marker);
		else
			GPSMarkerContainerComponent.OnTransferMarker(marker, true);
	}
	
	void DeleteGPSMarkerOnServer(RplId GPSContainerId, RHS_GPSMarker marker)
	{
		Rpc(RPC_DoDeleteGPSMarkerOnServer, GPSContainerId, marker);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_DoDeleteGPSMarkerOnServer(RplId GPSContainerId, RHS_GPSMarker marker)
	{
		RHS_GPSMarkerContainerComponent GPSMarkerContainerComponent = RHS_GPSMarkerContainerComponent.Cast(Replication.FindItem(GPSContainerId));
		if (!GPSMarkerContainerComponent)
			return;
		GPSMarkerContainerComponent.OnDeleteMarker(marker, true);
	}
	
	void SetTransmittingChannelOnServer(RplId GPSContainerId, string encryptionKey, int frequency)
	{
		Rpc(RPC_SetTransmittingChannelOnServer, GPSContainerId, encryptionKey, frequency);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_SetTransmittingChannelOnServer(RplId GPSContainerId, string encryptionKey, int frequency)
	{
		RHS_GPSMarkerContainerComponent GPSMarkerContainerComponent = RHS_GPSMarkerContainerComponent.Cast(Replication.FindItem(GPSContainerId));
		if (!GPSMarkerContainerComponent)
			return;
		GPSMarkerContainerComponent.SetTransmittingChannel(encryptionKey, frequency);
	}
	
	///////////////////////////////////////////////////////////
	/////////////////// FINGER PROCEDURAL /////////////////////
	///////////////////////////////////////////////////////////
	void UpdateProceduralFingerStateFinger(RplId CharacterRplId, float state, vector position, vector rotate)
	{
		Rpc(RPC_ServerUpdateProceduralFingerStateFinger, CharacterRplId, state, position, rotate);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_ServerUpdateProceduralFingerStateFinger(RplId CharacterRplId, float state, vector position, vector rotate)
	{
		RPC_DoUpdateProceduralFingerStateFinger(CharacterRplId, state, position, rotate);
		Rpc(RPC_DoUpdateProceduralFingerStateFinger, CharacterRplId, state, position, rotate);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_DoUpdateProceduralFingerStateFinger(RplId CharacterRplId, float state, vector position, vector rotate)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(Replication.FindItem(CharacterRplId));
		if (!character)
			return;
		CharacterAnimationComponent characterAnimationComponent = character.GetAnimationComponent();
		TAnimGraphVariable fingerTarget = characterAnimationComponent.BindVariableFloat("RHS_FingerTarget");
		characterAnimationComponent.SetVariableFloat(fingerTarget, state);
		if (state > 0)
			characterAnimationComponent.SetIKTarget("RHS_FingerTarget", "", position, rotate);
	}
	
	///////////////////////////////////////////////////////////
	///////////////////// GADGET STATE ////////////////////////
	///////////////////////////////////////////////////////////
	void UpdateGadgetStateOnServer(RHS_ItemGadgetTouchScreenComponent itemGadgetTouchScreenComponent)
	{
		if (!itemGadgetTouchScreenComponent)
			return;
		
		RplId id = itemGadgetTouchScreenComponent.GetRplId();
		Rpc(RPC_ServerUpdateGadgetStateOnServer, id, itemGadgetTouchScreenComponent.GetStateContainer());
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_ServerUpdateGadgetStateOnServer(RplId itemGadgetTouchScreenComponentRplId, RHS_SSnapshotContainer container)
	{
		RHS_ItemGadgetTouchScreenComponent itemGadgetTouchScreenComponent = RHS_ItemGadgetTouchScreenComponent.Cast(Replication.FindItem(itemGadgetTouchScreenComponentRplId));
		if (!itemGadgetTouchScreenComponent)
			return;
		itemGadgetTouchScreenComponent.SetStateContainer(container);
	}
	
	// request current gadget state from server
	void GetGadgetStateStateFromServer(RHS_ItemGadgetTouchScreenComponent itemGadgetTouchScreenComponent)
	{
		if (!itemGadgetTouchScreenComponent)
			return;
		
		RplId id = itemGadgetTouchScreenComponent.GetRplId();
		Rpc(RPC_ServerGetGadgetStateStateFromServer, id);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_ServerGetGadgetStateStateFromServer(RplId itemGadgetTouchScreenComponentRplId)
	{
		RHS_ItemGadgetTouchScreenComponent itemGadgetTouchScreenComponent = RHS_ItemGadgetTouchScreenComponent.Cast(Replication.FindItem(itemGadgetTouchScreenComponentRplId));
		if (!itemGadgetTouchScreenComponent)
			return;
		Rpc(RPC_OwnerGetGadgetStateStateFromServer, itemGadgetTouchScreenComponentRplId, itemGadgetTouchScreenComponent.GetStateContainer());
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RPC_OwnerGetGadgetStateStateFromServer(RplId itemGadgetTouchScreenComponentRplId, RHS_SSnapshotContainer container)
	{
		RHS_ItemGadgetTouchScreenComponent itemGadgetTouchScreenComponent = RHS_ItemGadgetTouchScreenComponent.Cast(Replication.FindItem(itemGadgetTouchScreenComponentRplId));
		if (!itemGadgetTouchScreenComponent)
			return;
		itemGadgetTouchScreenComponent.SetStateContainerFromServer(container);
	}

	//------------------------------------------------------------------------------------------------
	void ~RHS_RpcManager()
	{
		m_OwnedNVComponent = null;
	}
}




















