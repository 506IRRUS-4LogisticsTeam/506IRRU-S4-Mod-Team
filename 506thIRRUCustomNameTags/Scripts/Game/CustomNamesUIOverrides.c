//! CUSTOM NAMES UI OVERRIDES - Comprehensive name display overrides for all UI elements
//! This file contains all UI component overrides to display custom names

//------------------------------------------------------------------------------------------------
// Modded SCR_PlayerListMenu - Override scoreboard to show custom names
//------------------------------------------------------------------------------------------------
modded class SCR_PlayerListMenu
{
	//------------------------------------------------------------------------------------------------
	//! Override OnMenuOpen to inject custom names when menu opens
	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		// After menu opens, update all player names with custom ones
		UpdateCustomNames();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Override OnMenuUpdate to keep names updated
	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		// Periodically update custom names
		UpdateCustomNames();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Helper to update all displayed names with custom ones
	//------------------------------------------------------------------------------------------------
	protected void UpdateCustomNames()
	{
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
		{
			Print("[CustomNames][SCOREBOARD] Manager not available for scoreboard update", LogLevel.WARNING);
			return;
		}
		
		int updateCount = 0;
		
		// Update all player entry names
		foreach (SCR_PlayerListEntry entry : m_aEntries)
		{
			if (!entry || !entry.m_wName)
				continue;
			
			string playerIdStr = entry.m_iID.ToString();
			string customName = manager.GetCustomName(playerIdStr);
			if (!customName.IsEmpty())
			{
				entry.m_wName.SetText(customName);
				updateCount++;
				Print(string.Format("[CustomNames][SCOREBOARD] Updated player %1 to '%2'", playerIdStr, customName), LogLevel.NORMAL);
			}
		}
		
		if (updateCount > 0)
		{
			Print(string.Format("[CustomNames][SCOREBOARD] Updated %1 entries on scoreboard", updateCount), LogLevel.NORMAL);
		}
	}
}

//------------------------------------------------------------------------------------------------
// Modded SCR_VonDisplay - Override VON display to show custom names
//------------------------------------------------------------------------------------------------
modded class SCR_VonDisplay
{
	//------------------------------------------------------------------------------------------------
	//! Override DisplayUpdate to inject custom names in VON display
	//------------------------------------------------------------------------------------------------
	override void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		
		// After parent updates, check for custom names
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager || !m_wRoot)
			return;
		
		// Find and update VON speaker name widgets
		Widget speakerWidget = m_wRoot.FindAnyWidget("SpeakerName");
		if (speakerWidget)
		{
			TextWidget nameText = TextWidget.Cast(speakerWidget);
			if (nameText)
			{
				string currentText = nameText.GetText();
				PlayerManager playerManager = GetGame().GetPlayerManager();
				if (playerManager)
				{
					array<int> players = {};
					playerManager.GetPlayers(players);
					foreach (int playerId : players)
					{
						string originalName = playerManager.GetPlayerName(playerId);
						if (currentText == originalName)
						{
							string playerIdStr = playerId.ToString();
							string customName = manager.GetCustomName(playerIdStr);
							if (!customName.IsEmpty())
							{
								nameText.SetText(customName);
								break;
							}
						}
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
// Modded SCR_HUDGroupUIComponent - Override squad display to show custom names
//------------------------------------------------------------------------------------------------
modded class SCR_HUDGroupUIComponent
{
	//------------------------------------------------------------------------------------------------
	//! Override HandlerAttached to inject custom names when UI is created
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Schedule name updates after UI is fully initialized
		GetGame().GetCallqueue().CallLater(UpdateGroupMemberNames, 100, true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Helper to update group member names with custom ones
	//------------------------------------------------------------------------------------------------
	protected void UpdateGroupMemberNames()
	{
		if (!m_wRoot)
			return;
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
			return;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;
		
		// Find all group member name widgets and update them
		array<ref Widget> memberWidgets = {};
		SCR_WidgetHelper.GetAllChildren(m_wRoot, memberWidgets);
		
		foreach (Widget w : memberWidgets)
		{
			TextWidget nameWidget = TextWidget.Cast(w);
			if (!nameWidget)
				continue;
			
			string widgetName = w.GetName();
			if (!widgetName.Contains("Name") && !widgetName.Contains("Player"))
				continue;
			
			string currentText = nameWidget.GetText();
			if (currentText.IsEmpty())
				continue;
			
			// Check if this matches any player name
			array<int> players = {};
			playerManager.GetPlayers(players);
			foreach (int playerId : players)
			{
				string originalName = playerManager.GetPlayerName(playerId);
				if (currentText == originalName)
				{
					string playerIdStr = playerId.ToString();
					string customName = manager.GetCustomName(playerIdStr);
					if (!customName.IsEmpty())
					{
						nameWidget.SetText(customName);
						break;
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
// Helper class to provide centralized name resolution
//------------------------------------------------------------------------------------------------
class IRRU_PlayerNameHelper
{
	//------------------------------------------------------------------------------------------------
	//! Get display name for a player (checks custom names first)
	//------------------------------------------------------------------------------------------------
	static string GetPlayerDisplayName(int playerId)
	{
		Print(string.Format("[CustomNames][HELPER] GetPlayerDisplayName called for player %1", playerId), LogLevel.NORMAL);
		
		// First check for custom name
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			string playerIdStr = playerId.ToString();
			string customName = manager.GetCustomName(playerIdStr);
			
			Print(string.Format("[CustomNames][HELPER] Custom name lookup: '%1'", customName), LogLevel.NORMAL);
			
			if (!customName.IsEmpty())
			{
				Print(string.Format("[CustomNames][HELPER] Returning custom name: '%1'", customName), LogLevel.NORMAL);
				return customName;
			}
		}
		else
		{
			Print("[CustomNames][HELPER] Manager not available", LogLevel.WARNING);
		}
		
		// Fall back to platform name
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			string platformName = playerManager.GetPlayerName(playerId);
			Print(string.Format("[CustomNames][HELPER] Returning platform name: '%1'", platformName), LogLevel.NORMAL);
			return platformName;
		}
		
		Print("[CustomNames][HELPER] No name found, returning empty", LogLevel.WARNING);
		return "";
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if player has a custom name set
	//------------------------------------------------------------------------------------------------
	static bool HasCustomName(int playerId)
	{
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
			return false;
		
		string playerIdStr = playerId.ToString();
		string customName = manager.GetCustomName(playerIdStr);
		return !customName.IsEmpty();
	}
}