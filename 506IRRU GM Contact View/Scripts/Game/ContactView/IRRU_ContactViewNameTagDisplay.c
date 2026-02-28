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
	//! Check if local player has full GM/admin editor access (not limited/photo mode)
	static bool IsLocalPlayerGM()
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity editorManager = core.GetEditorManager();
		if (!editorManager)
			return false;

		return !editorManager.IsLimited();
	}

	//------------------------------------------------------------------------------------------------
	//! Get gradient contact color: Red (hot/in contact) -> Purple -> Blue (cold/no contact)
	//! Returns white for non-GM players
	static Color GetContactColor(int playerId)
	{
		// Only show contact colors to full GMs, not limited editor users
		if (!IsLocalPlayerGM())
			return new Color(1.0, 1.0, 1.0, 1.0);

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
	//! Get color along a Red -> Purple -> Blue gradient (hot to cold)
	protected static Color GetGradientColor(float progress)
	{
		float r, g, b;

		if (progress < 0.33)
		{
			// Red to Orange/Yellow
			r = 1.0;
			g = 0.5 * (progress / 0.33);
			b = 0.0;
		}
		else if (progress < 0.66)
		{
			// Yellow to Purple (reduce green, increase blue)
			float t = (progress - 0.33) / 0.33;
			r = 1.0 - 0.4 * t;
			g = 0.5 - 0.35 * t;
			b = t;
		}
		else
		{
			// Purple to Blue
			float t = (progress - 0.66) / 0.34;
			r = 0.55 - 0.4 * t;
			g = 0.1;
			b = 1.0;
		}

		return new Color(r, g, b, 1.0);
	}
}
