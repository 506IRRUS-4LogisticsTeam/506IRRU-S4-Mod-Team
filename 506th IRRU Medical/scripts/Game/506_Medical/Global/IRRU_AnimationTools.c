//------------------------------------------------------------------------------------------------
class IRRU_AnimationTools
{
	protected static ref IRRU_AnimationHelpersConfig s_AnimationHelpersConfig;
	protected static const ResourceName ANIMATION_HELPERS_CONFIG_NAME = "{C23FABB4BE5F1F55}Configs/IRRU_AnimationHelpers.conf";
	
	//------------------------------------------------------------------------------------------------
	protected static void Init()
	{
		Print("[DEBUG][AnimTools] Init() called", LogLevel.WARNING);
		Print(string.Format("[DEBUG][AnimTools] Loading config from: %1", ANIMATION_HELPERS_CONFIG_NAME), LogLevel.WARNING);
		
		s_AnimationHelpersConfig = SCR_ConfigHelperT<IRRU_AnimationHelpersConfig>.GetConfigObject(ANIMATION_HELPERS_CONFIG_NAME);
		
		if (s_AnimationHelpersConfig)
		{
			Print("[DEBUG][AnimTools] Config loaded successfully!", LogLevel.WARNING);
		}
		else
		{
			Print("[DEBUG][AnimTools] ERROR: Failed to load config! Check if file exists and is valid!", LogLevel.ERROR);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	static ACE_AnimationHelperCompartment AnimateWithHelperCompartment(IRRU_EAnimationHelperID helperID, notnull SCR_ChimeraCharacter performer)
	{
		vector transform[4];
		performer.GetWorldTransform(transform);
		return AnimateWithHelperCompartment(helperID, performer, transform);
	}
	
	//------------------------------------------------------------------------------------------------
	static ACE_AnimationHelperCompartment AnimateWithHelperCompartment(IRRU_EAnimationHelperID helperID, notnull SCR_ChimeraCharacter performer, vector transform[4])
	{
		Print("[DEBUG][AnimTools] ========== AnimateWithHelperCompartment START ==========", LogLevel.WARNING);
		Print(string.Format("[DEBUG][AnimTools] Helper ID requested: %1", helperID), LogLevel.WARNING);
		Print(string.Format("[DEBUG][AnimTools] Performer: %1", performer), LogLevel.WARNING);
		Print(string.Format("[DEBUG][AnimTools] Transform position: %1", transform[3]), LogLevel.WARNING);
		
		if (!s_AnimationHelpersConfig)
		{
			Print("[DEBUG][AnimTools] Config not loaded, attempting Init()", LogLevel.WARNING);
			Init();
		}
		
		if (!s_AnimationHelpersConfig)
		{
			Print("[DEBUG][AnimTools] ERROR: Animation helpers config still not found after Init()!", LogLevel.ERROR);
			Print(string.Format("[DEBUG][AnimTools] Expected config path: %1", ANIMATION_HELPERS_CONFIG_NAME), LogLevel.ERROR);
			return null;
		}
		
		Print("[DEBUG][AnimTools] Config loaded, getting prefab name...", LogLevel.WARNING);
		ResourceName prefabName = s_AnimationHelpersConfig.GetPrefabName(helperID);
		Print(string.Format("[DEBUG][AnimTools] Prefab name from config: '%1'", prefabName), LogLevel.WARNING);
		
		if (prefabName == "")
		{
			Print(string.Format("[DEBUG][AnimTools] ERROR: No prefab configured for helper ID %1!", helperID), LogLevel.ERROR);
			Print("[DEBUG][AnimTools] Check that IRRU_AnimationHelpers.conf has an entry for CPR!", LogLevel.ERROR);
			return null;
		}
		
		Resource res = Resource.Load(prefabName);
		if (!res.IsValid())
		{
			Print(string.Format("[DEBUG][AnimTools] ERROR: Failed to load prefab resource: %1", prefabName), LogLevel.ERROR);
			Print("[DEBUG][AnimTools] Check that the prefab file exists at this path!", LogLevel.ERROR);
			return null;
		}
		
		Print("[DEBUG][AnimTools] Resource loaded, spawning entity...", LogLevel.WARNING);
		
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = transform;
		
		IEntity spawnedEntity = GetGame().SpawnEntityPrefab(res, null, params);
		if (!spawnedEntity)
		{
			Print("[DEBUG][AnimTools] ERROR: SpawnEntityPrefab returned null!", LogLevel.ERROR);
			return null;
		}
		
		Print(string.Format("[DEBUG][AnimTools] Entity spawned: %1, Type: %2", spawnedEntity, spawnedEntity.ClassName()), LogLevel.WARNING);
		
		ACE_AnimationHelperCompartment helper = ACE_AnimationHelperCompartment.Cast(spawnedEntity);
		if (!helper)
		{
			Print(string.Format("[DEBUG][AnimTools] ERROR: Spawned entity is not ACE_AnimationHelperCompartment! It's: %1", spawnedEntity.ClassName()), LogLevel.ERROR);
			Print("[DEBUG][AnimTools] Check that the prefab uses ACE_AnimationHelperCompartment as its class!", LogLevel.ERROR);
			return null;
		}
		
		Print("[DEBUG][AnimTools] Helper cast successful, initializing...", LogLevel.WARNING);
		helper.Init(performer);
		Print("[DEBUG][AnimTools] ========== AnimateWithHelperCompartment SUCCESS ==========", LogLevel.WARNING);
		
		return helper;
	}
	
	//------------------------------------------------------------------------------------------------
	static ACE_AnimationHelperCompartment GetHelperCompartment(notnull IEntity performer)
	{
		return ACE_AnimationHelperCompartment.Cast(performer.GetParent());
	}
	
	//------------------------------------------------------------------------------------------------
	static void TerminateHelperCompartment(notnull IEntity performer, EGetOutType getOutType = EGetOutType.ANIMATED)
	{
		ACE_AnimationHelperCompartment helper = GetHelperCompartment(performer);
		if (helper)
			helper.Terminate(getOutType);
	}
}