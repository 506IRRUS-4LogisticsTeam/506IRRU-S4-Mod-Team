modded class SCR_VONEntryRadio : SCR_VONEntry
{
	const string LABEL_FREQUENCY_UNITS = "#AR-VON_FrequencyUnits_MHz";

	protected int m_iFrequency;
	protected int m_iTransceiverNumber;
	protected string m_sChannelText;

	protected BaseTransceiver m_RadioTransceiver;
	protected SCR_GadgetComponent m_GadgetComp;

	void SetOwner(IEntity owner)
	{
		m_Owner = owner;
	}

	override void AdjustEntryModif(int modifier)
	{
		if (!IsUsable() && modifier != 0)
			return;

		if (!m_RadioTransceiver)
			return;

		m_iFrequency = m_RadioTransceiver.GetFrequency();
		int minFreq = m_RadioTransceiver.GetMinFrequency();
		int maxFreq = m_RadioTransceiver.GetMaxFrequency();

		if ((modifier > 0  && m_iFrequency == maxFreq) || (modifier < 0 && m_iFrequency == minFreq))
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_CHANGEFREQUENCY_ERROR);
		else if (modifier != 0)
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_RADIO_CHANGEFREQUENCY);

		m_iFrequency = m_iFrequency + (modifier * m_RadioTransceiver.GetFrequencyResolution());
		m_iFrequency = Math.ClampInt(m_iFrequency, minFreq, maxFreq);

		float fFrequency = Math.Round(m_iFrequency * 0.1) * 0.01;
		m_sText = string.Format("%.1f %1", fFrequency, LABEL_FREQUENCY_UNITS);
	}

	void CycleRouting()
	{
		// Get local player controller and controlled entity
		SCR_PlayerController localPlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!localPlayerController)
		{
			Print("TRF_VONEntryRadio: Could not find local player controller.", LogLevel.WARNING);
			return;
		}
		
		IEntity controlledEntity = localPlayerController.GetControlledEntity();
		if (!controlledEntity)
		{
			Print("TRF_VONEntryRadio: Local player controller has no controlled entity.", LogLevel.WARNING);
			return;
		}

		// Find the VONRoutingComponent on the controlled entity
		VONRoutingComponent vonRoutingComponent = VONRoutingComponent.Cast(controlledEntity.FindComponent(VONRoutingComponent));

		if (!vonRoutingComponent)
		{
			Print("TRF_VONEntryRadio: Controlled entity is missing VONRoutingComponent.", LogLevel.WARNING);
			return;
		}
		
		// Cycle routing using the found component
		EVONAudioRouting currentRouting = vonRoutingComponent.GetCurrentRouting();
		EVONAudioRouting newRouting = vonRoutingComponent.GetNextRouting(currentRouting);

		vonRoutingComponent.ApplyRouting(newRouting);
		vonRoutingComponent.ShowRoutingHint("VON channel routed to " + vonRoutingComponent.RoutingToString(newRouting));
	}
}
