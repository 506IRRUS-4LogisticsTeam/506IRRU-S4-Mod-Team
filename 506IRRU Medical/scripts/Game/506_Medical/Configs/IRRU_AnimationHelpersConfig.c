//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class IRRU_AnimationHelpersConfig
{
	[Attribute(desc: "Registered animation helpers")]
	protected ref array<ref IRRU_AnimationHelperConfigEntry> m_aEntries;
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPrefabName(IRRU_EAnimationHelperID id)
	{
		Print(string.Format("[DEBUG][AnimConfig] GetPrefabName called for ID: %1", id), LogLevel.WARNING);
		
		if (!m_aEntries)
		{
			Print("[DEBUG][AnimConfig] ERROR: m_aEntries is null! Config not loaded properly!", LogLevel.ERROR);
			return "";
		}
		
		Print(string.Format("[DEBUG][AnimConfig] Config has %1 entries", m_aEntries.Count()), LogLevel.WARNING);
		
		int index = 0;
		foreach (IRRU_AnimationHelperConfigEntry entry : m_aEntries)
		{
			Print(string.Format("[DEBUG][AnimConfig] Entry %1: ID=%2, Prefab=%3", index, entry.GetID(), entry.GetPrefabName()), LogLevel.WARNING);
			
			if (entry.GetID() == id)
			{
				ResourceName prefabName = entry.GetPrefabName();
				Print(string.Format("[DEBUG][AnimConfig] Found matching entry! Returning prefab: %1", prefabName), LogLevel.WARNING);
				return prefabName;
			}
			index++;
		}
		
		Print(string.Format("[DEBUG][AnimConfig] ERROR: No entry found for ID %1!", id), LogLevel.ERROR);
		return "";
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(IRRU_EAnimationHelperID, "m_eID")]
class IRRU_AnimationHelperConfigEntry
{
	[Attribute(uiwidget: UIWidgets.SearchComboBox, desc: "ID of the animation helper", enums: ParamEnumArray.FromEnum(IRRU_EAnimationHelperID))]
	protected IRRU_EAnimationHelperID m_eID;
		
	[Attribute(desc: "Prefab name of the animation helper", params: "et")]
	protected ResourceName m_sPrefabName;
		
	//------------------------------------------------------------------------------------------------
	IRRU_EAnimationHelperID GetID()
	{
		return m_eID;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPrefabName()
	{
		return m_sPrefabName;
	}
}