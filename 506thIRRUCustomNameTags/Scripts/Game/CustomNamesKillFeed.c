//------------------------------------------------------------------------------------------------
modded class SCR_NotificationOnKillfeedChanged : SCR_NotificationPlayer
{
	//------------------------------------------------------------------------------------------------
	override protected bool GetPlayerName(int playerID, out string playerName)
	{
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
		return super.GetPlayerName(playerID, playerName);
	}
}