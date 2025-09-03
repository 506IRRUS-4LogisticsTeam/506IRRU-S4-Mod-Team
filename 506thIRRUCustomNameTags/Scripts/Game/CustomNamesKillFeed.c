//! CUSTOM NAMES KILL FEED - Override kill feed to show custom names
//! This intercepts kill notifications to display custom names

//------------------------------------------------------------------------------------------------
// Modded SCR_NotificationOnKillfeedChanged - Override kill feed notifications
//------------------------------------------------------------------------------------------------
modded class SCR_NotificationOnKillfeedChanged : SCR_NotificationPlayer
{
	//------------------------------------------------------------------------------------------------
	//! Override GetPlayerName to return custom names
	//------------------------------------------------------------------------------------------------
	override protected bool GetPlayerName(int playerID, out string playerName)
	{
		// First try to get custom name
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			string playerIdStr = playerID.ToString();
			string customName = manager.GetCustomName(playerIdStr);
			if (!customName.IsEmpty())
			{
				playerName = customName;
				return true;
			}
		}
		
		// Fall back to original implementation
		return super.GetPlayerName(playerID, playerName);
	}
}