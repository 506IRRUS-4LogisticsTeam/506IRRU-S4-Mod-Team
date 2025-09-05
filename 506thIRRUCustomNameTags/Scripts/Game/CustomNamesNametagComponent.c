//------------------------------------------------------------------------------------------------
modded class SCR_NameTagData : Managed
{
	//------------------------------------------------------------------------------------------------
	override void GetName(out string name, out notnull array<string> nameParams)
	{
		super.GetName(name, nameParams);
		if (m_eType == ENameTagEntityType.PLAYER && m_iPlayerID > 0)
		{
			Print(string.Format("[CustomNames][NAMETAG] GetName called for player %1", m_iPlayerID), LogLevel.NORMAL);
			Print(string.Format("[CustomNames][NAMETAG] Original name: '%1'", name), LogLevel.NORMAL);
			
			CustomNamesManager manager = CustomNamesManager.GetInstance();
			if (manager)
			{
				string playerIdStr = m_iPlayerID.ToString();
				string customName = manager.GetCustomName(playerIdStr);
				
				Print(string.Format("[CustomNames][NAMETAG] Custom name lookup result: '%1'", customName), LogLevel.NORMAL);
				
				if (!customName.IsEmpty())
				{
					name = customName;
					nameParams.Clear();
					m_sName = customName;
					
					Print(string.Format("[CustomNames][NAMETAG] Applied custom name: '%1'", customName), LogLevel.NORMAL);
				}
				else
				{
					Print(string.Format("[CustomNames][NAMETAG] No custom name, using default"), LogLevel.NORMAL);
				}
			}
			else
			{
				Print(string.Format("[CustomNames][NAMETAG] Manager not available"), LogLevel.WARNING);
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
		Print(string.Format("[CustomNames][FILTER] GetPlayerDisplayName called for player %1", playerId), LogLevel.NORMAL);
		
		string originalName = super.GetPlayerDisplayName(playerId);
		Print(string.Format("[CustomNames][FILTER] Original display name: '%1'", originalName), LogLevel.NORMAL);
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			string playerIdStr = playerId.ToString();
			string customName = manager.GetCustomName(playerIdStr);
			
			Print(string.Format("[CustomNames][FILTER] Custom name lookup: '%1'", customName), LogLevel.NORMAL);
			
			if (!customName.IsEmpty())
			{
				Print(string.Format("[CustomNames][FILTER] Returning custom name: '%1'", customName), LogLevel.NORMAL);
				return customName;
			}
			else
			{
				Print(string.Format("[CustomNames][FILTER] No custom name, using original"), LogLevel.NORMAL);
			}
		}
		else
		{
			Print(string.Format("[CustomNames][FILTER] Manager not available, using original"), LogLevel.WARNING);
		}
		
		return originalName;
	}
}
