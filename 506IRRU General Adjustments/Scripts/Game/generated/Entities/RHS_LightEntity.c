class RHS_LightEntityClass : LightEntityClass
{
}

class RHS_LightEntity : LightEntity
{
	/* const */ /* protected */ float REFLECTOR_EFFICENCY = 1.055; // LBT - default 1p045

	[Attribute("0", UIWidgets.CheckBox, "If this light is only visible in IR spectrum", category : "RHS")]
	protected bool m_bIsIR;
	[Attribute("0", UIWidgets.CheckBox, "Can this light lv be adjusted - if true then on activation color and LV will be adjusted", category : "RHS")]
	protected bool m_bCanAdjustLV;
	[Attribute("10", UIWidgets.Auto, "Light LV - since BI one isnt accesiable then this one will be used", category : "RHS")]
	protected float m_fLV;
	[Attribute("0", UIWidgets.CheckBox, "Can this light color be adjusted - if true then on activation color and LV will be adjusted", category : "RHS")]
	protected bool m_bCanAdjustColor;
	[Attribute(Color.White.ToString(), UIWidgets.ColorPicker, "Light color - since BI one isnt accesiable then this one will be used", category : "RHS")]
	protected ref Color m_cLightColor;
	[Attribute("0", UIWidgets.CheckBox, "Can this light angle be adjusted - if true then will adjust cone angle to be equal at least to min and less than max", category : "RHS")]
	/* protected */ bool m_bCanAdjustConeAngle;
	[Attribute("1", UIWidgets.Auto, "Min cone angle", category : "RHS")]
	/* protected */ float m_fMinConeAngle;
	[Attribute("10", UIWidgets.Auto, "Max cone angle", category : "RHS")]
	/* protected */ float m_fMaxConeAngle;
	// LBT ADDED
	protected float m_fMedConeAngle = (m_fMaxConeAngle + m_fMinConeAngle) / 2;
	// END LBT ADDED
	
	protected float m_fCurrentLV = -100;
	
	override void EOnActivate(IEntity owner)
	{
		/*if (m_bCanAdjustLV || m_bCanAdjustColor) // ORIGINAL RHS
		{
			//SetColor(m_cLightColor, m_fLV);
			//m_fCurrentLV = m_fLV;
		}

		if (m_bCanAdjustConeAngle && (GetConeAngle() < m_fMinConeAngle || GetConeAngle() > m_fMaxConeAngle))
			SetCurrentConeAngle(GetConeAngle(), REFLECTOR_EFFICENCY);*/ // END ORIGINAL RHS
		
		if (m_bCanAdjustLV && m_bCanAdjustConeAngle && GetConeAngle() > 80 && !m_bIsIR) // LBT - White light spill
		{
			REFLECTOR_EFFICENCY = 1.010;
			SetColor(m_cLightColor, m_fLV);
			m_fCurrentLV = 4.2; // LBT - default -100 - 6 for default RE - 5.8 for 1p055 RE - 4.2 for 1p010 RE
			//SetLV(GetLV());
			SetCurrentConeAngle(GetConeAngle(), REFLECTOR_EFFICENCY);
		}
		else if (m_bCanAdjustLV && m_bCanAdjustConeAngle) // LBT - White light spot and IR illuminators
		{
			SetColor(m_cLightColor, m_fLV);
			m_fCurrentLV = m_fLV;
			SetCurrentConeAngle(GetConeAngle(), REFLECTOR_EFFICENCY);
		}
		else
		{
			SetColor(m_cLightColor, m_fLV);
			m_fCurrentLV = m_fLV;
			SetCurrentConeAngle(GetConeAngle(), REFLECTOR_EFFICENCY);
		}
	}

	//! Enables the light but check if this is an IR light and local player can see such lights
	void SetEnabledWithIRCheck(bool state)
	{
		if (state && m_bIsIR)
		{
			SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (!pc)
				return;

			if (pc.RHS_IsNVOff())
				return;
		}

		SetEnabled(state);
	}

	bool CanConeAngleBeAdjusted()
	{
		return m_bCanAdjustConeAngle;
	}

	//! Set light cone angle if this light cone angle is meant to be adjsutable
	//! \param newAngle new cone angle which will be clamped to the min and max cone angle values
	//! \param reflectorEffectivnessOverride factor that will be used to decrease lv with if left as -1 then const which is equal 1.035
	void SetCurrentConeAngle(float newAngle)
	{

		if (!m_bCanAdjustConeAngle)
		{
#ifdef WORKBENCH
			Print("RHS_LightConfiguration => SetCurrentConeAngle => Method called despite the fact that this light cone angle is not meant to be adjusted", LogLevel.WARNING);
#endif
			return;
		}

		if (GetConeAngle() == newAngle)
			return;

		SetConeAngle(Math.Clamp(newAngle, m_fMinConeAngle, m_fMaxConeAngle));
	}

	//! Set light cone angle and try to adjust it LV
	//! \param newAngle sets the light entity cone angle and is used to reduce LV
	//! \param reflectorEfficency determines how much LV will be reduced where 1 == 100% efficency and values over 1 decrease LV exponencially by newAngle value - if 0 then static is used
	void SetCurrentConeAngle(float newAngle, float reflectorEfficency)
	{
		SetCurrentConeAngle(newAngle);
		if (reflectorEfficency == 0)
			reflectorEfficency = REFLECTOR_EFFICENCY;

		SetLV(m_fLV - Math.Pow(reflectorEfficency, newAngle));
	}

	//! Get bool value if this light entity is meant to have adjustable color
	bool CanColorBeAdjusted()
	{
		return m_bCanAdjustColor;
	}

	Color GetLightColor()
	{
		return m_cLightColor;
	}

	//! Sets light entity color if color and lv can be adjusted
	void SetLightColor(notnull Color newColor)
	{
		if (!m_bCanAdjustColor)
		{
#ifdef WORKBENCH
			Print("RHS_LightConfiguration => SetLightColor => Method called despite the fact that this light color is not meant to be adjusted", LogLevel.WARNING);
#endif
			return;
		}

		m_cLightColor = newColor;
		SetColor(m_cLightColor, m_fCurrentLV);
	}

	//! Get bool value if this light entity is meant to have adjustable lv
	bool CanLvBeAdjusted()
	{
		return m_bCanAdjustLV;
	}

	float GetDefaultLightLV()
	{
		return m_fLV;
	}

	float GetLV()
	{
		return m_fCurrentLV;
	}

	//! Sets light entity LV if color and lv can be adjusted
	void SetLV(float newLV)
	{
		if (!m_bCanAdjustLV)
		{
#ifdef WORKBENCH
			Print("RHS_LightConfiguration => SetCurrentLightLV => Method called despite the fact that this light LV is not meant to be adjusted", LogLevel.WARNING);
#endif
			return;
		}

		m_fCurrentLV = newLV;
		SetColor(m_cLightColor, m_fCurrentLV);
	}

	bool IsIR()
	{
		return m_bIsIR;
	}
	
	// LBT ADDED
	/*override void EOnDeactivate(IEntity owner)
	{
		
	}*/
	// END LBT ADDED
}