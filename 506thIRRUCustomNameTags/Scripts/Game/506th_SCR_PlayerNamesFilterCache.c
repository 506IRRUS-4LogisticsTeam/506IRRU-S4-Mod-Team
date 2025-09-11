//------------------------------------------------------------------------------------------------
modded class SCR_PlayerNamesFilterCache
{
	//------------------------------------------------------------------------------------------------
	override string GetPlayerDisplayName(int playerId)
	{
		if (playerId <= 0)
		{
			Print(string.Format("[CustomNames] Invalid player ID %1 for display name", playerId), LogLevel.WARNING);
			return super.GetPlayerDisplayName(playerId);
		}
		
		string originalName = super.GetPlayerDisplayName(playerId);
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			string customName = manager.GetCustomName(playerId);
			
			if (!customName.IsEmpty())
			{
				return customName;
			}
		}
		else
		{
			Print("[CustomNames] Filter manager not available", LogLevel.WARNING);
		}
		
		return originalName;
	}
}
