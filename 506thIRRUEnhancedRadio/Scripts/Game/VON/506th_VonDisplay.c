modded class SCR_VonDisplay
{
	override protected bool UpdateTransmission(TransmissionData data, BaseTransceiver radioTransceiver, int frequency, bool IsReceiving)
	{
		bool result = super.UpdateTransmission(data, radioTransceiver, frequency, IsReceiving);

		if (!IsReceiving && radioTransceiver && result)
		{
			SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
			if (settings.IsTransmittingOnAlternate())
				data.m_Widgets.m_wFrequency.SetColor(Color.FromInt(Color.CYAN));
		}

		return result;
	}
}
