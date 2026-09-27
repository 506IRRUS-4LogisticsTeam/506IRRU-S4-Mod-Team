// RR_VisionBinocularsComponent.c
// Scripts/Game/Components/SOFLAM/RR_VisionBinocularsComponent.c
//
// Extends RVX_SoflamSightsComponent instead of copying it, so every RVX script that looks for
// RVX_SoflamSightsComponent (FindComponent / Cast) still finds this component.
// Keep RVX_SoflamSightsComponent.c in the project (it must compile) - just remove it from the prefab.
//
// Adds: Normal / NV / Thermal cycling and stepped zoom while looking through the SOFLAM.

[EntityEditorProps(category: "GameScripted/Weapon/Sights", description: "SOFLAM sights with vision modes and zoom", color: "0 0 255 255")]
class RR_VisionBinocularsComponentClass : RVX_SoflamSightsComponentClass
{
}

enum RR_EVisionMode
{
	NORMAL,
	NIGHT,
	THERMAL
}

class RR_VisionBinocularsComponent : RVX_SoflamSightsComponent
{
	// ---------------- Vision modes ----------------
	[Attribute("", UIWidgets.ResourceNamePicker, "Night vision material (.emat)", "emat", category: "Vision Modes")]
	protected ResourceName m_sNightVisionMaterial;

	[Attribute("", UIWidgets.ResourceNamePicker, "Thermal material (.emat)", "emat", category: "Vision Modes")]
	protected ResourceName m_sThermalMaterial;

	[Attribute("20", UIWidgets.EditBox, "Post-process slot priority - use one vanilla does not use", category: "Vision Modes")]
	protected int m_iEffectPriority;

	// ---------------- Zoom ----------------
	[Attribute("12", UIWidgets.EditBox, "Magnification at full zoom-in (normal = the component's Magnification)", category: "Zoom")]
	protected float m_fMaxMagnification;

	[Attribute("1", UIWidgets.EditBox, "Steps between normal and max (1 = single jump)", category: "Zoom")]
	protected int m_iZoomSteps;

	// Input names - must match your chimeraInputCommon.conf override
	protected const string INPUT_CONTEXT  = "RR_OpticsContext";
	protected const string INPUT_CYCLE    = "RR_OpticsCycleVision";
	protected const string INPUT_ZOOM_IN  = "RR_OpticsZoomIn";
	protected const string INPUT_ZOOM_OUT = "RR_OpticsZoomOut";

	// Optional: add a TextWidget named "Text_VisionMode" to the layout
	protected TextWidget visionModeWidget;

	protected RR_EVisionMode m_eVisionMode = RR_EVisionMode.NORMAL;
	protected float m_fBaseMagnification;
	protected bool m_bBaseMagnificationStored;
	protected int m_iZoomStep;
	protected bool m_bVisionActive;

	//------------------------------------------------------------------------------------------------
	RR_EVisionMode GetVisionMode()
	{
		return m_eVisionMode;
	}

	//------------------------------------------------------------------------------------------------
	// Hooks into the original SOFLAM behaviour
	//------------------------------------------------------------------------------------------------
	protected override void OnSightADSActivated()
	{
		// Original calls soflamComponent.ToggleOn() without a null check
		if (!soflamComponent)
			Init(GetOwner());

		// Register with the SOFLAM component so its sightComponent is never null
		if (soflamComponent)
			soflamComponent.RR_SetSights(this);

		super.OnSightADSActivated();

		StartVision();
	}

	protected override void OnSightADSDeactivated()
	{
		StopVision();

		super.OnSightADSDeactivated();

		// Original removes the overlay but keeps the reference, so it never comes back.
		// Clearing it makes the overlay get recreated next time.
		root = null;
	}

	override void UpdateUI()
	{
		super.UpdateUI();

		if (IsUsingSights())
			UpdateVisionModeText();
	}

	protected override void FindWidgets()
	{
		super.FindWidgets();
		visionModeWidget = TextWidget.Cast(root.FindAnyWidget("Text_VisionMode"));
	}

	// Cleanup if the SOFLAM is deleted while being looked through
	void ~RR_VisionBinocularsComponent()
	{
		if (!GetGame())
			return;

		StopVision();
	}

	//------------------------------------------------------------------------------------------------
	// Vision / zoom handling
	//------------------------------------------------------------------------------------------------
	protected void StartVision()
	{
		if (!IsLocalPlayerOwner())
			return;

		if (!m_bBaseMagnificationStored)
		{
			m_fBaseMagnification = m_fMagnification;
			m_bBaseMagnificationStored = true;
		}

		m_bVisionActive = true;
		m_iZoomStep = 0;
		ApplyZoom();
		ApplyVisionEffect();

		InputManager input = GetGame().GetInputManager();
		input.AddActionListener(INPUT_CYCLE,    EActionTrigger.DOWN, OnCycleVision);
		input.AddActionListener(INPUT_ZOOM_IN,  EActionTrigger.DOWN, OnZoomIn);
		input.AddActionListener(INPUT_ZOOM_OUT, EActionTrigger.DOWN, OnZoomOut);

		// Input contexts must be activated every frame they should be live
		GetGame().GetCallqueue().CallLater(KeepContextActive, 0, true);
	}

	protected void StopVision()
	{
		if (!m_bVisionActive)
			return;

		m_bVisionActive = false;

		GetGame().GetCallqueue().Remove(KeepContextActive);

		InputManager input = GetGame().GetInputManager();
		input.RemoveActionListener(INPUT_CYCLE,    EActionTrigger.DOWN, OnCycleVision);
		input.RemoveActionListener(INPUT_ZOOM_IN,  EActionTrigger.DOWN, OnZoomIn);
		input.RemoveActionListener(INPUT_ZOOM_OUT, EActionTrigger.DOWN, OnZoomOut);

		// Remove our post-process; the selected mode is remembered for next time
		ClearVisionEffect();

		// Reset zoom to normal
		m_iZoomStep = 0;
		ApplyZoom();
	}

	protected void KeepContextActive()
	{
		GetGame().GetInputManager().ActivateContext(INPUT_CONTEXT);
	}

	protected void OnCycleVision(float value = 0.0, EActionTrigger reason = 0)
	{
		m_eVisionMode = ((int)m_eVisionMode + 1) % 3;

		// Skip modes that have no material assigned
		if (m_eVisionMode == RR_EVisionMode.NIGHT && m_sNightVisionMaterial.IsEmpty())
			m_eVisionMode = RR_EVisionMode.THERMAL;
		if (m_eVisionMode == RR_EVisionMode.THERMAL && m_sThermalMaterial.IsEmpty())
			m_eVisionMode = RR_EVisionMode.NORMAL;

		ApplyVisionEffect();
		UpdateVisionModeText();
	}

	protected void OnZoomIn(float value = 0.0, EActionTrigger reason = 0)
	{
		m_iZoomStep = Math.ClampInt(m_iZoomStep + 1, 0, Math.Max(m_iZoomSteps, 1));
		ApplyZoom();
	}

	protected void OnZoomOut(float value = 0.0, EActionTrigger reason = 0)
	{
		m_iZoomStep = Math.ClampInt(m_iZoomStep - 1, 0, Math.Max(m_iZoomSteps, 1));
		ApplyZoom();
	}

	protected void ApplyVisionEffect()
	{
		ResourceName material;
		switch (m_eVisionMode)
		{
			case RR_EVisionMode.NIGHT:   material = m_sNightVisionMaterial; break;
			case RR_EVisionMode.THERMAL: material = m_sThermalMaterial;     break;
		}

		if (material.IsEmpty())
		{
			ClearVisionEffect();
			return;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		world.SetCameraPostProcessEffect(world.GetCurrentCameraId(), m_iEffectPriority, PostProcessEffectType.HDR, material);
	}

	protected void ClearVisionEffect()
	{
		BaseWorld world = GetGame().GetWorld();
		if (world)
			world.SetCameraPostProcessEffect(world.GetCurrentCameraId(), m_iEffectPriority, PostProcessEffectType.None, "");
	}

	protected void ApplyZoom()
	{
		if (!m_bBaseMagnificationStored)
			return;

		float t = m_iZoomStep / (float)Math.Max(m_iZoomSteps, 1);
		m_fMagnification = Math.Lerp(m_fBaseMagnification, m_fMaxMagnification, t);

		// VERSION-DEPENDENT: check SCR_2DSightsComponent.c / SCR_2DOpticsComponent.c for how
		// m_fMagnification becomes the camera FOV. If m_fFovZoomed / CalculateZoomFOV() don't
		// exist in your version, replace this line with whatever those files use.
		m_fFovZoomed = CalculateZoomFOV(m_fMagnification);
	}

	protected void UpdateVisionModeText()
	{
		if (!visionModeWidget)
			return;

		switch (m_eVisionMode)
		{
			case RR_EVisionMode.NIGHT:   visionModeWidget.SetText("NV");  break;
			case RR_EVisionMode.THERMAL: visionModeWidget.SetText("TI");  break;
			default:                     visionModeWidget.SetText("DAY"); break;
		}
	}

	protected bool IsLocalPlayerOwner()
	{
		IEntity rootEnt = GetOwner().GetRootParent();
		return rootEnt && rootEnt == SCR_PlayerController.GetLocalControlledEntity();
	}
}