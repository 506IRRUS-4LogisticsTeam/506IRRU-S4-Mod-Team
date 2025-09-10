//------------------------------------------------------------------------------------------------
modded class SCR_NameTagData : Managed
{
	//------------------------------------------------------------------------------------------------
	override void GetName(out string name, out notnull array<string> nameParams)
	{
		// For player entities with custom names, we completely take over
		if (m_eType == ENameTagEntityType.PLAYER && m_iPlayerID > 0)
		{
			CustomNamesManager manager = CustomNamesManager.GetInstance();
			if (manager)
			{
				string customName = manager.GetCustomName(m_iPlayerID);
				if (!customName.IsEmpty())
				{
					// AGGRESSIVE OVERRIDE - We win, CSI loses
					m_sName = customName;
					name = customName;
					nameParams.Clear();
					m_aNameParams.Clear();
					
					Print(string.Format("[CustomNames] OVERRIDE: Player %1 -> '%2' (CSI formatting ignored)", m_iPlayerID, customName), LogLevel.NORMAL);
					return;  // Don't call super - we're done here
				}
			}
		}
		
		// Only call super if we don't have a custom name
		super.GetName(name, nameParams);
	}
	
	//------------------------------------------------------------------------------------------------
	// Force our name to stick by also overriding any getter
	string GetName()
	{
		if (m_eType == ENameTagEntityType.PLAYER && m_iPlayerID > 0)
		{
			CustomNamesManager manager = CustomNamesManager.GetInstance();
			if (manager)
			{
				string customName = manager.GetCustomName(m_iPlayerID);
				if (!customName.IsEmpty())
				{
					m_sName = customName;
					return customName;
				}
			}
		}
		return m_sName;
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
