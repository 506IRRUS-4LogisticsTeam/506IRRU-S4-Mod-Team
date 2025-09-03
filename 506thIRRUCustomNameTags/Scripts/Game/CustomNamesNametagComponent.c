//! CUSTOM NAMES NAMETAG COMPONENT - Player Name Override for 3D Nametags
//! FIXED VERSION - Properly overrides name display in nametags

//------------------------------------------------------------------------------------------------
// Modded SCR_NameTagData - Override GetName method properly
//------------------------------------------------------------------------------------------------
modded class SCR_NameTagData : Managed
{
	protected const string LOG_PREFIX_CUSTOM_NAMES_NAMETAG = "[CustomNames][Nametag]";
	
	//------------------------------------------------------------------------------------------------
	//! Override GetName to return custom names for nametags
	//------------------------------------------------------------------------------------------------
	override void GetName(out string name, out notnull array<string> nameParams)
	{
		// Call parent first to get the original name
		super.GetName(name, nameParams);
		
		// Then check if we have a custom name override
		if (m_eType == ENameTagEntityType.PLAYER && m_iPlayerID > 0)
		{
			CustomNamesManager manager = CustomNamesManager.GetInstance();
			if (manager)
			{
				string playerIdStr = m_iPlayerID.ToString();
				string customName = manager.GetCustomName(playerIdStr);
				if (!customName.IsEmpty())
				{
					// Override with custom name
					name = customName;
					nameParams.Clear(); // Clear any formatting params
					m_sName = customName; // Update internal cache
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
// Modded SCR_PlayerNamesFilterCache - Intercept at the cache level
//------------------------------------------------------------------------------------------------
modded class SCR_PlayerNamesFilterCache
{
	//------------------------------------------------------------------------------------------------
	//! Override GetPlayerDisplayName to return custom names
	//------------------------------------------------------------------------------------------------
	override string GetPlayerDisplayName(int playerId)
	{
		// First check for custom name
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			string playerIdStr = playerId.ToString();
			string customName = manager.GetCustomName(playerIdStr);
			if (!customName.IsEmpty())
				return customName;
		}
		
		// Fall back to original implementation
		return super.GetPlayerDisplayName(playerId);
	}
}
