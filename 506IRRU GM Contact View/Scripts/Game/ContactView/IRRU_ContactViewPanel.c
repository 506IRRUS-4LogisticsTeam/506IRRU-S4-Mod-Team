class IRRU_ContactViewGroupRowData
{
	int m_iGroupId;
	Widget m_wRow;
	TextWidget m_wExpandIcon;
	TextWidget m_wStatusIcon;
	TextWidget m_wGroupName;
	TextWidget m_wPlayerCount;
	TextWidget m_wTimeText;
	ref array<Widget> m_aPlayerRows;

	void IRRU_ContactViewGroupRowData()
	{
		m_aPlayerRows = new array<Widget>();
	}
}

//------------------------------------------------------------------------------------------------
class IRRU_ContactViewPanel : ScriptedWidgetEventHandler
{
	protected static const ResourceName GROUP_ROW_LAYOUT = "{F0B63F7142E79141}UI/Layouts/ContactViewGroupRow.layout";
	protected static const ResourceName PLAYER_ROW_LAYOUT = "{8B0CD8298F0077A1}UI/Layouts/ContactViewPlayerRow.layout";

	protected Widget m_wRoot;
	protected Widget m_wGroupList;
	protected Widget m_wNoDataLabel;
	protected TextWidget m_wHeaderStats;
	protected float m_fUpdateInterval = 1.0;
	protected float m_fTimeSinceUpdate = 0;
	protected bool m_bVisible = false;

	protected ref map<int, bool> m_mGroupExpandedState;
	protected ref map<int, ref IRRU_ContactViewGroupRowData> m_mGroupRows;

	protected static const int COLOR_GREEN_INT = 0xFF00FF00;
	protected static const int COLOR_YELLOW_INT = 0xFFFFFF00;
	protected static const int COLOR_RED_INT = 0xFFFF0000;
	protected static const int COLOR_GRAY_INT = 0xFF808080;

	//------------------------------------------------------------------------------------------------
	void IRRU_ContactViewPanel(Widget root)
	{
		m_wRoot = root;
		m_mGroupExpandedState = new map<int, bool>();
		m_mGroupRows = new map<int, ref IRRU_ContactViewGroupRowData>();

		FindWidgets();
		Hide();

		if (root)
			root.AddHandler(this);
	}

	//------------------------------------------------------------------------------------------------
	protected void FindWidgets()
	{
		if (!m_wRoot)
			return;

		m_wGroupList = m_wRoot.FindAnyWidget("ContactViewList");
		m_wNoDataLabel = m_wRoot.FindAnyWidget("NoDataLabel");
		m_wHeaderStats = TextWidget.Cast(m_wRoot.FindAnyWidget("HeaderStats"));
	}

	//------------------------------------------------------------------------------------------------
	void Update(float timeSlice)
	{
		if (!m_bVisible)
			return;

		m_fTimeSinceUpdate += timeSlice;
		if (m_fTimeSinceUpdate < m_fUpdateInterval)
			return;

		m_fTimeSinceUpdate = 0;
		UpdateGroupList();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateGroupList()
	{
		array<ref IRRU_ContactViewGroupData> groupData = new array<ref IRRU_ContactViewGroupData>();
		IRRU_ContactViewManager.GetInstance().GetGroupContactData(groupData);

		if (groupData.IsEmpty())
		{
			ClearAllRows();
			if (m_wNoDataLabel)
				m_wNoDataLabel.SetVisible(true);
			return;
		}

		if (m_wNoDataLabel)
			m_wNoDataLabel.SetVisible(false);

		int totalCritical, totalWarning, totalGreen;
		IRRU_ContactViewManager.GetInstance().GetTotalStatusCounts(totalCritical, totalWarning, totalGreen);

		if (m_wHeaderStats)
			m_wHeaderStats.SetText(string.Format("%1 Crit | %2 Warn", totalCritical, totalWarning));

		set<int> validGroupIds = new set<int>();

		int zOrder = 0;
		foreach (IRRU_ContactViewGroupData group : groupData)
		{
			int groupId = group.GetGroupId();
			validGroupIds.Insert(groupId);

			IRRU_ContactViewGroupRowData rowData;
			if (m_mGroupRows.Contains(groupId))
			{
				rowData = m_mGroupRows.Get(groupId);
			}
			else
			{
				rowData = CreateGroupRow(groupId);
				if (!rowData)
					continue;
				m_mGroupRows.Set(groupId, rowData);
			}

			UpdateGroupRowContent(rowData, group);

			if (rowData.m_wRow)
				rowData.m_wRow.SetZOrder(zOrder);
			zOrder++;

			bool isExpanded = IsGroupExpanded(groupId);
			UpdatePlayerRows(rowData, group, isExpanded, zOrder);
			if (isExpanded)
				zOrder += group.GetPlayerCount();
		}

		array<int> groupsToRemove = new array<int>();
		for (int i = 0; i < m_mGroupRows.Count(); i++)
		{
			int groupId = m_mGroupRows.GetKey(i);
			if (!validGroupIds.Contains(groupId))
				groupsToRemove.Insert(groupId);
		}

		foreach (int removeId : groupsToRemove)
		{
			RemoveGroupRow(removeId);
		}

		if (IRRU_ContactViewSettings.IsDebugEnabled())
		{
			PrintDebugStatus(groupData, totalCritical, totalWarning, totalGreen);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected IRRU_ContactViewGroupRowData CreateGroupRow(int groupId)
	{
		if (!m_wGroupList)
			return null;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return null;

		Widget rowWidget = workspace.CreateWidgets(GROUP_ROW_LAYOUT, m_wGroupList);
		if (!rowWidget)
		{
			Print(string.Format("[ContactView] Failed to create group row widget for group %1, using fallback", groupId));
			return CreateFallbackGroupRow(groupId);
		}

		IRRU_ContactViewGroupRowData rowData = new IRRU_ContactViewGroupRowData();
		rowData.m_iGroupId = groupId;
		rowData.m_wRow = rowWidget;

		rowData.m_wExpandIcon = TextWidget.Cast(rowWidget.FindAnyWidget("ExpandIcon"));
		rowData.m_wStatusIcon = TextWidget.Cast(rowWidget.FindAnyWidget("StatusIcon"));
		rowData.m_wGroupName = TextWidget.Cast(rowWidget.FindAnyWidget("GroupName"));
		rowData.m_wPlayerCount = TextWidget.Cast(rowWidget.FindAnyWidget("PlayerCount"));
		rowData.m_wTimeText = TextWidget.Cast(rowWidget.FindAnyWidget("TimeText"));

		SCR_ButtonBaseComponent buttonComp = SCR_ButtonBaseComponent.Cast(rowWidget.FindHandler(SCR_ButtonBaseComponent));
		if (buttonComp)
		{
			buttonComp.m_OnClicked.Insert(OnGroupRowClicked);
		}

		return rowData;
	}

	//------------------------------------------------------------------------------------------------
	protected IRRU_ContactViewGroupRowData CreateFallbackGroupRow(int groupId)
	{
		if (!m_wGroupList)
			return null;

		IRRU_ContactViewGroupRowData rowData = new IRRU_ContactViewGroupRowData();
		rowData.m_iGroupId = groupId;

		return rowData;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateGroupRowContent(IRRU_ContactViewGroupRowData rowData, IRRU_ContactViewGroupData group)
	{
		if (!rowData)
			return;

		float avgTime = group.GetAverageTimeSinceContact();
		bool isExpanded = IsGroupExpanded(group.GetGroupId());

		if (rowData.m_wExpandIcon)
		{
			if (isExpanded)
				rowData.m_wExpandIcon.SetText("v");
			else
				rowData.m_wExpandIcon.SetText(">");
		}

		if (rowData.m_wStatusIcon)
		{
			rowData.m_wStatusIcon.SetText(GetStatusIcon(avgTime));
			rowData.m_wStatusIcon.SetColorInt(GetStatusColorInt(avgTime));
		}

		if (rowData.m_wGroupName)
			rowData.m_wGroupName.SetText(group.GetGroupName());

		if (rowData.m_wPlayerCount)
			rowData.m_wPlayerCount.SetText(string.Format("(%1)", group.GetPlayerCount()));

		if (rowData.m_wTimeText)
		{
			rowData.m_wTimeText.SetText(IRRU_ContactViewManager.FormatTimeSinceContact(avgTime));
			rowData.m_wTimeText.SetColorInt(GetStatusColorInt(avgTime));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayerRows(IRRU_ContactViewGroupRowData rowData, IRRU_ContactViewGroupData group, bool isExpanded, int startZOrder)
	{
		if (!rowData || !m_wGroupList)
			return;

		foreach (Widget playerRow : rowData.m_aPlayerRows)
		{
			if (playerRow)
				playerRow.RemoveFromHierarchy();
		}
		rowData.m_aPlayerRows.Clear();

		if (!isExpanded)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		array<int> playerIds = group.GetPlayerIds();
		int zOrder = startZOrder;

		foreach (int playerId : playerIds)
		{
			Widget playerRow = workspace.CreateWidgets(PLAYER_ROW_LAYOUT, m_wGroupList);
			if (!playerRow)
			{
				continue;
			}

			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (playerName.IsEmpty())
				playerName = string.Format("Player %1", playerId);

			float timeSinceContact = IRRU_ContactViewManager.GetInstance().GetTimeSinceContact(playerId);

			TextWidget statusIcon = TextWidget.Cast(playerRow.FindAnyWidget("StatusIcon"));
			TextWidget nameText = TextWidget.Cast(playerRow.FindAnyWidget("PlayerName"));
			TextWidget timeText = TextWidget.Cast(playerRow.FindAnyWidget("TimeText"));

			if (statusIcon)
			{
				statusIcon.SetText(GetStatusIcon(timeSinceContact));
				statusIcon.SetColorInt(GetStatusColorInt(timeSinceContact));
			}

			if (nameText)
				nameText.SetText(playerName);

			if (timeText)
			{
				timeText.SetText(IRRU_ContactViewManager.FormatTimeSinceContact(timeSinceContact));
				timeText.SetColorInt(GetStatusColorInt(timeSinceContact));
			}

			playerRow.SetZOrder(zOrder);
			zOrder++;

			rowData.m_aPlayerRows.Insert(playerRow);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveGroupRow(int groupId)
	{
		if (!m_mGroupRows.Contains(groupId))
			return;

		IRRU_ContactViewGroupRowData rowData = m_mGroupRows.Get(groupId);
		if (rowData)
		{
			foreach (Widget playerRow : rowData.m_aPlayerRows)
			{
				if (playerRow)
					playerRow.RemoveFromHierarchy();
			}

			if (rowData.m_wRow)
				rowData.m_wRow.RemoveFromHierarchy();
		}

		m_mGroupRows.Remove(groupId);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearAllRows()
	{
		array<int> allGroupIds = new array<int>();
		for (int i = 0; i < m_mGroupRows.Count(); i++)
		{
			allGroupIds.Insert(m_mGroupRows.GetKey(i));
		}

		foreach (int groupId : allGroupIds)
		{
			RemoveGroupRow(groupId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGroupRowClicked(SCR_ButtonBaseComponent button)
	{
		if (!button)
			return;

		Widget rowWidget = button.GetRootWidget();
		if (!rowWidget)
			return;

		for (int i = 0; i < m_mGroupRows.Count(); i++)
		{
			IRRU_ContactViewGroupRowData rowData = m_mGroupRows.GetElement(i);
			if (rowData && rowData.m_wRow == rowWidget)
			{
				ToggleGroupExpanded(rowData.m_iGroupId);
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected string GetStatusIcon(float timeSinceContact)
	{
		float warningThreshold = IRRU_ContactViewSettings.GetWarningThreshold();
		float criticalThreshold = IRRU_ContactViewSettings.GetCriticalThreshold();

		if (timeSinceContact < 0)
			return "?";

		if (timeSinceContact < warningThreshold)
			return "OK";

		if (timeSinceContact < criticalThreshold)
			return "!!";

		return "XX";
	}

	//------------------------------------------------------------------------------------------------
	protected int GetStatusColorInt(float timeSinceContact)
	{
		float warningThreshold = IRRU_ContactViewSettings.GetWarningThreshold();
		float criticalThreshold = IRRU_ContactViewSettings.GetCriticalThreshold();

		if (timeSinceContact < 0)
			return COLOR_GRAY_INT;

		if (timeSinceContact < warningThreshold)
			return COLOR_GREEN_INT;

		if (timeSinceContact < criticalThreshold)
			return COLOR_YELLOW_INT;

		return COLOR_RED_INT;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsGroupExpanded(int groupId)
	{
		if (m_mGroupExpandedState.Contains(groupId))
			return m_mGroupExpandedState.Get(groupId);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	void ToggleGroupExpanded(int groupId)
	{
		bool currentState = IsGroupExpanded(groupId);
		m_mGroupExpandedState.Set(groupId, !currentState);

		m_fTimeSinceUpdate = m_fUpdateInterval;
	}

	//------------------------------------------------------------------------------------------------
	void ExpandAll()
	{
		array<ref IRRU_ContactViewGroupData> groupData = new array<ref IRRU_ContactViewGroupData>();
		IRRU_ContactViewManager.GetInstance().GetGroupContactData(groupData);

		foreach (IRRU_ContactViewGroupData group : groupData)
		{
			m_mGroupExpandedState.Set(group.GetGroupId(), true);
		}

		m_fTimeSinceUpdate = m_fUpdateInterval;
	}

	//------------------------------------------------------------------------------------------------
	void CollapseAll()
	{
		m_mGroupExpandedState.Clear();
		m_fTimeSinceUpdate = m_fUpdateInterval;
	}

	//------------------------------------------------------------------------------------------------
	protected void PrintDebugStatus(array<ref IRRU_ContactViewGroupData> groupData, int totalCritical, int totalWarning, int totalGreen)
	{
		Print("[ContactView] ========== GROUP STATUS ==========");
		Print(string.Format("[ContactView] Total: %1 Critical | %2 Warning | %3 OK", totalCritical, totalWarning, totalGreen));
		Print("[ContactView] ----------------------------------");

		foreach (IRRU_ContactViewGroupData group : groupData)
		{
			string statusIcon = GetStatusIcon(group.GetAverageTimeSinceContact());
			string timeStr = IRRU_ContactViewManager.FormatTimeSinceContact(group.GetAverageTimeSinceContact());
			bool isExpanded = IsGroupExpanded(group.GetGroupId());
			string expandIcon;
			if (isExpanded)
				expandIcon = "v";
			else
				expandIcon = ">";

			Print(string.Format("[ContactView] %1 %2 %3 (%4 players) - Avg: %5",
				expandIcon,
				statusIcon,
				group.GetGroupName(),
				group.GetPlayerCount(),
				timeStr));

			if (isExpanded)
			{
				array<int> playerIds = group.GetPlayerIds();
				foreach (int playerId : playerIds)
				{
					string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
					if (playerName.IsEmpty())
						playerName = string.Format("Player %1", playerId);

					float timeSinceContact = IRRU_ContactViewManager.GetInstance().GetTimeSinceContact(playerId);
					string playerStatusIcon = GetStatusIcon(timeSinceContact);
					string playerTimeStr = IRRU_ContactViewManager.FormatTimeSinceContact(timeSinceContact);

					Print(string.Format("[ContactView]    %1 %2 - %3", playerStatusIcon, playerName, playerTimeStr));
				}
			}
		}
		Print("[ContactView] ==================================");
	}

	//------------------------------------------------------------------------------------------------
	protected string GetContactTypeString(IRRU_EContactType type)
	{
		switch (type)
		{
			case IRRU_EContactType.FIRED:
				return "Fired";
			case IRRU_EContactType.DAMAGED:
				return "Damaged";
			case IRRU_EContactType.NONE:
				return "None";
		}
		return "Unknown";
	}

	//------------------------------------------------------------------------------------------------
	void Show()
	{
		if (m_wRoot)
		{
			m_wRoot.SetVisible(true);
			m_bVisible = true;
			UpdateGroupList();
		}
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		if (m_wRoot)
		{
			m_wRoot.SetVisible(false);
			m_bVisible = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	void Toggle()
	{
		if (m_bVisible)
			Hide();
		else
			Show();
	}

	//------------------------------------------------------------------------------------------------
	bool IsVisible()
	{
		return m_bVisible;
	}
}
