//------------------------------------------------------------------------------------------------
//! Dark vignette screen effect for pneumothorax injury
//! Registered in SCR_ScreenEffectsManager on DefaultPlayerController prefab
class IRRU_PneumothoraxScreenEffect : SCR_BaseScreenEffect
{
	protected static const string VIGNETTE_WIDGET_NAME = "IRRU_Medical_BlackFlash";
	protected static const float MIN_OPACITY = 0.1;
	protected static const float MAX_OPACITY = 0.87;
	protected static const float MAX_MASK_PROGRESS = 0.4;
	protected static const float PULSE_AMPLITUDE = 0.05;
	protected static const float PULSE_PERIOD = 4.0;
	protected static const float FADEOUT_SPEED = 0.3;

	protected ImageWidget m_wVignette;
	protected IRRU_PneumothoraxComponent m_PneumoComp;
	protected float m_fEffectTimer = 0.0;
	protected float m_fPulsePhase = 0.0;
	protected float m_fCurrentOpacity = 0.0;
	protected bool m_bFadingOut = false;
	protected bool m_bEffectActive = false;

	//------------------------------------------------------------------------------------------------
	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		ClearEffects();
		m_PneumoComp = null;

		if (!to)
			return;

		// Find the vignette widget in layout
		if (m_wRoot)
			m_wVignette = ImageWidget.Cast(m_wRoot.FindAnyWidget(VIGNETTE_WIDGET_NAME));

		// Find pneumothorax component on new character
		m_PneumoComp = IRRU_PneumothoraxComponent.Cast(to.FindComponent(IRRU_PneumothoraxComponent));

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
			Print(string.Format("[PneumoEffect] DisplayControlledEntityChanged - Widget: %1 | PneumoComp: %2", m_wVignette, m_PneumoComp));

		if (!m_PneumoComp)
			return;

		UpdatePneumoEffectState();
	}

	//------------------------------------------------------------------------------------------------
	override void UpdateEffect(float timeSlice)
	{
		if (!m_wVignette)
			return;

		// Handle fade-out after treatment
		if (m_bFadingOut)
		{
			m_fCurrentOpacity = m_fCurrentOpacity - FADEOUT_SPEED * timeSlice;
			if (m_fCurrentOpacity <= 0)
			{
				m_fCurrentOpacity = 0;
				m_bFadingOut = false;
				m_bEffectActive = false;
				HideSingleEffect(m_wVignette);
			}
			else
			{
				float ratio = m_fCurrentOpacity / MAX_OPACITY;
				m_wVignette.SetOpacity(m_fCurrentOpacity);
				m_wVignette.SetMaskProgress(ratio * MAX_MASK_PROGRESS);
			}
			return;
		}

		if (!m_PneumoComp)
			return;

		// Check if pneumothorax started while effect was inactive
		if (!m_bEffectActive)
		{
			if (m_PneumoComp.HasPneumothorax())
			{
				if (IRRU_PneumothoraxSettings.IsDebugEnabled())
					Print(string.Format("[PneumoEffect] Detected pneumothorax start - Stage: %1 | ProgTimer: %2", m_PneumoComp.GetStage(), m_PneumoComp.GetProgressionTimer()));
				UpdatePneumoEffectState();
			}
			return;
		}

		if (!m_PneumoComp.HasPneumothorax())
		{
			m_bFadingOut = true;
			return;
		}

		int stage = m_PneumoComp.GetStage();
		float opacity;

		if (stage == IRRU_EPneumothoraxStage.SIMPLE)
		{
			// Exponential ramp (t^1.5) from MIN_OPACITY to MAX_OPACITY over progression time
			m_fEffectTimer += timeSlice;

			float progressionTime = IRRU_PneumothoraxSettings.GetProgressionTime();
			if (progressionTime <= 0)
				progressionTime = 90.0;

			float t = m_fEffectTimer / progressionTime;
			if (t > 1.0)
				t = 1.0;

			float ramp = Math.Pow(t, 1.5);
			opacity = MIN_OPACITY + ramp * (MAX_OPACITY - MIN_OPACITY);
		}
		else
		{
			// TENSION: full intensity with sinusoidal pulsing
			m_fPulsePhase += timeSlice;
			if (m_fPulsePhase > PULSE_PERIOD)
				m_fPulsePhase = m_fPulsePhase - PULSE_PERIOD;

			float pulse = Math.Sin(m_fPulsePhase * Math.PI2 / PULSE_PERIOD);
			opacity = MAX_OPACITY + PULSE_AMPLITUDE * pulse;
		}

		m_fCurrentOpacity = opacity;
		float ratio = opacity / MAX_OPACITY;
		m_wVignette.SetOpacity(opacity);
		m_wVignette.SetMaskProgress(ratio * MAX_MASK_PROGRESS);
	}

	//------------------------------------------------------------------------------------------------
	override void ClearEffects()
	{
		m_bEffectActive = false;
		m_bFadingOut = false;
		m_fCurrentOpacity = 0;
		m_fEffectTimer = 0;
		m_fPulsePhase = 0;

		if (m_wVignette)
			HideSingleEffect(m_wVignette);
	}

	//------------------------------------------------------------------------------------------------
	protected override void DisplayOnSuspended()
	{
		if (m_wVignette && (m_bEffectActive || m_bFadingOut))
			m_wVignette.SetOpacity(0);
	}

	//------------------------------------------------------------------------------------------------
	protected override void DisplayOnResumed()
	{
		UpdatePneumoEffectState();
	}

	//------------------------------------------------------------------------------------------------
	override protected void DisplayConsciousnessChanged(bool conscious, bool init)
	{
		if (init)
			return;

		UpdatePneumoEffectState();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePneumoEffectState()
	{
		if (!m_PneumoComp || !m_wVignette)
			return;

		if (m_PneumoComp.HasPneumothorax())
		{
			if (!m_bEffectActive)
			{
				m_bEffectActive = true;
				m_bFadingOut = false;

				// Make widget visible but start at zero — UpdateEffect ramps it
				m_wVignette.SetVisible(true);
				m_wVignette.SetEnabled(true);
				m_wVignette.SetOpacity(0);
				m_wVignette.SetMaskProgress(0);

				// Sync timer with current progression so ramp is accurate
				m_fEffectTimer = m_PneumoComp.GetProgressionTimer();

				if (IRRU_PneumothoraxSettings.IsDebugEnabled())
					Print(string.Format("[PneumoEffect] Effect STARTED - SyncTimer: %1 | Stage: %2", m_fEffectTimer, m_PneumoComp.GetStage()));
			}
		}
		else
		{
			if (m_bEffectActive)
				m_bFadingOut = true;
		}
	}
}
