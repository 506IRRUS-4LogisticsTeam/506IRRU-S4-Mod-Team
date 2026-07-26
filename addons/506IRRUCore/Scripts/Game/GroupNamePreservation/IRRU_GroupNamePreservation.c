modded class SCR_HUDGroupUIComponent
{
	protected static ref map<string, string> s_mPreservedGroupNames = new map<string, string>();

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		// Poll group-name widgets because game-side join/leave updates can overwrite text after attach.
		GetGame().GetCallqueue().CallLater(IRRU_PreserveGroupNameTick, 150, true);
	}

	protected void IRRU_PreserveGroupNameTick()
	{
		if (!m_wRoot)
			return;

		SCR_AIGroup group = IRRU_GetLocalPlayerGroup();
		if (!group)
			return;

		TextWidget groupNameWidget = IRRU_FindGroupNameWidget();
		if (!groupNameWidget)
			return;

		string currentName = groupNameWidget.GetText();
		currentName.Trim();

		string groupKey = IRRU_BuildGroupKey(group);
		if (groupKey.IsEmpty())
			return;

		if (!IRRU_IsDefaultOrEmptyGroupName(currentName))
		{
			s_mPreservedGroupNames[groupKey] = currentName;
			return;
		}

		string preservedName;
		if (!s_mPreservedGroupNames.Find(groupKey, preservedName))
			return;

		if (preservedName.IsEmpty())
			return;

		if (groupNameWidget.GetText() != preservedName)
			groupNameWidget.SetText(preservedName);
	}

	protected SCR_AIGroup IRRU_GetLocalPlayerGroup()
	{
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return null;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return null;

		return groupsManager.GetPlayerGroup(pc.GetPlayerId());
	}

	protected TextWidget IRRU_FindGroupNameWidget()
	{
		array<ref Widget> widgets = {};
		SCR_WidgetHelper.GetAllChildren(m_wRoot, widgets);

		foreach (Widget widget : widgets)
		{
			TextWidget textWidget = TextWidget.Cast(widget);
			if (!textWidget)
				continue;

			string widgetName = widget.GetName();
			widgetName.ToLower();

			if (widgetName.Contains("group") && widgetName.Contains("name"))
				return textWidget;

			if (widgetName.Contains("squad") && widgetName.Contains("name"))
				return textWidget;
		}

		return null;
	}

	protected string IRRU_BuildGroupKey(SCR_AIGroup group)
	{
		if (!group)
			return "";

		return group.ToString();
	}

	protected bool IRRU_IsDefaultOrEmptyGroupName(string value)
	{
		value.Trim();
		if (value.IsEmpty())
			return true;

		string normalized = value;
		normalized.ToLower();

		if (normalized == "group" || normalized == "squad")
			return true;

		if (normalized.IndexOf("group ") == 0 || normalized.IndexOf("squad ") == 0)
			return true;

		// Localized placeholders are not manually assigned names.
		if (normalized.IndexOf("$") == 0)
			return true;

		return false;
	}
}
