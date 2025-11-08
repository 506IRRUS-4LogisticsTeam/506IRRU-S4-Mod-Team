//------------------------------------------------------------------------------------------------
//! Override CSI's UIHelper to inject custom names before rank is added
//------------------------------------------------------------------------------------------------
modded class CSI_UIHelper
{
	//------------------------------------------------------------------------------------------------
	//! Get players name with custom name support, then add rank if enabled
	override static string GetPlayersName(int playerID)
	{
		string name;

		// First try to get custom name from our manager
		CustomNamesManager customNamesMgr = CustomNamesManager.GetInstance();
		if (customNamesMgr)
		{
			string customName = customNamesMgr.GetCustomName(playerID);
			if (!customName.IsEmpty())
			{
				name = customName;
			}
			else
			{
				name = GetGame().GetPlayerManager().GetPlayerName(playerID);
			}
		}
		else
		{
			name = GetGame().GetPlayerManager().GetPlayerName(playerID);
		}

		// Now add rank prefix if CSI settings have rank visible
		CSI_PlayerData playerData = CSI_PlayerDataManager.GetInstance().GetPlayerData(playerID);

		if (!playerData || !CSI_SettingsManager.GetInstance().GetSettingBool(CSI_GameSettings.RANK_VISIBLE))
			return name;

		SCR_ECharacterRank rankEnum = playerData.GetRank();
		SCR_Faction faction = SCR_Faction.Cast(SCR_FactionManager.SGetPlayerFaction(playerID));

		if (!faction)
			return name;

		string rank = faction.GetRankName(rankEnum);

		if (rank.IsEmpty())
			return name;

		return string.Format("%1 %2", rank, name);
	}
}