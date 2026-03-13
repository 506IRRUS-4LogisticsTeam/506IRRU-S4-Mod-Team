class IRRU_AnimationTools
{
	protected static ref IRRU_AnimationHelpersConfig s_AnimationHelpersConfig;
	protected static const ResourceName ANIMATION_HELPERS_CONFIG_NAME = "{C23FABB4BE5F1F55}Configs/IRRU_AnimationHelpers.conf";

	protected static void Init()
	{
		s_AnimationHelpersConfig = SCR_ConfigHelperT<IRRU_AnimationHelpersConfig>.GetConfigObject(ANIMATION_HELPERS_CONFIG_NAME);
	}

	static ACE_AnimationHelperCompartment AnimateWithHelperCompartment(IRRU_EAnimationHelperID helperID, notnull SCR_ChimeraCharacter performer)
	{
		vector transform[4];
		performer.GetWorldTransform(transform);
		return AnimateWithHelperCompartment(helperID, performer, transform);
	}

	static ACE_AnimationHelperCompartment AnimateWithHelperCompartment(IRRU_EAnimationHelperID helperID, notnull SCR_ChimeraCharacter performer, vector transform[4])
	{
		if (!s_AnimationHelpersConfig)
			Init();

		if (!s_AnimationHelpersConfig)
			return null;

		ResourceName prefabName = s_AnimationHelpersConfig.GetPrefabName(helperID);
		if (prefabName == "")
			return null;

		Resource res = Resource.Load(prefabName);
		if (!res.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = transform;

		IEntity spawnedEntity = GetGame().SpawnEntityPrefab(res, null, params);
		if (!spawnedEntity)
			return null;

		ACE_AnimationHelperCompartment helper = ACE_AnimationHelperCompartment.Cast(spawnedEntity);
		if (!helper)
			return null;

		helper.Init(performer);
		return helper;
	}

	static ACE_AnimationHelperCompartment GetHelperCompartment(notnull IEntity performer)
	{
		return ACE_AnimationHelperCompartment.Cast(performer.GetParent());
	}

	static void TerminateHelperCompartment(notnull IEntity performer, EGetOutType getOutType = EGetOutType.ANIMATED)
	{
		ACE_AnimationHelperCompartment helper = GetHelperCompartment(performer);
		if (helper)
			helper.Terminate(getOutType);
	}
}
