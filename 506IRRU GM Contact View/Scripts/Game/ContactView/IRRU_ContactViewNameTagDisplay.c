//------------------------------------------------------------------------------------------------
// Mod the Editor player label to show contact status colors
//------------------------------------------------------------------------------------------------
modded class SCR_PlayerEditableEntityUIComponent
{
	protected int m_iCachedPlayerId;
	protected TextWidget m_wCachedNameWidget;
	protected static const int COLOR_UPDATE_INTERVAL_MS = 1000;

	//------------------------------------------------------------------------------------------------
	override void OnInit(SCR_EditableEntityComponent entity, SCR_UIInfo info, SCR_EditableEntityBaseSlotUIComponent slot)
	{
		super.OnInit(entity, info, slot);

		Widget widget = GetWidget();
		if (!widget)
			return;

		m_wCachedNameWidget = TextWidget.Cast(widget.FindAnyWidget(m_sPlayerNameWidgetName));

		SCR_EditablePlayerDelegateComponent delegate = SCR_EditablePlayerDelegateComponent.Cast(entity);
		if (delegate)
		{
			m_iCachedPlayerId = delegate.GetPlayerID();
		}
		else
		{
			SCR_PlayersManagerEditorComponent playersManager = SCR_PlayersManagerEditorComponent.Cast(SCR_PlayersManagerEditorComponent.GetInstance(SCR_PlayersManagerEditorComponent));
			if (playersManager)
				m_iCachedPlayerId = playersManager.GetPlayerID(entity);
		}

		UpdateContactColor();
		ScheduleColorUpdate();
	}

	//------------------------------------------------------------------------------------------------
	override void OnRefresh(SCR_EditableEntityBaseSlotUIComponent slot)
	{
		super.OnRefresh(slot);
		UpdateContactColor();
	}

	//------------------------------------------------------------------------------------------------
	protected void ScheduleColorUpdate()
	{
		GetGame().GetCallqueue().Remove(UpdateContactColorLoop);
		GetGame().GetCallqueue().CallLater(UpdateContactColorLoop, COLOR_UPDATE_INTERVAL_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateContactColorLoop()
	{
		if (!m_wCachedNameWidget || m_iCachedPlayerId <= 0)
			return;

		UpdateContactColor();
		ScheduleColorUpdate();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateContactColor()
	{
		if (!m_wCachedNameWidget || m_iCachedPlayerId <= 0)
			return;

		m_wCachedNameWidget.SetColor(IRRU_ContactViewHelper.GetContactColor(m_iCachedPlayerId));
	}
}

//------------------------------------------------------------------------------------------------
class IRRU_ContactViewHelper
{
	//------------------------------------------------------------------------------------------------
	//! Get gradient contact color: Red (just contacted) -> Orange -> Yellow -> Green (quiet)
	static Color GetContactColor(int playerId)
	{
		IRRU_ContactViewManager manager = IRRU_ContactViewManager.GetInstance();
		if (!manager)
			return new Color(0.5, 0.5, 0.5, 1.0);

		float timeSinceContact = manager.GetTimeSinceContact(playerId);
		if (timeSinceContact < 0)
			return new Color(0.5, 0.5, 0.5, 1.0);

		float progress = Math.Clamp(timeSinceContact / IRRU_ContactViewSettings.GetGreenThreshold(), 0.0, 1.0);

		return GetGradientColor(progress);
	}

	//------------------------------------------------------------------------------------------------
	//! Get color along a Red -> Orange -> Yellow -> Green gradient
	protected static Color GetGradientColor(float progress)
	{
		float r, g;

		if (progress < 0.33)
		{
			// Red to Orange
			r = 1.0;
			g = 0.5 * (progress / 0.33);
		}
		else if (progress < 0.66)
		{
			// Orange to Yellow
			r = 1.0;
			g = 0.5 + 0.5 * ((progress - 0.33) / 0.33);
		}
		else
		{
			// Yellow to Green
			r = 1.0 - ((progress - 0.66) / 0.34);
			g = 1.0;
		}

		return new Color(r, g, 0.0, 1.0);
	}
}
