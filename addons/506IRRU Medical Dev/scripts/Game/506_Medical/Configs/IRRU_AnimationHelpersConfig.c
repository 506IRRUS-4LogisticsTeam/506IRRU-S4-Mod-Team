//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true)]
class IRRU_AnimationHelpersConfig
{
	[Attribute(desc: "Registered animation helpers")]
	protected ref array<ref IRRU_AnimationHelperConfigEntry> m_aEntries;
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPrefabName(IRRU_EAnimationHelperID id)
	{
		if (!m_aEntries)
		{
			Print("[AnimConfig] m_aEntries is null - config not loaded properly", LogLevel.ERROR);
			return "";
		}

		foreach (IRRU_AnimationHelperConfigEntry entry : m_aEntries)
		{
			if (entry.GetID() == id)
				return entry.GetPrefabName();
		}

		Print(string.Format("[AnimConfig] No entry found for ID %1", id), LogLevel.ERROR);
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