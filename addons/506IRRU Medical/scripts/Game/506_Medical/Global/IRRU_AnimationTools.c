class IRRU_AnimationTools
{
	protected static ref IRRU_AnimationHelpersConfig s_AnimationHelpersConfig;
	protected static const ResourceName ANIMATION_HELPERS_CONFIG_NAME = "{C23FABB4BE5F1F55}Configs/IRRU_AnimationHelpers.conf";

	//------------------------------------------------------------------------------------------------
	//! Spawn the helper compartment registered for helperID at transform and seat the performer in it
	static ACE_AnimationHelperCompartment AnimateWithHelperCompartment(IRRU_EAnimationHelperID helperID, notnull SCR_ChimeraCharacter performer, vector transform[4])
	{
		if (!s_AnimationHelpersConfig)
			s_AnimationHelpersConfig = SCR_ConfigHelperT<IRRU_AnimationHelpersConfig>.GetConfigObject(ANIMATION_HELPERS_CONFIG_NAME);
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

		ACE_AnimationHelperCompartment helper = ACE_AnimationHelperCompartment.Cast(GetGame().SpawnEntityPrefab(res, null, params));
		if (!helper)
			return null;

		helper.Init(performer);
		return helper;
	}
}
