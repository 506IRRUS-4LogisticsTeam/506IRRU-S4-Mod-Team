enum RHS_ANPEQ16_Modes {
	OFF,
	LIGHT,
	AIM,
	DUAL,
	AIMLOW,
	DUALLOW,
	AIMHIGH,
	DUALHIGH
}

class RHS_SGC_ANPEQ16ComponentClass : RHS_LightDeviceClass
{
}

class RHS_SGC_ANPEQ16Component : RHS_LightDevice
{
	[Attribute(RHS_ANPEQ16_Modes.OFF.ToString(), UIWidgets.ComboBox, "Which mode should be set for ANPEQ by default - overrides Selected Light Type value", "", ParamEnumArray.FromEnum(RHS_ANPEQ16_Modes))]
	protected RHS_ANPEQ16_Modes m_eCurrentMode;

	// light arrays
	protected const ref array<int> OLA = {};
	protected const ref array<int> DLA = {0, 1};
	protected const ref array<int> LLA = {0};
	protected const ref array<int> ALA = {1};
	protected const ref array<int> ALLA = {2};
	protected const ref array<int> DLLA = {2, 3};
	protected const ref array<int> AHLA = {4};
	protected const ref array<int> DHLA = {4, 5};
	protected const ref array<int> SPOTLIGHTS = {0, 3, 5};

	protected const int s_iMaxSpotAngle = 35;
	protected const int s_iMinSpotAngle = 15;

	// the anpeq switches off automatically after 5 minutes
	protected int m_iSwitchOnTimeoutMinutes = 5;

	// the pulse setting for the IR Illuminator. 0 is continuous and the rest are number of pulses per second. The LED light mirrors this behavior.
	protected int m_iPulsePerSecond = 0;

	protected ParametricMaterialInstanceComponent m_ParamMatInstComponent;
	protected SignalsManagerComponent m_SignalManagerComponent;
	protected int m_iKnobAnimationSignal = -1;
	protected int m_iSpotAngle = 25;
	protected bool m_bIsAdjustingAngle = false;

	bool IsAdjustingAngle()
	{
		return m_bIsAdjustingAngle;
	}

	void SetAdjustingAngleStatus(bool pNewState)
	{
		m_bIsAdjustingAngle = pNewState;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
	}

	RHS_ANPEQ16_Modes GetMode()
	{
		return m_eCurrentMode;
	}

	int GetPulse()
	{
		return m_iPulsePerSecond;
	}

	void GetSpotlightAngleLimits(out int min, out int max)
	{
		min = s_iMinSpotAngle;
		max = s_iMaxSpotAngle
	}

	override void CycleLightTypes()
	{
		m_eCurrentMode++;
		if (m_eCurrentMode > RHS_ANPEQ16_Modes.DUALHIGH)
			m_eCurrentMode = RHS_ANPEQ16_Modes.AIM;
		else if (m_eCurrentMode == RHS_ANPEQ16_Modes.OFF)
			m_eCurrentMode = RHS_ANPEQ16_Modes.LIGHT;

		SetMode(m_eCurrentMode);

		if (m_CharacterRpcManager)
			m_CharacterRpcManager.AskAuthorityToSyncLightType(this, m_eCurrentMode);
	}

	void SetMode(RHS_ANPEQ16_Modes mode, bool playSound = true)
	{
		m_eCurrentMode = mode;

		m_bIsTurnedOn = m_bIsTurnedOn && m_eCurrentMode != RHS_ANPEQ16_Modes.OFF;
		SwitchDeviceState(m_bIsTurnedOn, false);

		if (m_iKnobAnimationSignal < 0 || !m_SignalManagerComponent)
		{
			m_SignalManagerComponent = SignalsManagerComponent.Cast(this.GetOwner().FindComponent(SignalsManagerComponent));
			if (!m_SignalManagerComponent)
				return;

			m_iKnobAnimationSignal = m_SignalManagerComponent.FindSignal("ANPEQ16State");
		}

		if (m_iKnobAnimationSignal > -1)
		{
			m_SignalManagerComponent.SetSignalValue(m_iKnobAnimationSignal, mode);
			
			if(playSound)
				PlaySound("SOUND_KNOB_CLICK");
		}

		if (m_eCurrentMode == RHS_ANPEQ16_Modes.DUALLOW || m_eCurrentMode == RHS_ANPEQ16_Modes.DUALHIGH)
		{
			// if frequency was previously set, and new mode is ir illum, make strobe
			StrobeEffect(true);
		}
	}

	void SetPulse(int pulsePerSecond = 0)
	{
		m_iPulsePerSecond = pulsePerSecond;
		StrobeEffect(true);
	}

	void StrobeEffect(bool pState)
	{
		if (!m_bIsTurnedOn)
			return;

		int lightId = 3;
		if (m_eCurrentMode == RHS_ANPEQ16_Modes.DUALHIGH)
		{
			lightId = 5;
		}
		else if(m_eCurrentMode != RHS_ANPEQ16_Modes.DUALLOW)
		{
			// only needs to do something if IR illuminator is on
			return;
		}

		GetGame().GetCallqueue().Remove(StrobeEffect);

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pState)
		{
			LEDOn();
			super.ChangeLightVisibility(lightId, true && pc && !pc.RHS_IsNVOff());
		}
		else
		{
			LEDOff();
			super.ChangeLightVisibility(lightId, false && pc && !pc.RHS_IsNVOff());
		}

		if (m_iPulsePerSecond > 0)
			GetGame().GetCallqueue().CallLater(StrobeEffect, 1000 / m_iPulsePerSecond / 2, param1: !pState);
	}

	void DeviceFire()
	{
		m_bIsTurnedOn = true;
		// set timer to approx 1 second
		m_iSwitchOnTimeoutMinutes = 0.016;

		super.SwitchOnLightArray(GetLightArrayFromCurrentMode());

		StartTimeout();
	}

	override void SwitchDeviceState(bool pNewState = false, bool pPlaySound = true)
	{
		m_bIsTurnedOn = pNewState;

		if (pNewState)
		{
			// reset timer to 5 mins
			m_iSwitchOnTimeoutMinutes = 5;

			ToggleLedStatus();
			super.SwitchOnLightArray(GetLightArrayFromCurrentMode(), pPlaySound);

			if (m_eCurrentMode == RHS_ANPEQ16_Modes.DUALLOW || m_eCurrentMode == RHS_ANPEQ16_Modes.DUALHIGH)
			{
				// if frequency was previously set, and new mode is ir illum, make strobe
				StrobeEffect(true);
			}

			StartTimeout();
		}
		else
		{
			super.TurnOffAllLights(pPlaySound);
			ToggleLedStatus();

			// reset the timeout to 5 minutes
			m_iSwitchOnTimeoutMinutes = 5;
		}
	}

	private void StartTimeout()
	{
		int ms = m_iSwitchOnTimeoutMinutes * 60 * 1000;

		//to prevent multiple calls when swiching modes while device was ON
		GetGame().GetCallqueue().Remove(SwitchDeviceState);

		GetGame().GetCallqueue().CallLater(SwitchDeviceState, ms, param1: false, param2: false);
	}

	private array<int> GetLightArrayFromCurrentMode()
	{
		switch (m_eCurrentMode)
		{
			case RHS_ANPEQ16_Modes.OFF:
				return OLA;
			case RHS_ANPEQ16_Modes.DUAL:
				return DLA;
			case RHS_ANPEQ16_Modes.LIGHT:
				return LLA;
			case RHS_ANPEQ16_Modes.AIM:
				return ALA;
			case RHS_ANPEQ16_Modes.AIMLOW:
				return ALLA;
			case RHS_ANPEQ16_Modes.DUALLOW:
				return DLLA;
			case RHS_ANPEQ16_Modes.AIMHIGH:
				return AHLA;
			case RHS_ANPEQ16_Modes.DUALHIGH:
				return DHLA;
		}

		return {};
	}

	private void ToggleLedStatus()
	{
		if (!m_bIsTurnedOn)
		{
			// device is off
			LEDOff();
			return;
		}

		// device is on, mode is off
		if (m_eCurrentMode == RHS_ANPEQ16_Modes.OFF)
		{
			LEDOff();
		} else {
			LEDOn();
		}
	}

	// turns on the status LED. The interval sets whether the LED is continuous (0), blinks 1 per second, 4 or 8 times per second. In line with IR Illum setting
	private void LEDOn()
	{
		if (!m_ParamMatInstComponent)
		{
			m_ParamMatInstComponent = ParametricMaterialInstanceComponent.Cast(this.GetOwner().FindComponent(ParametricMaterialInstanceComponent));
			if (!m_ParamMatInstComponent)
				return;
		}

		m_ParamMatInstComponent.SetEmissiveMultiplier(m_bIsTurnedOn);
	}

	private void LEDOff()
	{
		if (!m_ParamMatInstComponent)
		{
			m_ParamMatInstComponent = ParametricMaterialInstanceComponent.Cast(this.GetOwner().FindComponent(ParametricMaterialInstanceComponent));
			if (!m_ParamMatInstComponent)
				return;
		}

		m_ParamMatInstComponent.SetEmissiveMultiplier(false);
	}

	int GetMaxSpotAngle()
	{
		return s_iMaxSpotAngle;
	}

	int GetMinSpotAngle()
	{
		return s_iMinSpotAngle;
	}

	int GetCurrentSpotAngle()
	{
		return m_iSpotAngle;
	}

	void IncreaseSpotAngle(int pNewValue)
	{
		m_iSpotAngle = Math.Clamp(pNewValue, s_iMinSpotAngle, s_iMaxSpotAngle);
		foreach (int lightId : SPOTLIGHTS)
		{
			m_aLightEntities[m_mLightEntityTypeMap.Get(m_iAvailableLightTypes[lightId])].SetCurrentConeAngle(m_iSpotAngle, 0);
		}
	}

	override void ChangeIRVisibility(bool pNewState)
	{
		super.ChangeIRVisibility(pNewState);
		StrobeEffect(true);
	}

	override bool OnPlayerControllerReady()
	{
		if (!super.OnPlayerControllerReady())
			return false;

		SetMode(m_eCurrentMode, false);
		return true;
	}

	override void OnSlotChanged(InventoryStorageSlot pOldSlot, InventoryStorageSlot pNewSlot)
	{
		super.OnSlotChanged(pOldSlot, pNewSlot);
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		GetGame().GetCallqueue().Remove(StrobeEffect);
	}

	override void RpcDo_SyncLightType(int pNewType)
	{
		SetMode(pNewType);
	}

	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_eCurrentMode);
		writer.WriteInt(m_iPulsePerSecond);
		writer.WriteInt(m_iSpotAngle);

		return super.RplSave(writer);
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadInt(m_eCurrentMode);
		SetMode(m_eCurrentMode);
		reader.ReadInt(m_iPulsePerSecond);
		SetPulse(m_iPulsePerSecond);
		reader.ReadInt(m_iSpotAngle);
		IncreaseSpotAngle(0);

		return super.RplLoad(reader);
	}
}
