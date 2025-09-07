//------------------------------------------------------------------------------------------------
modded class SCR_NameTagData : Managed
{
	//------------------------------------------------------------------------------------------------
	override void GetName(out string name, out notnull array<string> nameParams)
	{
		super.GetName(name, nameParams);
		if (m_eType == ENameTagEntityType.PLAYER)
		{
			if (m_iPlayerID <= 0)
			{
				Print(string.Format("[CustomNames] Invalid player ID %1 for nametag", m_iPlayerID), LogLevel.WARNING);
				return;
			}
			
			CustomNamesManager manager = CustomNamesManager.GetInstance();
			if (manager)
			{
				string customName = manager.GetCustomName(m_iPlayerID);
				
				if (!customName.IsEmpty())
				{
					name = customName;
					nameParams.Clear();
					m_sName = customName;
				}
			}
			else
			{
				Print("[CustomNames] NameTag manager not available", LogLevel.WARNING);
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
