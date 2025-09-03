//! CUSTOM NAMES CHAT PANEL - Override chat display to show custom names
//! This intercepts the actual chat rendering to display custom names

//------------------------------------------------------------------------------------------------
// Modded SCR_ChatPanel - Override message display
//------------------------------------------------------------------------------------------------
modded class SCR_ChatPanel
{
	//------------------------------------------------------------------------------------------------
	//! Override OnUpdateChat to inject custom names periodically
	//------------------------------------------------------------------------------------------------
	override void OnUpdateChat(float timeSlice)
	{
		super.OnUpdateChat(timeSlice);
		
		// Periodically update displayed names with custom ones
		UpdateChatNamesDisplay();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Helper to update chat message display with custom names
	//------------------------------------------------------------------------------------------------
	protected void UpdateChatNamesDisplay()
	{
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager || !m_wRoot)
			return;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;
		
		// Find all message widgets and update player names
		array<ref Widget> messageWidgets = {};
		SCR_WidgetHelper.GetAllChildren(m_wRoot, messageWidgets);
		
		foreach (Widget w : messageWidgets)
		{
			TextWidget textWidget = TextWidget.Cast(w);
			if (!textWidget)
				continue;
			
			string currentText = textWidget.GetText();
			if (currentText.IsEmpty())
				continue;
			
			// Check if text contains any player names and replace with custom names
			array<int> players = {};
			playerManager.GetPlayers(players);
			bool textModified = false;
			string modifiedText = currentText;
			
			foreach (int playerId : players)
			{
				string originalName = playerManager.GetPlayerName(playerId);
				if (!originalName.IsEmpty() && modifiedText.Contains(originalName))
				{
					string playerIdStr = playerId.ToString();
					string customName = manager.GetCustomName(playerIdStr);
					if (!customName.IsEmpty())
					{
						modifiedText.Replace(originalName, customName);
						textModified = true;
					}
				}
			}
			
			if (textModified)
			{
				textWidget.SetText(modifiedText);
			}
		}
	}
}