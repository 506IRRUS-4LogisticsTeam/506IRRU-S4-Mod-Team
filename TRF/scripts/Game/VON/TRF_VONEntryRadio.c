modded class SCR_VONEntryRadio : SCR_VONEntry
{
	// ------------------------------------------------------------------ constants
	const string LABEL_FREQUENCY_UNITS = "#AR‑VON_FrequencyUnits_MHz";

	// ------------------------------------------------------------------ state
	protected int               m_iFrequency;
	protected int               m_iTransceiverNumber;
	protected string            m_sChannelText;
	protected BaseTransceiver   m_RadioTransceiver;
	protected SCR_GadgetComponent m_GadgetComp;

	// ------------------------------------------------------------------ helpers
	protected IEntity GetPlayerEntity()
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		return pc ? pc.GetControlledEntity() : null;
	}

	// ------------------------------------------------------------------ frequency knob
	override void AdjustEntryModif(int modifier)
	{
		if (!IsUsable() && modifier != 0)
			return;

		// lazy‑initialise the transceiver from the player’s gadget
		if (!m_RadioTransceiver)
		{
			IEntity ply = GetPlayerEntity();
			if (ply)
			{
				m_GadgetComp      = SCR_GadgetComponent.Cast(ply.FindComponent(SCR_GadgetComponent));
				m_RadioTransceiver = m_GadgetComp ? BaseTransceiver.Cast(m_GadgetComp.GetCurrentGadget()) : null;
			}
		}
		if (!m_RadioTransceiver)
			return;

		m_iFrequency  = m_RadioTransceiver.GetFrequency();
		int minFreq   = m_RadioTransceiver.GetMinFrequency();
		int maxFreq   = m_RadioTransceiver.GetMaxFrequency();

		if ((modifier > 0 && m_iFrequency == maxFreq) ||
		    (modifier < 0 && m_iFrequency == minFreq))
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_CHANGEFREQUENCY_ERROR);
		}
		else if (modifier != 0)
		{
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_CHANGEFREQUENCY);
		}

		m_iFrequency  = Math.ClampInt(
							m_iFrequency + modifier * m_RadioTransceiver.GetFrequencyResolution(),
							minFreq, maxFreq);

		float fFrequency = Math.Round(m_iFrequency * 0.1) * 0.01;
		m_sText = string.Format("%.1f %1", fFrequency, LABEL_FREQUENCY_UNITS);
	}

	// ------------------------------------------------------------------ routing button
	void CycleRouting()
	{
		IEntity ply = GetPlayerEntity();
		if (!ply)
		{
			Print("SCR_VONEntryRadio: No local player entity.", LogLevel.WARNING);
			return;
		}

		VONRoutingComponent routing = VONRoutingComponent.Cast(ply.FindComponent(VONRoutingComponent));
		if (!routing)
		{
			Print("SCR_VONEntryRadio: Player missing VONRoutingComponent.", LogLevel.WARNING);
			return;
		}

		EVONAudioRouting newRouting = routing.GetNextRouting(routing.GetCurrentRouting());
		routing.ApplyRouting(newRouting);
		routing.ShowRoutingHint("VON channel routed to " + routing.RoutingToString(newRouting));
	}
}
