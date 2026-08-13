//------------------------------------------------------------------------------------------------
modded class SCR_ChatPanel
{
	//------------------------------------------------------------------------------------------------
	override void OnUpdateChat(float timeSlice)
	{
		super.OnUpdateChat(timeSlice);
		IRRU_UpdateChatNamesDisplay();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void IRRU_UpdateChatNamesDisplay()
	{
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager || !m_wRoot)
			return;
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;
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
			array<int> players = {};
			playerManager.GetPlayers(players);
			bool textModified = false;
			string modifiedText = currentText;
			
			foreach (int playerId : players)
			{
				string originalName = playerManager.GetPlayerName(playerId);
				if (!originalName.IsEmpty() && modifiedText.Contains(originalName))
				{
					string customName = manager.GetCustomName(playerId);
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