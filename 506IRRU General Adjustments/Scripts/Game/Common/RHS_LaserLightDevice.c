class RHS_LightDeviceClass : ScriptGameComponentClass
{
}

class RHS_LightDevice : ScriptGameComponent
{
	[Attribute("0", UIWidgets.ComboBox, "Available light types from BaseLightManagerComponent (ELightType)", "", ParamEnumArray.FromEnum(ELightType))]
	protected ref array<int> m_iAvailableLightTypes;
	[Attribute("1", UIWidgets.ComboBox, "If item should emit light while on the ground or when attached to the object that is on the ground")]
	protected bool m_bEmitWhileOnTheGround;
	[Attribute("0", UIWidgets.ComboBox, "If item should emit light when its spawned")]
	protected bool m_bIsTurnedOn;
	[Attribute(ELightType.AllLights.ToString(), UIWidgets.ComboBox, "Which light should be used when Is Turned On == true", "", ParamEnumArray.FromEnum(ELightType))]
	protected ELightType m_eSelectedLightType;

//Action Listeners setup
	[Attribute(string.Empty, UIWidgets.Auto, "Name of action context - Should be empty unless this item should activate specific context", category : "RHS-Action Listeners")]
	protected string m_sActionContextName;
//Toggle function
	[Attribute(string.Empty, UIWidgets.Auto, "Name of Toggle Action Listener - Should be empty unless this item should have this functionality by key strokes", category : "RHS-Action Listeners")]
	protected string m_sToggleActionListenerName;
//Mode switch function
	[Attribute(string.Empty, UIWidgets.Auto, "Name of Change mode listener - Should be empty unless this item should have this functionality by key strokes", category : "RHS-Action Listeners")]
	protected string m_sCycleActionListenerName;
//Hold function
	[Attribute(string.Empty, UIWidgets.Auto, "Name of Hold action listener - Should be empty unless this item should have this functionality by key strokes", category : "RHS-Action Listeners")]
	protected string m_sHoldActionListenerName;

	protected ref map<ELightType, ref int> m_mLightEntityTypeMap = new map<ELightType, ref int>();
	protected ref array<RHS_LightEntity> m_aLightEntities = {};
	protected ref array<int> m_iActiveLightId = {};
	protected bool m_bIsIRActive = false;
	protected bool m_bIsSuspended = false;
	protected bool m_bPreSuspensionStatus = false;
	protected BaseLightManagerComponent m_LightManagerComponent;
	protected SoundComponent m_SoundComponent;
	protected BaseWeaponManagerComponent m_WeaponManagerComp;
	protected SCR_CharacterControllerComponent m_CharControllerComp;
	protected RHS_RpcManager m_CharacterRpcManager;
	protected SCR_ChimeraCharacter m_CharacterOwner;
	protected RplComponent m_RplComp;
	// LBT ADDED
	protected float REFLECTOR_EFFICENCY = 1.055;
	protected float angleStep;
	float setAngle;
	// END LBT ADDED
	
	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	bool IsTurnedOn()
	{
		return m_bIsTurnedOn;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	bool IsIRActive()
	{
		return m_bIsIRActive;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	bool IsSuspended()
	{
		return m_bIsSuspended;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	ELightType GetSelectedLightType()
	{
		return m_eSelectedLightType;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	bool AreIRLightsActive()
	{
		RHS_LightEntity light;
		foreach (int lightType : m_iAvailableLightTypes)
		{
			light = m_aLightEntities[m_mLightEntityTypeMap.Get(lightType)];
			if (light.IsIR() && m_LightManagerComponent.GetLightsState(lightType))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//!
	void ToggleDeviceState()
	{
		SwitchDeviceState(!m_bIsTurnedOn, true);
		if (m_CharacterRpcManager)
			m_CharacterRpcManager.AskAuthorityToSyncLightDeviceState(this, m_bIsTurnedOn);
	}

	//------------------------------------------------------------------------------------------------
	//!
	void CycleLightTypes()
	{
		int numberOfTypes = m_iAvailableLightTypes.Count();
		if (numberOfTypes > 1)
		{
			int currentTypeID = m_iAvailableLightTypes.Find(m_eSelectedLightType);
			if (currentTypeID == numberOfTypes - 1 || currentTypeID == -1)
				m_eSelectedLightType = m_iAvailableLightTypes[0];
			else
				m_eSelectedLightType = m_iAvailableLightTypes[currentTypeID + 1];

			SetLightType(m_eSelectedLightType);
		}

		if (m_CharacterRpcManager)
			m_CharacterRpcManager.AskAuthorityToSyncLightType(this, m_eSelectedLightType);
	}

	//------------------------------------------------------------------------------------------------
	//!
	void HoldFunctionalityPressed()
	{
		if (!m_bIsTurnedOn)
		{
			SwitchDeviceState(true, true);
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncLightDeviceState(this, m_bIsTurnedOn);
		}
	}

	//------------------------------------------------------------------------------------------------
	//!
	void HoldFunctionalityReleased()
	{
		if (m_bIsTurnedOn)
		{
			SwitchDeviceState(false, true);
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncLightDeviceState(this, m_bIsTurnedOn);
		}
	}
	
	// LBT ADDED
	//---------------------------------------------//
	//--- LBT - Call-backs for action listeners ---//
	//---------------------------------------------//
	
	void ConeAngleIncrease()
	{
		RHS_LightEntity whiteLightSpill;
		RHS_LightEntity whiteLightSpot;
		RHS_LightEntity irIlluminatorLow;
		RHS_LightEntity irIlluminatorHigh;
		whiteLightSpill = m_aLightEntities[m_mLightEntityTypeMap.Get(10)];
		whiteLightSpot = m_aLightEntities[m_mLightEntityTypeMap.Get(3)];
		irIlluminatorLow = m_aLightEntities[m_mLightEntityTypeMap.Get(9)];
		irIlluminatorHigh = m_aLightEntities[m_mLightEntityTypeMap.Get(8)];
		
		RHS_LightEntity other;
		other = m_aLightEntities[m_mLightEntityTypeMap.Get(4)];
		
		if (other.GetConeAngle() > 60 && other.GetConeAngle() < 80) // LBT - This is to exclude NVG mounted illuminators
			return;
		
		if (whiteLightSpill)
		{
			REFLECTOR_EFFICENCY = 1.010;
			angleStep = 15;
			
			if (whiteLightSpill.GetConeAngle() + angleStep > whiteLightSpill.m_fMaxConeAngle + 0.1)
				return;
			
			setAngle = whiteLightSpill.GetConeAngle() + angleStep;
			whiteLightSpill.SetCurrentConeAngle(setAngle, REFLECTOR_EFFICENCY);
			
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncWLightSpillAngle(this, setAngle);
		}
		
		if (whiteLightSpot)
		{
			REFLECTOR_EFFICENCY = 1.055;
			angleStep = 5;
			
			if (whiteLightSpot.GetConeAngle() + angleStep > whiteLightSpot.m_fMaxConeAngle + 0.1)
				return;
			
			setAngle = whiteLightSpot.GetConeAngle() + angleStep;
			whiteLightSpot.SetCurrentConeAngle(setAngle, REFLECTOR_EFFICENCY);
			
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncWLightSpotAngle(this, setAngle);
		}
		
		if (irIlluminatorLow)
		{
			REFLECTOR_EFFICENCY = 1.055;
			angleStep = 5;
			
			if (irIlluminatorLow.GetConeAngle() + angleStep > irIlluminatorLow.m_fMaxConeAngle + 0.1)
				return;
			
			setAngle = irIlluminatorLow.GetConeAngle() + angleStep;
			irIlluminatorLow.SetCurrentConeAngle(setAngle, REFLECTOR_EFFICENCY);
			
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncIRLowSpotAngle(this, setAngle);
		}
		
		if (irIlluminatorHigh)
		{
			REFLECTOR_EFFICENCY = 1.055;
			angleStep = 5;
			
			if (irIlluminatorHigh.GetConeAngle() + angleStep > irIlluminatorHigh.m_fMaxConeAngle + 0.1)
				return;
			
			setAngle = irIlluminatorHigh.GetConeAngle() + angleStep;
			irIlluminatorHigh.SetCurrentConeAngle(setAngle, REFLECTOR_EFFICENCY);
			
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncIRHighSpotAngle(this, setAngle);
		}
	}
	
	void ConeAngleDecrease()
	{
		RHS_LightEntity whiteLightSpill;
		RHS_LightEntity whiteLightSpot;
		RHS_LightEntity irIlluminatorLow;
		RHS_LightEntity irIlluminatorHigh;
		whiteLightSpill = m_aLightEntities[m_mLightEntityTypeMap.Get(10)];
		whiteLightSpot = m_aLightEntities[m_mLightEntityTypeMap.Get(3)];
		irIlluminatorLow = m_aLightEntities[m_mLightEntityTypeMap.Get(9)];
		irIlluminatorHigh = m_aLightEntities[m_mLightEntityTypeMap.Get(8)];
		
		RHS_LightEntity other;
		other = m_aLightEntities[m_mLightEntityTypeMap.Get(4)];
		
		if (other.GetConeAngle() > 60 && other.GetConeAngle() < 80) // LBT - This is to exclude NVG mounted illuminators
			return;
		
		if (whiteLightSpill)
		{
			REFLECTOR_EFFICENCY = 1.010;
			angleStep = 15;
			
			if (whiteLightSpill.GetConeAngle() - angleStep < whiteLightSpill.m_fMinConeAngle - 0.1)
				return;
			
			setAngle = whiteLightSpill.GetConeAngle() - angleStep;
			whiteLightSpill.SetCurrentConeAngle(setAngle, REFLECTOR_EFFICENCY);
			
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncWLightSpillAngle(this, setAngle);
		}
		
		if (whiteLightSpot)
		{
			REFLECTOR_EFFICENCY = 1.055;
			angleStep = 5;
			
			if (whiteLightSpot.GetConeAngle() - angleStep < whiteLightSpot.m_fMinConeAngle - 0.1)
				return;
			
			setAngle = whiteLightSpot.GetConeAngle() - angleStep;
			whiteLightSpot.SetCurrentConeAngle(setAngle, REFLECTOR_EFFICENCY);
			
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncWLightSpotAngle(this, setAngle);
		}
		
		if (irIlluminatorLow)
		{
			REFLECTOR_EFFICENCY = 1.055;
			angleStep = 5;
			
			if (irIlluminatorLow.GetConeAngle() - angleStep < irIlluminatorLow.m_fMinConeAngle - 0.1)
				return;
			
			setAngle = irIlluminatorLow.GetConeAngle() - angleStep;
			irIlluminatorLow.SetCurrentConeAngle(setAngle, REFLECTOR_EFFICENCY);
			
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncIRLowSpotAngle(this, setAngle);
		}
		
		if (irIlluminatorHigh)
		{
			REFLECTOR_EFFICENCY = 1.055;
			angleStep = 5;
			
			if (irIlluminatorHigh.GetConeAngle() - angleStep < irIlluminatorHigh.m_fMinConeAngle - 0.1)
				return;
			
			setAngle = irIlluminatorHigh.GetConeAngle() - angleStep;
			irIlluminatorHigh.SetCurrentConeAngle(setAngle, REFLECTOR_EFFICENCY);
			
			if (m_CharacterRpcManager)
				m_CharacterRpcManager.AskAuthorityToSyncIRHighSpotAngle(this, setAngle);
		}
	}
	
	//-------------------------//
	//--- LBT - Replication ---//
	//-------------------------//
	
	void AuthoritySyncWLightSpillAngle(float pNewAngle)
	{
		Rpc(RpcDo_SyncWLightSpillAngle, pNewAngle);
		RpcDo_SyncWLightSpillAngle(pNewAngle);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SyncWLightSpillAngle(float pNewAngle)
	{
		RHS_LightEntity whiteLightSpill;
		whiteLightSpill = m_aLightEntities[m_mLightEntityTypeMap.Get(10)];
		
		if (!whiteLightSpill)
			return;
		
		if (whiteLightSpill)
		{
			REFLECTOR_EFFICENCY = 1.010;
			whiteLightSpill.SetCurrentConeAngle(pNewAngle, REFLECTOR_EFFICENCY);
		}
	}
	
	void AuthoritySyncWLightSpotAngle(float pNewAngle)
	{
		Rpc(RpcDo_SyncWLightSpotAngle, pNewAngle);
		RpcDo_SyncWLightSpotAngle(pNewAngle);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SyncWLightSpotAngle(float pNewAngle)
	{
		RHS_LightEntity whiteLightSpot;
		whiteLightSpot = m_aLightEntities[m_mLightEntityTypeMap.Get(3)];
		
		if (!whiteLightSpot)
			return;
		
		if (whiteLightSpot)
		{
			REFLECTOR_EFFICENCY = 1.055;
			whiteLightSpot.SetCurrentConeAngle(pNewAngle, REFLECTOR_EFFICENCY);
		}
	}
	
	void AuthoritySyncIRLowSpotAngle(float pNewAngle)
	{
		Rpc(RpcDo_SyncIRLowSpotAngle, pNewAngle);
		RpcDo_SyncIRLowSpotAngle(pNewAngle);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SyncIRLowSpotAngle(float pNewAngle)
	{
		RHS_LightEntity irIlluminatorLow;
		irIlluminatorLow = m_aLightEntities[m_mLightEntityTypeMap.Get(9)];
		
		if (!irIlluminatorLow)
			return;
		
		if (irIlluminatorLow)
		{
			REFLECTOR_EFFICENCY = 1.055;
			irIlluminatorLow.SetCurrentConeAngle(pNewAngle, REFLECTOR_EFFICENCY);
		}
	}
	
	void AuthoritySyncIRHighSpotAngle(float pNewAngle)
	{
		Rpc(RpcDo_SyncIRHighSpotAngle, pNewAngle);
		RpcDo_SyncIRHighSpotAngle(pNewAngle);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SyncIRHighSpotAngle(float pNewAngle)
	{
		RHS_LightEntity irIlluminatorHigh;
		irIlluminatorHigh = m_aLightEntities[m_mLightEntityTypeMap.Get(8)];
		
		if (!irIlluminatorHigh)
			return;
		
		if (irIlluminatorHigh)
		{
			REFLECTOR_EFFICENCY = 1.055;
			irIlluminatorHigh.SetCurrentConeAngle(pNewAngle, REFLECTOR_EFFICENCY);
		}
	}
	// END LBT ADDED
	
	protected void SetActionListeners()
	{
		if (GetGame().GetInputManager())
		{
			if (m_sActionContextName != string.Empty)
				GetGame().GetInputManager().ActivateContext(m_sActionContextName);

			if (m_sToggleActionListenerName != string.Empty)
				GetGame().GetInputManager().AddActionListener(m_sToggleActionListenerName, EActionTrigger.DOWN, ToggleDeviceState);

			if (m_sCycleActionListenerName != string.Empty)
				GetGame().GetInputManager().AddActionListener(m_sCycleActionListenerName, EActionTrigger.DOWN, CycleLightTypes);

			if (m_sHoldActionListenerName != string.Empty)
			{
				GetGame().GetInputManager().AddActionListener(m_sHoldActionListenerName, EActionTrigger.DOWN, HoldFunctionalityPressed);
				GetGame().GetInputManager().AddActionListener(m_sHoldActionListenerName, EActionTrigger.UP, HoldFunctionalityReleased);
			}
			// LBT ADDED
			GetGame().GetInputManager().AddActionListener("LBT_RHS_LightDeviceCAInc", EActionTrigger.DOWN, ConeAngleIncrease);
			GetGame().GetInputManager().AddActionListener("LBT_RHS_LightDeviceCADec", EActionTrigger.DOWN, ConeAngleDecrease);
			// END LBT ADDED
		}
	}

	protected void RemoveActionListeners()
	{
		if (GetGame().GetInputManager())
		{
			if (m_sToggleActionListenerName != string.Empty)
				GetGame().GetInputManager().RemoveActionListener(m_sToggleActionListenerName, EActionTrigger.DOWN, ToggleDeviceState);

			if (m_sCycleActionListenerName != string.Empty)
				GetGame().GetInputManager().RemoveActionListener(m_sCycleActionListenerName, EActionTrigger.DOWN, CycleLightTypes);

			if (m_sHoldActionListenerName != string.Empty)
			{
				GetGame().GetInputManager().RemoveActionListener(m_sHoldActionListenerName, EActionTrigger.DOWN, HoldFunctionalityPressed);
				GetGame().GetInputManager().RemoveActionListener(m_sHoldActionListenerName, EActionTrigger.UP, HoldFunctionalityReleased);
			}
			// LBT ADDED
			GetGame().GetInputManager().RemoveActionListener("LBT_RHS_LightDeviceCAInc", EActionTrigger.DOWN, ConeAngleIncrease);
			GetGame().GetInputManager().RemoveActionListener("LBT_RHS_LightDeviceCADec", EActionTrigger.DOWN, ConeAngleDecrease);
			// END LBT ADDED
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pDesiredType
	void SetLightType(ELightType pDesiredType)
	{
		if (m_iAvailableLightTypes.Contains(pDesiredType) || pDesiredType == ELightType.AllLights)
			m_eSelectedLightType = pDesiredType;
		else
			return;

		TurnOffAllLights(false);
		SwitchLightStateByType(m_eSelectedLightType, m_bIsTurnedOn, false);
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	bool InitializeLightManager()
	{
		if (m_LightManagerComponent)
			return true;

		m_LightManagerComponent = BaseLightManagerComponent.Cast(this.GetOwner().FindComponent(BaseLightManagerComponent));

		if (!m_LightManagerComponent)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pNewState
	//! \param[in] pPlaySound
	void SwitchDeviceState(bool pNewState, bool pPlaySound = true)
	{
		TurnOffAllLights(false);
		m_bIsTurnedOn = pNewState;
		if (m_eSelectedLightType == ELightType.AllLights)
		{
			foreach (ELightType light : m_iAvailableLightTypes)
			{
				SwitchLightStateByType(light, pNewState, false);
			}
		}
		else
		{
			SwitchLightStateByType(m_eSelectedLightType, pNewState, false);
		}
		if (pPlaySound)
			PlaySound("SOUND_BUTTON_CLICK");
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pLightType
	//! \return
	bool IsSpecificLightOn(ELightType pLightType)
	{
		if (m_LightManagerComponent)
			return m_LightManagerComponent.GetLightsState(pLightType);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pSoundName
	void PlaySound(string pSoundName)
	{
		if (!m_SoundComponent)
		{
			m_SoundComponent = SoundComponent.Cast(this.GetOwner().FindComponent(SoundComponent));
			if (!m_SoundComponent)
				return;
		}

		if (m_SoundComponent)
			m_SoundComponent.SoundEvent(pSoundName);
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] lightArray
	//! \param[in] playSound
	void SwitchOnLightArray(array<int> lightArray, bool playSound = true)
	{
		if (!InitializeLightManager())
			return;

		TurnOffAllLights(false);

		RHS_LightEntity light;
		foreach (int lightId : lightArray)
		{
			light = m_aLightEntities[m_mLightEntityTypeMap.Get(m_iAvailableLightTypes[lightId])];
			if (!light)
				continue;

			if (light.IsIR())
				m_bIsIRActive = true;
			else if (Replication.FindId(this).IsValid() && !m_RplComp.Role())
				m_LightManagerComponent.SetLightsState(m_iAvailableLightTypes[lightId], true);
		}
		m_iActiveLightId.Copy(lightArray);

		if (m_bIsIRActive)
			ChangeIRVisibility(true);

		if (playSound)
			PlaySound("SOUND_BUTTON_CLICK");
	}

	protected void TurnOffAllLights(bool playSound = true)
	{

		if (!InitializeLightManager())
			return;

		if (!m_LightManagerComponent.GetLightsEnabled() && !m_iActiveLightId.Count())
			return;

		RHS_LightEntity light;
		foreach (int lightType : m_iAvailableLightTypes)
		{
			if (Replication.FindId(this).IsValid() && !m_RplComp.Role())
				m_LightManagerComponent.SetLightsState(lightType, false);

			light = m_aLightEntities[m_mLightEntityTypeMap.Get(lightType)];
			if (!light)
				continue;
			
			int index = m_mLightEntityTypeMap.Get(lightType);
			if (m_aLightEntities.IsIndexValid(index) && m_aLightEntities[index])
				light.SetEnabledWithIRCheck(false);
		}

		m_iActiveLightId.Clear();
		m_bIsIRActive = false;

		if (playSound)
			PlaySound("SOUND_BUTTON_CLICK");
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] lightId
	//! \param[in] playSound
	void SwitchOnLight(int lightId, bool playSound = true)
	{
		int lightIndex = m_iActiveLightId.Find(lightId);

		if (lightIndex != -1)
			return;

		if (!InitializeLightManager())
			return;

		RHS_LightEntity light = m_aLightEntities[m_mLightEntityTypeMap.Get(m_iAvailableLightTypes[lightId])];
		if (!light)
			return;

		if (light.IsIR())
		{
			m_bIsIRActive = true;
			ChangeIRVisibility(true);
		}
		else
			m_LightManagerComponent.SetLightsState(m_iAvailableLightTypes[lightId], true);

		m_iActiveLightId.Insert(lightId);
		if (playSound)
			PlaySound("SOUND_BUTTON_CLICK");
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pLightType
	//! \param[in] pNewState
	//! \param[in] pPlaySound
	void SwitchLightStateByType(ELightType pLightType, bool pNewState, bool pPlaySound = true)
	{

		if (!InitializeLightManager())
			return;

		int arrayIndex = m_iActiveLightId.Find(m_iAvailableLightTypes.Find(pLightType));
		if (pNewState && arrayIndex == -1)
		{
			m_iActiveLightId.Insert(m_iAvailableLightTypes.Find(pLightType));
		}
		else if (!pNewState && arrayIndex != -1)
		{
			m_iActiveLightId.Remove(arrayIndex);
		}

		
		RHS_LightEntity light = m_aLightEntities[m_mLightEntityTypeMap.Get(pLightType)];
		if (light && light.IsIR())
		{
			if (pNewState)
			{
				m_bIsIRActive = pNewState;
				ChangeIRVisibility(true);
			}
			else
			{
				light.SetEnabledWithIRCheck(pNewState);
				m_bIsIRActive = AreIRLightsActive();
			}
		}
		else if (Replication.FindId(this).IsValid() && !m_RplComp.Role())
			m_LightManagerComponent.SetLightsState(pLightType, pNewState);

		if (pPlaySound)
			PlaySound("SOUND_BUTTON_CLICK");
	}

	protected void TurnOffLight(int lightId, bool playSound = true)
	{
		int lightIndex = m_iActiveLightId.Find(lightId);

		if (lightIndex == -1)
			return;

		m_iActiveLightId.Remove(lightIndex);

		if (!InitializeLightManager())
			return;

		if (!m_LightManagerComponent.GetLightsEnabled())
			return;

		if (Replication.FindId(this).IsValid() && !m_RplComp.Role())
			m_LightManagerComponent.SetLightsState(m_iAvailableLightTypes[lightId], false);

		m_aLightEntities[m_mLightEntityTypeMap.Get(m_iAvailableLightTypes[lightId])].SetEnabledWithIRCheck(false);

		if (!m_iActiveLightId.Count())
			m_bIsIRActive = false;
		else
			m_bIsIRActive = AreIRLightsActive();

		if (playSound)
			PlaySound("SOUND_BUTTON_CLICK");
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pNewState
	void ChangeIRVisibility(bool pNewState)
	{
		RHS_LightEntity light;
		foreach (int lightId : m_iActiveLightId)
		{
			light = m_aLightEntities[m_mLightEntityTypeMap.Get(m_iAvailableLightTypes[lightId])];
			if (!light)
				continue;

			light.SetEnabledWithIRCheck(pNewState);
		}
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] lightId
	//! \param[in] pNewState
	void ChangeLightVisibility(int lightId, bool pNewState)
	{
		int lightEntityIndex = m_mLightEntityTypeMap.Get(m_iAvailableLightTypes[lightId]);
		if (m_aLightEntities.IsIndexValid(lightEntityIndex) && m_aLightEntities[lightEntityIndex])
			m_aLightEntities[lightEntityIndex].SetEnabledWithIRCheck(pNewState);
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] lightIdArray
	//! \param[in] pNewState
	void ChangeLightVisibility(array<int> lightIdArray, bool pNewState)
	{
		foreach (int id : lightIdArray)
		{
			ChangeLightVisibility(id, pNewState);
		}
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pLightType
	//! \param[in] pNewState
	void ChangeLightVisibilityByType(ELightType pLightType, bool pNewState)
	{
		int lightEntityIndex = m_mLightEntityTypeMap.Get(pLightType);
		if (m_aLightEntities.IsIndexValid(lightEntityIndex) && m_aLightEntities[lightEntityIndex])
			m_aLightEntities[m_mLightEntityTypeMap.Get(pLightType)].SetEnabledWithIRCheck(pNewState);
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pOldSlot
	//! \param[in] pNewSlot
	void OnSlotChanged(InventoryStorageSlot pOldSlot, InventoryStorageSlot pNewSlot)
	{
		BaseInventoryStorageComponent oldStorage;
		BaseInventoryStorageComponent newStorage;
		if (pOldSlot)
		{
			oldStorage = pOldSlot.GetStorage();
			if (m_CharacterOwner && m_CharacterOwner == SCR_PlayerController.GetLocalControlledEntity())
			{
				RemoveActionListeners();
			}
		}

		if (pNewSlot)
			newStorage = pNewSlot.GetStorage();

		if (!newStorage && pNewSlot)
			newStorage = BaseInventoryStorageComponent.Cast(pNewSlot.GetOwner().FindComponent(BaseInventoryStorageComponent));//becasue pNewSlot.GetStorage(); will not work on init

		if (LoadoutSlotInfo.Cast(pOldSlot) || SCR_WeaponAttachmentsStorageComponent.Cast(oldStorage))
		{
			oldStorage.m_OnParentSlotChangedInvoker.Remove(OnParentSlotChanged);
			m_CharacterOwner = null;
			if (m_CharControllerComp)
			{
				m_CharControllerComp.m_OnPlayerDeath.Remove(OnPlayerDeath);
				m_CharControllerComp.m_OnControlledByPlayer.Remove(OnControlledByPlayer);
				m_CharControllerComp = null;
			}
			if (m_WeaponManagerComp)
			{
				m_WeaponManagerComp.m_OnWeaponChangeCompleteInvoker.Remove(OnWeaponChangeComplete);
				m_WeaponManagerComp = null;
			}
		}

		if (LoadoutSlotInfo.Cast(pNewSlot) || SCR_WeaponAttachmentsStorageComponent.Cast(newStorage))
		{
			newStorage.m_OnParentSlotChangedInvoker.Insert(OnParentSlotChanged);
			OnParentSlotChanged(null, newStorage.GetParentSlot());//to check if its not attached to the object that is on the ground
		}
		else if (!m_bEmitWhileOnTheGround)
		{
			SetSuspensionStatus(true);
		}
		else if (m_bIsSuspended && (!pNewSlot && m_bEmitWhileOnTheGround || LoadoutSlotInfo.Cast(pNewSlot) || EquipedWeaponStorageComponent.Cast(newStorage)))
		{
			SetSuspensionStatus(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pOldSlot
	//! \param[in] pNewSlot
	void OnParentSlotChanged(InventoryStorageSlot pOldSlot, InventoryStorageSlot pNewSlot)
	{
		if (pOldSlot && m_CharacterOwner)
		{
			if (m_CharacterOwner == SCR_PlayerController.GetLocalControlledEntity())
				RemoveActionListeners();

			if (m_sActionContextName != string.Empty)
				ClearEventMask(this.GetOwner(), EntityEvent.FRAME);

			if (m_CharControllerComp)
			{
				m_CharControllerComp.m_OnPlayerDeath.Remove(OnPlayerDeath);
				m_CharControllerComp.m_OnControlledByPlayer.Remove(OnControlledByPlayer);
				m_CharControllerComp = null;

				if (m_WeaponManagerComp)
				{
					m_WeaponManagerComp.m_OnWeaponChangeCompleteInvoker.Remove(OnWeaponChangeComplete);
					m_WeaponManagerComp = null;
				}
			}
		}

		m_CharacterOwner = null;

		BaseInventoryStorageComponent newStorage;
		if (pNewSlot)
			newStorage = pNewSlot.GetStorage();

		if (!pNewSlot && !m_bEmitWhileOnTheGround || !LoadoutSlotInfo.Cast(pNewSlot) && !EquipedWeaponStorageComponent.Cast(newStorage))
		{
			SetSuspensionStatus(true);
		}
		else if (LoadoutSlotInfo.Cast(pNewSlot) || EquipedWeaponStorageComponent.Cast(newStorage) || !pNewSlot && m_bEmitWhileOnTheGround)
		{
			SetSuspensionStatus(false);
			if (newStorage)
				m_CharacterOwner = SCR_ChimeraCharacter.Cast(newStorage.GetOwner());
		}

		if (!m_CharacterOwner)
			return;

		if (SCR_PlayerController.GetLocalControlledEntity() == m_CharacterOwner && m_sActionContextName != string.Empty)
			SetEventMask(this.GetOwner(), EntityEvent.FRAME);

		m_CharacterRpcManager = m_CharacterOwner.GetRHSRpcManager();

		if (m_CharacterOwner)
			m_CharControllerComp = SCR_CharacterControllerComponent.Cast(m_CharacterOwner.GetCharacterController());

		if (m_CharControllerComp)
		{
			m_CharControllerComp.m_OnPlayerDeath.Insert(OnPlayerDeath);
			m_CharControllerComp.m_OnControlledByPlayer.Insert(OnControlledByPlayer);
			if (EquipedWeaponStorageComponent.Cast(newStorage))
				m_WeaponManagerComp = m_CharControllerComp.GetWeaponManagerComponent();

			if (m_CharacterOwner == SCR_PlayerController.GetLocalControlledEntity())
			{
				if (m_WeaponManagerComp)
				{
					m_WeaponManagerComp.m_OnWeaponChangeCompleteInvoker.Insert(OnWeaponChangeComplete);
					OnWeaponChangeComplete(m_WeaponManagerComp.GetCurrentWeapon());
				}
				else
					SetActionListeners();
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pShouldSuspend
	void SetSuspensionStatus(bool pShouldSuspend)
	{
		if (m_bIsSuspended == pShouldSuspend)
			return;

		m_bIsSuspended = pShouldSuspend;
		if (m_bIsSuspended)
		{
			m_bPreSuspensionStatus = m_bIsTurnedOn;

			if (m_sActionContextName != string.Empty)
				ClearEventMask(this.GetOwner(), EntityEvent.FRAME);

			if (m_bIsTurnedOn)
				SwitchDeviceState(!m_bIsTurnedOn, false);
		}
		else
		{
			if (m_CharacterOwner && SCR_PlayerController.GetLocalControlledEntity() == m_CharacterOwner && m_sActionContextName != string.Empty)
				SetEventMask(this.GetOwner(), EntityEvent.FRAME);

			if (m_bPreSuspensionStatus)
			{
				SwitchDeviceState(m_bPreSuspensionStatus, false);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//!
	void OnPlayerDeath()
	{
		m_CharControllerComp.m_OnPlayerDeath.Remove(OnPlayerDeath);
		if (m_CharacterOwner && m_CharacterOwner == SCR_PlayerController.GetLocalControlledEntity())
			RemoveActionListeners();

		if (m_sActionContextName != string.Empty)
			ClearEventMask(this.GetOwner(), EntityEvent.FRAME);

		m_CharControllerComp = null;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pOwner
	//! \param[in] pNewState
	void OnControlledByPlayer(IEntity pOwner, bool pNewState)
	{
		if (!m_CharacterOwner || m_CharacterOwner != SCR_PlayerController.GetLocalControlledEntity())
			return;

		if (pNewState)	//Player is taking controll of this entity
		{
			if (m_WeaponManagerComp)
				OnWeaponChangeComplete(m_WeaponManagerComp.GetCurrentWeapon());
			else
				SetActionListeners();

		}
		else			//Player is no longer in controll of this entity
		{
			RemoveActionListeners();
		}
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pNowUsedWeapon
	void OnWeaponChangeComplete(BaseWeaponComponent pNowUsedWeapon)
	{
		RemoveActionListeners();
		InventoryItemComponent invItemComp = InventoryItemComponent.Cast(this.GetOwner().FindComponent(InventoryItemComponent));
		if (!invItemComp)
		{
			if (m_WeaponManagerComp)
				m_WeaponManagerComp.m_OnWeaponChangeCompleteInvoker.Remove(OnWeaponChangeComplete);

			return;
		}

		if (!pNowUsedWeapon)
			return;

		if (!invItemComp.GetParentSlot())
			return;
	
		IEntity entityOfParent = GetOwner().GetParent();
		IEntity entityOfCurrentlyUsedWeapon;
		if (CharacterWeaponSlotComponent.Cast(pNowUsedWeapon))
			entityOfCurrentlyUsedWeapon = CharacterWeaponSlotComponent.Cast(pNowUsedWeapon).GetWeaponEntity();
		else //if (pNowUsedWeapon.Type() == WeaponComponent)
			entityOfCurrentlyUsedWeapon = pNowUsedWeapon.GetOwner();

		if (entityOfParent && entityOfParent == entityOfCurrentlyUsedWeapon)
			SetActionListeners();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_CharacterOwner && m_CharacterOwner == SCR_PlayerController.GetLocalControlledEntity() && m_sActionContextName != string.Empty)
			GetGame().GetInputManager().ActivateContext(m_sActionContextName);
	}

	override void EOnInit(IEntity owner)
	{
		InitializeLightManager();
		if (!m_LightManagerComponent)
			return;

		m_RplComp = RplComponent.Cast(owner.FindComponent(RplComponent));

		InventoryItemComponent ownerInventoryItemComp = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
		if (ownerInventoryItemComp)
		{
			ownerInventoryItemComp.m_OnParentSlotChangedInvoker.Insert(OnSlotChanged);
			if (ownerInventoryItemComp.GetParentSlot())
				OnSlotChanged(null, ownerInventoryItemComp.GetParentSlot());
		}

		array<BaseLightSlot> tmpLightSlotArray = {};
		m_LightManagerComponent.GetLights(tmpLightSlotArray);
		foreach (BaseLightSlot slot : tmpLightSlotArray)
		{
			if (!slot.GetLightEntity())
			{
#ifdef WORKBENCH
				UIInfo uii = ownerInventoryItemComp.GetUIInfo();
				Print("* RHS_LightDevice => This prefab (name = "+uii.GetName()+") is missing LightEntity in its BaseLightManagerComponent => slot number = " + (m_aLightEntities.Count()+ 1) + " *", LogLevel.ERROR);
#endif
				return;
			}
			m_aLightEntities.Insert(RHS_LightEntity.Cast(slot.GetLightEntity()));
			m_mLightEntityTypeMap.Set(slot.GetLightType(), m_aLightEntities.Count() - 1);
		}

		if (!System.IsConsoleApp())
			OnWorldStart();
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	bool OnWorldStart()
	{
		if (!GetGame().GetWorld())
		{
			GetGame().GetCallqueue().CallLater(OnWorldStart, 100);
			return false;
		}

		OnPlayerControllerReady();

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \return
	bool OnPlayerControllerReady()
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
		{
			GetGame().GetCallqueue().CallLater(OnPlayerControllerReady, 100);
			return false;
		}

		pc.RHS_AddNearbyIRDevice(this);
		SetLightType(m_eSelectedLightType);
		if (m_bIsTurnedOn)
			SwitchDeviceState(m_bIsTurnedOn, false);

		if (m_bIsTurnedOn)
			SwitchDeviceState(m_bIsTurnedOn, false);

		return true;
	}

	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	override bool OnTicksOnRemoteProxy()
	{
		return true;
	}

	override void OnDelete(IEntity owner)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return;

		pc.RHS_RemoveIRDevice(this);
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pNewState
	void AuthoritySyncLightDeviceState(bool pNewState)
	{
		Rpc(RpcDo_SyncLightDeviceState, pNewState);
		RpcDo_SyncLightDeviceState(pNewState);
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pNewType
	void AuthoritySyncLightType(int pNewType)
	{
		Rpc(RpcDo_SyncLightType, pNewType);
		RpcDo_SyncLightType(pNewType);
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pNewState
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SyncLightDeviceState(bool pNewState)
	{
		if (m_bIsTurnedOn != pNewState)
			SwitchDeviceState(pNewState);
	}

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] pNewType
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SyncLightType(int pNewType)
	{
		SetLightType(pNewType);
	}

	//------------------------------------------------------------------------------------------------
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteBool(m_bIsSuspended);
		writer.WriteBool(m_bPreSuspensionStatus);
		writer.WriteBool(m_bIsTurnedOn);
		writer.WriteInt(m_eSelectedLightType);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadBool(m_bIsSuspended);
		reader.ReadBool(m_bPreSuspensionStatus);
		reader.ReadBool(m_bIsTurnedOn);
		SwitchDeviceState(m_bIsTurnedOn);
		reader.ReadInt(m_eSelectedLightType);
		SetLightType(m_eSelectedLightType);

		return true;
	}
}
