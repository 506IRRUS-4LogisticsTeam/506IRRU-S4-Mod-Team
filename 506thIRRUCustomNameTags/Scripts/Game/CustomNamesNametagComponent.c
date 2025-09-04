//------------------------------------------------------------------------------------------------
modded class SCR_NameTagData : Managed
{
	//------------------------------------------------------------------------------------------------
	override void GetName(out string name, out notnull array<string> nameParams)
	{
		super.GetName(name, nameParams);
		if (m_eType == ENameTagEntityType.PLAYER && m_iPlayerID > 0)
		{
			CustomNamesManager manager = CustomNamesManager.GetInstance();
			if (manager)
			{
				string playerIdStr = m_iPlayerID.ToString();
				string customName = manager.GetCustomName(playerIdStr);
				if (!customName.IsEmpty())
				{
					name = customName;
					nameParams.Clear();
					m_sName = customName;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerNamesFilterCache
{
	//------------------------------------------------------------------------------------------------
	override string GetPlayerDisplayName(int playerId)
	{
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			string playerIdStr = playerId.ToString();
			string customName = manager.GetCustomName(playerIdStr);
			if (!customName.IsEmpty())
				return customName;
		}
		return super.GetPlayerDisplayName(playerId);
	}
}
