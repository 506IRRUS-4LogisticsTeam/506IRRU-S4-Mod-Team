//------------------------------------------------------------------------------------------------
modded class SCR_NotificationOnKillfeedChanged : SCR_NotificationPlayer
{
	//------------------------------------------------------------------------------------------------
	override protected bool GetPlayerName(int playerID, out string playerName)
	{
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			string customName = manager.GetCustomName(playerID);
			if (!customName.IsEmpty())
			{
				playerName = customName;
				return true;
			}
		}
		return super.GetPlayerName(playerID, playerName);
	}
}