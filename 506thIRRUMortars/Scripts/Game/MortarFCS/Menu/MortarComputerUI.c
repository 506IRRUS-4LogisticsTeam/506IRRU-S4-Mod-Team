// 506thIRRUMortars/Scripts/Game/MortarFCS/Menu/MortarComputerUI.c
// Minimal menu that pops the map for the mortar fire-control system.

[EntityEditorProps(category: "Menu", description: "Mortar fire-control UI")]
class MortarComputerUI : ChimeraMenuBase
{
	protected SCR_MapEntity   m_MapEntity;
	protected SCR_ChatPanel   m_ChatPanel;          // not critical – keeps chat hotkey functional

	// Called once when menu is spawned
	override void OnMenuInit()
	{
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}

	// Called as soon as the menu becomes visible
	override void OnMenuOpen()
	{
		if (!m_MapEntity)      // failsafe: still not ready? bail.
			return;

		//Configure the map for full-screen FCS mode (own .conf is optional but meh???)
		MapConfiguration cfg = m_MapEntity.SetupMapConfig(
			EMapEntityMode.FULLSCREEN,
			"{3C6C98B0E342CAA0}Configs/Map/MapArtilleryComputer.conf",   // reuse vanil
			GetRootWidget()
		);

		//Show the map
		m_MapEntity.OpenMap(cfg);
		m_MapEntity.GetOnMapOpen().Insert(OnMapOpen);

		//Bind ‘ESC’ (same input as tutorial map) for quick close
		InputManager im = GetGame().GetInputManager();
		if (im)
			im.AddActionListener("TutorialFastTravelMapMenuClose", EActionTrigger.DOWN, Close);
	}

	override void OnMenuClose()
	{
		// Unhook input
		InputManager im = GetGame().GetInputManager();
		if (im)
			im.RemoveActionListener("TutorialFastTravelMapMenuClose", EActionTrigger.DOWN, Close);

		// Close map if still open
		if (m_MapEntity)
		{
			m_MapEntity.CloseMap();
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_HUD_MAP_CLOSE);
		}

		// De-register open-callback just in case
		m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
	}

	override void OnMenuUpdate(float tDelta)
	{
		// Keep chat alive so players can still type while in FCS
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
	}

	protected void OnMapOpen(MapConfiguration cfg)
	{
		// One-shot sound & any post-init you need
		m_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_HUD_MAP_OPEN);
	}
}
