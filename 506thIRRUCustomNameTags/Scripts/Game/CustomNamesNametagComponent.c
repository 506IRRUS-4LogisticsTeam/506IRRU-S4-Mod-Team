//------------------------------------------------------------------------------------------------
modded class SCR_NameTagData : Managed
{
	//------------------------------------------------------------------------------------------------
	override void GetName(out string name, out notnull array<string> nameParams)
	{
		Print(string.Format("[CustomNames-NameTag] GetName() called - EntityType: %1, PlayerID: %2", m_eType, m_iPlayerID), LogLevel.NORMAL);
		
		// For non-player entities, use default behavior
		if (m_eType != ENameTagEntityType.PLAYER)
		{
			Print("[CustomNames-NameTag] Non-player entity, using default behavior", LogLevel.NORMAL);
			super.GetName(name, nameParams);
			return;
		}
		
		// For player entities, check for custom name first
		if (m_iPlayerID <= 0)
		{
			Print(string.Format("[CustomNames-NameTag] Invalid player ID %1, using default behavior", m_iPlayerID), LogLevel.WARNING);
			super.GetName(name, nameParams);
			return;
		}
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
		{
			Print("[CustomNames-NameTag] Manager not available, using default behavior", LogLevel.WARNING);
			super.GetName(name, nameParams);
			return;
		}
		
		string customName = manager.GetCustomName(m_iPlayerID);
		Print(string.Format("[CustomNames-NameTag] Retrieved custom name for player %1: '%2'", m_iPlayerID, customName), LogLevel.NORMAL);
		
		if (!customName.IsEmpty())
		{
			// Set the custom name
			m_sName = customName;
			name = customName;
			nameParams.Clear();
			Print(string.Format("[CustomNames-NameTag] Applied custom name '%1' to nametag (m_sName and output)", customName), LogLevel.NORMAL);
			return;
		}
		
		// Fall back to default behavior if no custom name
		Print("[CustomNames-NameTag] No custom name found, using default behavior", LogLevel.NORMAL);
		super.GetName(name, nameParams);
		
		// Log what the default behavior returned
		Print(string.Format("[CustomNames-NameTag] Default name returned: '%1'", name), LogLevel.NORMAL);
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
