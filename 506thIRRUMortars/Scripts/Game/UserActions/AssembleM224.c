// Capt Ringrose - 27/11/2025

class AssembleMortarUserAction : ScriptedUserAction
{
	[Attribute("{1111111111111111}Prefabs/Vehicles/Static/Mortar_82mm.et",
		uiwidget: UIWidgets.ResourceNamePicker,
		params: "et",
		desc: "Static prefab to spawn (e.g. mortar)",
		category: "Convert To Static")]
	protected ResourceName m_TargetPrefab;

	[Attribute("Build Mortar", uiwidget: UIWidgets.EditBox, desc: "Action name shown to player")]
	protected string m_ActionName;

	[Attribute("0.0", uiwidget: UIWidgets.SpinBox, desc: "Vertical offset in meters above ground")]
	protected float m_SpawnYOffset;

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (m_ActionName.IsEmpty())
			outName = "Convert to Static";
		else
			outName = m_ActionName;
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return !m_TargetPrefab.IsEmpty();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return !m_TargetPrefab.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || m_TargetPrefab.IsEmpty())
			return;

		World world = pOwnerEntity.GetWorld();
		if (!world)
			return;

		Resource res = Resource.Load(m_TargetPrefab);
		if (!res)
		{
			Print(string.Format(
				"[ConvertToStaticPrefabUserAction] Failed to load resource '%1'", m_TargetPrefab
			), LogLevel.ERROR);
			return;
		}

		vector transform[4];
		pOwnerEntity.GetWorldTransform(transform);

		if (m_SpawnYOffset != 0.0)
		{
			transform[3][1] = transform[3][1] + m_SpawnYOffset;
		}

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = transform;

		IEntity newEntity = GetGame().SpawnEntityPrefab(res, world, params);
		if (!newEntity)
		{
			Print(string.Format(
				"[ConvertToStaticPrefabUserAction] Failed to spawn prefab '%1'", m_TargetPrefab
			), LogLevel.ERROR);
			return;
		}

		delete pOwnerEntity;
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
}
