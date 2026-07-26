enum RHS_MOUNTED_LIGHT_Modes {
	VISIBLE,
	IR
}

class RHS_WeaponMountedLightDeviceComponentClass : RHS_LightDeviceClass
{
}

class RHS_WeaponMountedLightDeviceComponent : RHS_LightDevice
{
	[Attribute(RHS_MOUNTED_LIGHT_Modes.VISIBLE.ToString(), UIWidgets.ComboBox, "Which mode should be set for light by default - overrides Selected Light Type value", "", ParamEnumArray.FromEnum(RHS_MOUNTED_LIGHT_Modes))]
	private RHS_MOUNTED_LIGHT_Modes m_eCurrentMode;
	
	[Attribute("35", UIWidgets.EditBox, "Maximum spotlight angle", "1 90")]
	private int s_iMaxSpotAngle;
	
	[Attribute("15", UIWidgets.EditBox, "Minimum spotlight angle", "1 90")]
	private int s_iMinSpotAngle;
	
	[Attribute("true", UIWidgets.CheckBox, "Does the device have normal visible light mode?")]
	private bool m_bHasVisibleLightMode;
	
	[Attribute("false", UIWidgets.CheckBox, "Does the device have normal visible light mode?", "1 90")]
	private bool m_bHasIRMode;
	
	[Attribute("false", UIWidgets.CheckBox, "Does the device have strobe mode (this will be enableable for all wavelength modes)?", "1 90")]
	private bool m_bHasStobeMode;
	
	[Attribute("2", desc: "The frequency of strobe in Hz (flashes per second).")]
	protected int m_iStrobeFrequency;
	
	protected int m_iPulsePerSecond = 0;
	
	private int m_iAnimationSignal = -1;
	private int m_iSpotAngle = 25; // 25 15
	private bool m_bIsAdjustingAngle = false;
	
	private const ref array<int> Vis = {2, 0};
	private const ref array<int> IR = {1};
	private const ref array<int> SPOTLIGHTS = {0, 1, 2};
		
	private SignalsManagerComponent m_SignalManagerComponent;
	private int m_iSwitchOnTimeoutMinutes = 0.016;
	

	RHS_MOUNTED_LIGHT_Modes GetMode()
	{
		return m_eCurrentMode;
	}
	
	bool IsAdjustingAngle()
	{
		return m_bIsAdjustingAngle;
	}

	void SetAdjustingAngleStatus(bool pNewState)
	{
		m_bIsAdjustingAngle = pNewState;
	}
	
	bool HasVisMode()
	{
		return m_bHasVisibleLightMode;
	}
	
	bool HasIRMode()
	{
		return m_bHasIRMode;
	}
	
	bool HasStrobeMode()
	{
		return m_bHasStobeMode;
	}
	
	
	bool IsStrobeOn()
	{
		return m_iPulsePerSecond > 0;
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
	}
	
	void GetSpotlightAngleLimits(out int min, out int max)
	{
		min = s_iMinSpotAngle;
		max = s_iMaxSpotAngle;
	}
	
	private array<int> GetLightArrayFromCurrentMode()
	{
		switch (m_eCurrentMode)
		{
			case RHS_MOUNTED_LIGHT_Modes.VISIBLE:
				return Vis;
			case RHS_MOUNTED_LIGHT_Modes.IR:
				return IR;
		}

		return {};
	}
	
	override void CycleLightTypes()
	{
		if (m_eCurrentMode == RHS_MOUNTED_LIGHT_Modes.VISIBLE && m_bHasIRMode)
			m_eCurrentMode = RHS_MOUNTED_LIGHT_Modes.IR;

		SetMode(m_eCurrentMode);

		if (m_CharacterRpcManager)
			m_CharacterRpcManager.AskAuthorityToSyncLightType(this, m_eCurrentMode);
	}

	void SetMode(RHS_MOUNTED_LIGHT_Modes mode)
	{
		m_eCurrentMode = mode;

		SwitchDeviceState(m_bIsTurnedOn, false);

		if (m_iAnimationSignal < 0 || !m_SignalManagerComponent)
		{
			m_SignalManagerComponent = SignalsManagerComponent.Cast(this.GetOwner().FindComponent(SignalsManagerComponent));
			if (!m_SignalManagerComponent)
				return;

			m_iAnimationSignal = m_SignalManagerComponent.FindSignal("LightState");
		}

		if (m_iAnimationSignal > -1)
		{
			m_SignalManagerComponent.SetSignalValue(m_iAnimationSignal, mode);
			PlaySound("SOUND_KNOB_CLICK");
		}
		
		// if frequency was previously set, and new mode is ir illum, make strobe
		if(m_bHasStobeMode)
			StrobeEffect(true);
	}
	
	void EnableStrobe()
	{
		m_iPulsePerSecond = m_iStrobeFrequency;
		StrobeEffect(true);
	}
	
	void DisableStrobe()
	{
		m_iPulsePerSecond = 0;
		
		if (!m_bIsTurnedOn)
			return;
		
		//StrobeEffect(false);
		
		// if we are on we should stay on
		int lightId = 0;
		if (m_eCurrentMode == RHS_MOUNTED_LIGHT_Modes.IR)
		{
			lightId = 1;
		}
		
		// remove recurrent call
		GetGame().GetCallqueue().Remove(StrobeEffect);

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		// remain on
		
		//LEDOn();
		if (m_eCurrentMode == RHS_MOUNTED_LIGHT_Modes.IR)
		{
			super.ChangeLightVisibility(lightId, true && pc && !pc.RHS_IsNVOff());
		} else {
			super.ChangeLightVisibility(lightId, true);
		}
	}

	void StrobeEffect(bool pState)
	{
		if (!m_bIsTurnedOn)
			return;

		int lightId = 0;
		if (m_eCurrentMode == RHS_MOUNTED_LIGHT_Modes.IR)
		{
			lightId = 1;
		}
		
		GetGame().GetCallqueue().Remove(StrobeEffect);

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		if (pState)
		{
			//LEDOn();
			if (m_eCurrentMode == RHS_MOUNTED_LIGHT_Modes.IR)
			{
				super.ChangeLightVisibility(lightId, true && pc && !pc.RHS_IsNVOff());
			} else {
				super.ChangeLightVisibility(lightId, true);
			}
		}
		else
		{
			//LEDOff();
			if (m_eCurrentMode == RHS_MOUNTED_LIGHT_Modes.IR)
			{
				super.ChangeLightVisibility(lightId, false && pc && !pc.RHS_IsNVOff());
			} else {
				super.ChangeLightVisibility(lightId, false);
			}
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
	
	private void StartTimeout()
	{
		int ms = m_iSwitchOnTimeoutMinutes * 60 * 1000;

		//to prevent multiple calls when swiching modes while device was ON
		GetGame().GetCallqueue().Remove(SwitchDeviceState);

		GetGame().GetCallqueue().CallLater(SwitchDeviceState, ms, param1: false, param2: false);
	}
	
	override void SwitchDeviceState(bool pNewState = false, bool pPlaySound = true)
	{
		m_bIsTurnedOn = pNewState;

		if (pNewState)
		{
			super.SwitchOnLightArray(GetLightArrayFromCurrentMode(), pPlaySound);
		}
		else
		{
			DisableStrobe();
			super.TurnOffAllLights(pPlaySound);
		}
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
			if(lightId == 1 && !m_bHasIRMode)
				continue;
			
			m_aLightEntities[m_mLightEntityTypeMap.Get(m_iAvailableLightTypes[lightId])].SetCurrentConeAngle(m_iSpotAngle, 0);
		}
	}
	
	override bool OnPlayerControllerReady()
	{
		if (!super.OnPlayerControllerReady())
			return false;

		SetMode(m_eCurrentMode);
		return true;
	}
	
	override void OnSlotChanged(InventoryStorageSlot pOldSlot, InventoryStorageSlot pNewSlot)
	{
		super.OnSlotChanged(pOldSlot, pNewSlot);
	}
	
	override void RpcDo_SyncLightType(int pNewType)
	{
		SetMode(pNewType);
	}

	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_eCurrentMode);
		writer.WriteInt(m_iSpotAngle);
		writer.WriteInt(m_iPulsePerSecond);

		return super.RplSave(writer);
	}

	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadInt(m_eCurrentMode);
		SetMode(m_eCurrentMode);
		reader.ReadInt(m_iSpotAngle);
		IncreaseSpotAngle(0);
		reader.ReadInt(m_iPulsePerSecond);

		return super.RplLoad(reader);
	}
	
}