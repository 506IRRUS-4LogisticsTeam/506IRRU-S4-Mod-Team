//------------------------------------------------------------------------------------------------
//! Dark vignette + blur screen effect for pneumothorax injury
//! Registered in SCR_ScreenEffectsManager on DefaultPlayerController prefab
class IRRU_PneumothoraxScreenEffect : SCR_BaseScreenEffect
{
	// Vignette constants
	protected static const string VIGNETTE_WIDGET_NAME = "IRRU_Medical_BlackFlash";
	protected static const float MIN_OPACITY = 0.2;
	protected static const float MAX_OPACITY = 0.95;
	protected static const float MAX_MASK_PROGRESS = 0.55;
	protected static const float PULSE_AMPLITUDE = 0.08;
	protected static const float PULSE_PERIOD = 3.5;
	protected static const float FADEOUT_SPEED = 0.3;

	// Blur constants
	protected static const string BLUR_WIDGET_NAME = "IRRU_PneumothoraxBlur";
	protected static const float BLUR_MIN_INTENSITY = 0.0;
	protected static const float BLUR_MAX_STAGE1 = 0.25;
	protected static const float BLUR_MAX_STAGE2 = 0.6;
	protected static const float BLUR_PULSE_AMPLITUDE = 0.15;
	protected static const float BLUR_PULSE_PERIOD = 2.5;
	protected static const float BLUR_FADEOUT_SPEED = 0.5;

	protected ImageWidget m_wVignette;
	protected BlurWidget m_wBlur;
	protected IRRU_PneumothoraxComponent m_PneumoComp;
	protected float m_fEffectTimer = 0.0;
	protected float m_fPulsePhase = 0.0;
	protected float m_fCurrentOpacity = 0.0;
	protected float m_fCurrentBlur = 0.0;
	protected float m_fBlurPulsePhase = 0.0;
	protected bool m_bFadingOut = false;
	protected bool m_bEffectActive = false;

	//------------------------------------------------------------------------------------------------
	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		ClearEffects();
		m_PneumoComp = null;

		if (!to)
			return;

		// Find widgets in layout
		if (m_wRoot)
		{
			m_wVignette = ImageWidget.Cast(m_wRoot.FindAnyWidget(VIGNETTE_WIDGET_NAME));
			m_wBlur = BlurWidget.Cast(m_wRoot.FindAnyWidget(BLUR_WIDGET_NAME));
		}

		// Find pneumothorax component on new character
		m_PneumoComp = IRRU_PneumothoraxComponent.Cast(to.FindComponent(IRRU_PneumothoraxComponent));

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
			m_fCurrentBlur = m_fCurrentBlur - BLUR_FADEOUT_SPEED * timeSlice;

			bool vignetteDone = (m_fCurrentOpacity <= 0);
			bool blurDone = (m_fCurrentBlur <= 0);

			if (vignetteDone && blurDone)
			{
				m_fCurrentOpacity = 0;
				m_fCurrentBlur = 0;
				m_bFadingOut = false;
				m_bEffectActive = false;
				HideSingleEffect(m_wVignette);
				HideBlur();
			}
			else
			{
				if (m_fCurrentOpacity < 0)
					m_fCurrentOpacity = 0;
				if (m_fCurrentBlur < 0)
					m_fCurrentBlur = 0;

				float ratio = m_fCurrentOpacity / MAX_OPACITY;
				m_wVignette.SetOpacity(m_fCurrentOpacity);
				m_wVignette.SetMaskProgress(ratio * MAX_MASK_PROGRESS);
				ApplyBlur(m_fCurrentBlur);
			}
			return;
		}

		if (!m_PneumoComp)
			return;

		// Check if pneumothorax started while effect was inactive
		if (!m_bEffectActive)
		{
			if (m_PneumoComp.HasPneumothorax())
				UpdatePneumoEffectState();
			return;
		}

		if (!m_PneumoComp.HasPneumothorax())
		{
			m_bFadingOut = true;
			return;
		}

		int stage = m_PneumoComp.GetStage();
		float opacity;
		float blurIntensity;

		if (stage == IRRU_EPneumothoraxStage.SIMPLE)
		{
			// Exponential ramp (t^1.5) from MIN to MAX over progression time
			m_fEffectTimer += timeSlice;

			float progressionTime = IRRU_PneumothoraxSettings.GetProgressionTime();
			if (progressionTime <= 0)
				progressionTime = 90.0;

			float t = m_fEffectTimer / progressionTime;
			if (t > 1.0)
				t = 1.0;

			float ramp = Math.Pow(t, 1.5);
			opacity = MIN_OPACITY + ramp * (MAX_OPACITY - MIN_OPACITY);
			blurIntensity = BLUR_MIN_INTENSITY + ramp * (BLUR_MAX_STAGE1 - BLUR_MIN_INTENSITY);
		}
		else
		{
			// TENSION: full intensity with sinusoidal pulsing
			m_fPulsePhase += timeSlice;
			if (m_fPulsePhase > PULSE_PERIOD)
				m_fPulsePhase = m_fPulsePhase - PULSE_PERIOD;

			float pulse = Math.Sin(m_fPulsePhase * Math.PI2 / PULSE_PERIOD);
			opacity = MAX_OPACITY + PULSE_AMPLITUDE * pulse;

			// Blur pulses on a faster cycle to simulate gasping
			m_fBlurPulsePhase += timeSlice;
			if (m_fBlurPulsePhase > BLUR_PULSE_PERIOD)
				m_fBlurPulsePhase = m_fBlurPulsePhase - BLUR_PULSE_PERIOD;

			float blurPulse = Math.Sin(m_fBlurPulsePhase * Math.PI2 / BLUR_PULSE_PERIOD);
			blurIntensity = BLUR_MAX_STAGE2 + BLUR_PULSE_AMPLITUDE * blurPulse;
		}

		m_fCurrentOpacity = opacity;
		m_fCurrentBlur = blurIntensity;

		float ratio = opacity / MAX_OPACITY;
		m_wVignette.SetOpacity(opacity);
		m_wVignette.SetMaskProgress(ratio * MAX_MASK_PROGRESS);
		ApplyBlur(blurIntensity);
	}

	//------------------------------------------------------------------------------------------------
	override void ClearEffects()
	{
		m_bEffectActive = false;
		m_bFadingOut = false;
		m_fCurrentOpacity = 0;
		m_fCurrentBlur = 0;
		m_fEffectTimer = 0;
		m_fPulsePhase = 0;
		m_fBlurPulsePhase = 0;

		if (m_wVignette)
			HideSingleEffect(m_wVignette);

		HideBlur();
	}

	//------------------------------------------------------------------------------------------------
	protected override void DisplayOnSuspended()
	{
		if (m_wVignette && (m_bEffectActive || m_bFadingOut))
			m_wVignette.SetOpacity(0);

		HideBlur();
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
	protected void ApplyBlur(float intensity)
	{
		if (!m_wBlur)
			return;

		m_wBlur.SetVisible(true);
		m_wBlur.SetEnabled(true);
		m_wBlur.SetIntensity(intensity);
	}

	//------------------------------------------------------------------------------------------------
	protected void HideBlur()
	{
		if (!m_wBlur)
			return;

		m_wBlur.SetIntensity(0);
		m_wBlur.SetVisible(false);
		m_wBlur.SetEnabled(false);
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

				// Make vignette visible but start at zero — UpdateEffect ramps it
				m_wVignette.SetVisible(true);
				m_wVignette.SetEnabled(true);
				m_wVignette.SetOpacity(0);
				m_wVignette.SetMaskProgress(0);

				// Sync timer with current progression so ramp is accurate
				m_fEffectTimer = m_PneumoComp.GetProgressionTimer();
			}
		}
		else
		{
			if (m_bEffectActive)
				m_bFadingOut = true;
		}
	}
}
