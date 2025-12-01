// Thermal Vision Integration for Night Vision Goggles
// This mod hooks into the NVS_NightVisionComponent to add thermal imaging support

modded class NVS_NightVisionComponent
{
	[Attribute("1", desc: "Enable thermal vision when night vision is active", category: "Thermal Vision")]
	protected bool m_bThermalEnabled;
	
	[Attribute("{74E71247FB271851}UI/Materials/Editor/ThermalNVG_Sharp.emat", UIWidgets.ResourceNamePicker, desc: "Thermal imaging material to be applied when night vision is active.", "emat", category: "Thermal Vision")]
	protected ResourceName m_sThermalMaterial;
	
	// Override SetNVEffect to add thermal imaging support
	override void SetNVEffect(bool activate, bool isForced = false)
	{
		// Call the original SetNVEffect first
		super.SetNVEffect(activate, isForced);
		
		// Only proceed if this is the local player or forced
		if (!m_CharacterOwner || (m_CharacterOwner != SCR_PlayerController.GetLocalControlledEntity() && !isForced))
			return;
		
		const BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		
		const int cameraId = world.GetCurrentCameraId();
		
		// Apply or remove thermal imaging effect
		if (activate && m_bIsEquipped && m_bThermalEnabled && !m_sThermalMaterial.IsEmpty())
		{
			world.SetCameraPostProcessEffect(cameraId, 11, PostProcessEffectType.ThermalImaging, m_sThermalMaterial);
		}
		else
		{
			// Remove thermal imaging effect when deactivating or disabled
			if (m_bThermalEnabled && !m_sThermalMaterial.IsEmpty())
				world.SetCameraPostProcessEffect(cameraId, 11, PostProcessEffectType.ThermalImaging, string.Empty);
		}
	}
}

