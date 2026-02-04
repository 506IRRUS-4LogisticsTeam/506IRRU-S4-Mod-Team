class RHS_ANPEQ16Action : RHS_ItemActionGlobal
{
	protected RHS_SGC_ANPEQ16Component m_anpeq;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_anpeq = RHS_SGC_ANPEQ16Component.Cast(pOwnerEntity.FindComponent(RHS_SGC_ANPEQ16Component));
	}
}

// ----------- Device State

class RHS_ANPEQ16_Fire_ButtonAction : RHS_ANPEQ16Action
{
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_anpeq || m_anpeq.IsTurnedOn() || m_anpeq.GetMode() == RHS_ANPEQ16_Modes.OFF || m_anpeq.IsAdjustingAngle())
			return false;

		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_anpeq || m_anpeq.IsTurnedOn())
			return;

		m_anpeq.DeviceFire();
	}
}

class RHS_ANPEQ16_On_ButtonAction : RHS_ANPEQ16Action
{
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_anpeq || m_anpeq.IsTurnedOn() || m_anpeq.GetMode() == RHS_ANPEQ16_Modes.OFF || m_anpeq.IsAdjustingAngle())
			return false;

		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_anpeq || m_anpeq.IsTurnedOn())
			return;

		m_anpeq.SwitchDeviceState(true);
	}
}

class RHS_ANPEQ16_Off_ButtonAction : RHS_ANPEQ16Action
{
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_anpeq || !m_anpeq.IsTurnedOn() || m_anpeq.IsAdjustingAngle() || m_anpeq.IsAdjustingAngle())
			return false;

		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_anpeq || !m_anpeq.IsTurnedOn())
			return;

		m_anpeq.SwitchDeviceState(false);
	}
}

// ----------- IR Pulses

class RHS_ANPEQ16_Pulse_SwitchAction : RHS_ANPEQ16Action
{
	[Attribute(string.Empty, UIWidgets.Auto, "String that will instead of shwoing numeric value of frequency", category : "RHS")]
	protected string m_sAlternativePulseName;

	protected int m_iPulse = 0;

	override bool CanBeShownScript(IEntity user)
	{
		if (!m_anpeq || m_anpeq.GetPulse() == m_iPulse || m_anpeq.IsAdjustingAngle())
			return false;

		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_anpeq || m_anpeq.GetPulse() == m_iPulse)
			return;

		m_anpeq.SetPulse(m_iPulse);
	}

	override bool GetActionNameScript(out string outName)
	{
		if (m_sAlternativePulseName == string.Empty)
			m_sAlternativePulseName = m_iPulse.ToString() + " Hz";

		UIInfo actionInfo = GetUIInfo();
		if (actionInfo)
		{
			outName = string.Format("%1 %2", actionInfo.GetName(), m_sAlternativePulseName);
			return true;
		}
		return false;
	}
}

class RHS_ANPEQ16_Pulse_0_SwitchAction : RHS_ANPEQ16_Pulse_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_iPulse = 0;
	}
}

class RHS_ANPEQ16_Pulse_1_SwitchAction : RHS_ANPEQ16_Pulse_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_iPulse = 1;
	}
}

class RHS_ANPEQ16_Pulse_4_SwitchAction : RHS_ANPEQ16_Pulse_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_iPulse = 4;
	}
}

class RHS_ANPEQ16_Pulse_8_SwitchAction : RHS_ANPEQ16_Pulse_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_iPulse = 8;
	}
}

// ----------- MODES

class RHS_ANPEQ16_Mode_SwitchAction : RHS_ANPEQ16Action
{
	protected RHS_ANPEQ16_Modes m_eMode = RHS_ANPEQ16_Modes.OFF;
	protected string m_sModeName;

	override bool CanBeShownScript(IEntity user)
	{
		if (!m_anpeq || m_anpeq.GetMode() == m_eMode || m_anpeq.IsAdjustingAngle())
			return false;

		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_anpeq || m_anpeq.GetMode() == m_eMode)
			return;

		m_anpeq.SetMode(m_eMode);
	}

	override bool GetActionNameScript(out string outName)
	{
		UIInfo actionInfo = GetUIInfo();
		if (actionInfo)
		{
			outName = string.Format("%1 %2", actionInfo.GetName(), m_sModeName);
			return true;
		}
		return false;
	}
}

class RHS_ANPEQ16_Mode_O_SwitchAction : RHS_ANPEQ16_Mode_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_eMode = RHS_ANPEQ16_Modes.OFF;
		m_sModeName = "O";
	}
}

class RHS_ANPEQ16_Mode_D_SwitchAction : RHS_ANPEQ16_Mode_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_eMode = RHS_ANPEQ16_Modes.DUAL;
		m_sModeName = "D";
	}
}

class RHS_ANPEQ16_Mode_L_SwitchAction : RHS_ANPEQ16_Mode_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_eMode = RHS_ANPEQ16_Modes.LIGHT;
		m_sModeName = "L";
	}
}

class RHS_ANPEQ16_Mode_A_SwitchAction : RHS_ANPEQ16_Mode_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_eMode = RHS_ANPEQ16_Modes.AIM;
		m_sModeName = "A";
	}
}

class RHS_ANPEQ16_Mode_AL_SwitchAction : RHS_ANPEQ16_Mode_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_eMode = RHS_ANPEQ16_Modes.AIMLOW;
		m_sModeName = "AL";
	}
}

class RHS_ANPEQ16_Mode_DL_SwitchAction : RHS_ANPEQ16_Mode_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_eMode = RHS_ANPEQ16_Modes.DUALLOW;
		m_sModeName = "DL";
	}
}

class RHS_ANPEQ16_Mode_AH_SwitchAction : RHS_ANPEQ16_Mode_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_eMode = RHS_ANPEQ16_Modes.AIMHIGH;
		m_sModeName = "AH";
	}
}

class RHS_ANPEQ16_Mode_DH_SwitchAction : RHS_ANPEQ16_Mode_SwitchAction
{
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_eMode = RHS_ANPEQ16_Modes.DUALHIGH;
		m_sModeName = "DH";
	}
}

class RHS_ANPEQ16_SpotAngleChange : SCR_AdjustSignalAction
{
	protected RHS_SGC_ANPEQ16Component m_anpeq;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_anpeq = RHS_SGC_ANPEQ16Component.Cast(pOwnerEntity.FindComponent(RHS_SGC_ANPEQ16Component));
	}

	override bool CanBeShownScript(IEntity user)
	{
		if (!m_anpeq)
			return false;

		return true;
	}

	override void OnActionStart(IEntity pUserEntity)
	{
		if (!m_anpeq)
			return;

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		m_bIsAdjustedByPlayer = pc && pc.GetMainEntity() == pUserEntity;
		m_anpeq.SetAdjustingAngleStatus(m_bIsAdjustedByPlayer);

		if (!m_bIsAdjustedByPlayer)
			return;

		m_fTargetValue = m_anpeq.GetCurrentSpotAngle();

		if (!m_sActionIncrease.IsEmpty())
			GetGame().GetInputManager().AddActionListener(m_sActionIncrease, EActionTrigger.VALUE, HandleAction);
	}

	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_anpeq)
			return;

		if (!m_bIsAdjustedByPlayer)
			return;

		m_bIsAdjustedByPlayer = false;
		m_anpeq.SetAdjustingAngleStatus(m_bIsAdjustedByPlayer);

		if (!m_sActionIncrease.IsEmpty())
			GetGame().GetInputManager().RemoveActionListener(m_sActionIncrease, EActionTrigger.VALUE, HandleAction);
	}

	override void HandleAction(float value)
	{
		if (m_fAdjustmentStep <= 0)
			return;

		if (value > 0.5)
			value = m_fAdjustmentStep;
		else if (value < -0.5)
			value = -m_fAdjustmentStep;
		else
			return;

		// Limit to normalized current value +/- adjustment limit
		float previousValue = m_fTargetValue;
		m_fTargetValue += value;

		// Round to adjustment step
		m_fTargetValue = Math.Round(m_fTargetValue / m_fAdjustmentStep) * m_fAdjustmentStep;

		// Limit to min/max value
		m_fTargetValue = Math.Clamp(m_fTargetValue, m_anpeq.GetMinSpotAngle(), m_anpeq.GetMaxSpotAngle());

		if (!float.AlmostEqual(m_fTargetValue, previousValue))
			SetSendActionDataFlag();
	}

	//------------------------------------------------------------------------------------------------
	override float GetActionProgressScript(float fProgress, float timeSlice)
	{
		if (!m_anpeq || m_anpeq.GetMaxSpotAngle() - m_anpeq.GetMinSpotAngle() == 0)
			return super.GetActionProgressScript(fProgress, timeSlice);

		return (m_anpeq.GetCurrentSpotAngle() - m_anpeq.GetMinSpotAngle()) / (m_anpeq.GetMaxSpotAngle() - m_anpeq.GetMinSpotAngle());
	}

	override bool HasLocalEffectOnlyScript() { return false; };

	override bool CanBroadcastScript() { return true; };

	override protected bool OnSaveActionData(ScriptBitWriter writer)
	{
		writer.WriteFloat(m_fTargetValue);
		m_anpeq.IncreaseSpotAngle(m_fTargetValue);

		return true;
	};

	override protected bool OnLoadActionData(ScriptBitReader reader)
	{
		if (m_bIsAdjustedByPlayer)
			return true;

		reader.ReadFloat(m_fTargetValue);
		m_anpeq.IncreaseSpotAngle(m_fTargetValue);

		return true;
	};
}
