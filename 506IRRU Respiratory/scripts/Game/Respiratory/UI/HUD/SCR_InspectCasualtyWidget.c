//! Respiratory mod extension - adds pneumothorax status to casualty inspection

modded class SCR_InspectCasualtyWidget : SCR_InfoDisplayExtended
{
	//------------------------------------------------------------------------------------------------
	override protected void UpdateWidgetData()
	{
		super.UpdateWidgetData();

		if (!m_Target || !m_wCasualtyInspectWidget)
			return;

		if (!m_wPneumothoraxText)
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(m_Target);
		if (!character)
		{
			m_wPneumothoraxText.SetVisible(false);
			return;
		}

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			character.FindComponent(IRRU_PneumothoraxComponent));

		if (pneumo && pneumo.HasPneumothorax())
		{
			int stage = pneumo.GetStage();
			if (stage == IRRU_EPneumothoraxStage.TENSION)
			{
				m_wPneumothoraxText.SetText("PNEUMOTHORAX - TENSION");
				m_wPneumothoraxText.SetColor(Color.FromSRGBA(255, 0, 0, 255));
			}
			else
			{
				m_wPneumothoraxText.SetText("PNEUMOTHORAX");
				m_wPneumothoraxText.SetColor(Color.FromSRGBA(255, 165, 0, 255));
			}
			m_wPneumothoraxText.SetVisible(true);
		}
		else
		{
			m_wPneumothoraxText.SetVisible(false);
		}
	}
}
