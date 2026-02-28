// Capt Ringrose - 27/11/2025

class DisassembleMortarUserAction : ScriptedUserAction
{
	[Attribute("{1111111111111111}Prefabs/Items/MortarKit.et",
		uiwidget: UIWidgets.ResourceNamePicker,
		params: "et",
		desc: "Prefab to spawn on the ground when disassembling",
		category: "Disassembly")]
	protected ResourceName m_ResultPrefab;

	[Attribute("Disassemble", uiwidget: UIWidgets.EditBox, desc: "Action name shown to player")]
	protected string m_ActionName;

	[Attribute("1.0", uiwidget: UIWidgets.SpinBox, desc: "Spawn distance in meters in front of user")]
	protected float m_ForwardDistance;

	[Attribute("0.05", uiwidget: UIWidgets.SpinBox, desc: "Vertical Y offset above ground")]
	protected float m_SpawnYOffset;

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (m_ActionName.IsEmpty())
			outName = "Disassemble";
		else
			outName = m_ActionName;
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return !m_ResultPrefab.IsEmpty();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return !m_ResultPrefab.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity || m_ResultPrefab.IsEmpty())
			return;

		// Only spawn on server/authority - replication handles syncing to clients
		if (Replication.IsRunning() && !Replication.IsServer())
			return;

		World world = pOwnerEntity.GetWorld();
		if (!world)
			return;

		Resource res = Resource.Load(m_ResultPrefab);
		if (!res)
		{
			Print(string.Format(
				"[DisassembleMortarUserAction] Failed to load resource '%1'", m_ResultPrefab
			), LogLevel.ERROR);
			return;
		}

		vector userTrf[4];
		pUserEntity.GetWorldTransform(userTrf);

		vector ownerTrf[4];
		pOwnerEntity.GetWorldTransform(ownerTrf);

		vector spawnPos = userTrf[3] + userTrf[2] * m_ForwardDistance;
		spawnPos[1] = spawnPos[1] + m_SpawnYOffset;

		vector spawnTrf[4];
		spawnTrf[0] = ownerTrf[0];
		spawnTrf[1] = ownerTrf[1];
		spawnTrf[2] = ownerTrf[2];
		spawnTrf[3] = spawnPos;

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = spawnTrf;

		IEntity resultEntity = GetGame().SpawnEntityPrefab(res, world, params);
		if (!resultEntity)
		{
			Print(string.Format(
				"[DisassembleMortarUserAction] Failed to spawn prefab '%1'", m_ResultPrefab
			), LogLevel.ERROR);
			return;
		}

		// Use RplComponent.DeleteRplEntity for proper networked deletion
		RplComponent rpl = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));
		if (rpl)
			RplComponent.DeleteRplEntity(pOwnerEntity, false);
		else
			delete pOwnerEntity;
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
}
