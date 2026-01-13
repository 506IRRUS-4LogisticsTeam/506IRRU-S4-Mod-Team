sealed class Loadout_SaveCurrentFromAiAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript() { return false; }
	override bool CanBroadcastScript() { return true; }

	override bool CanBePerformedScript(IEntity user)
	{
		return user != null;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		if (!pUserEntity)
			return;

		Loadout_SaveCurrentLoadout.SaveCurrentCharacterLoadout(pUserEntity, "CurrentLoadout");
	}
};

sealed class Loadout_ApplyRiflemanFromAiAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript() { return false; }
	override bool CanBroadcastScript() { return true; }

	override bool CanBePerformedScript(IEntity user)
	{
		return user != null;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		if (!pUserEntity)
			return;

		Loadout_ApplyRiflemanLoadout.ApplyRiflemanLoadout(pUserEntity);
	}
};

sealed class Loadout_CopyPlayerToAiAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript() { return false; }
	override bool CanBroadcastScript() { return true; }

	override bool CanBePerformedScript(IEntity user)
	{
		return user != null;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		if (!pOwnerEntity || !pUserEntity)
			return;

		Loadout_TransferHelpers.TransferLoadout(pUserEntity, pOwnerEntity);
	}
};

sealed class Loadout_CopyAiToPlayerAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript() { return false; }
	override bool CanBroadcastScript() { return true; }

	override bool CanBePerformedScript(IEntity user)
	{
		return user != null;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		if (!pOwnerEntity || !pUserEntity)
			return;

		Loadout_TransferHelpers.TransferLoadout(pOwnerEntity, pUserEntity);
	}
};

sealed class Loadout_OpenLoadoutMenuAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript() { return true; }
	override bool CanBroadcastScript() { return false; }

	override bool CanBePerformedScript(IEntity user)
	{
		return user != null;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pUserEntity)
			return;

		Loadout_LoadoutMenu.Open(pUserEntity);
	}
};
