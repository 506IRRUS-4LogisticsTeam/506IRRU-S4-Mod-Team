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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "Rifleman");
	}
};

sealed class Loadout_ApplyAutoriflemanFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "Autorifleman");
	}
};

sealed class Loadout_ApplyGrenadierFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "Grenadier");
	}
};

sealed class Loadout_ApplyMedicFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "Medic");
	}
};

sealed class Loadout_ApplyTeamLeaderFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "TeamLeader");
	}
};

sealed class Loadout_ApplySquadLeaderFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "SquadLeader");
	}
};

sealed class Loadout_ApplyATGunnerFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "ATGunner");
	}
};

sealed class Loadout_ApplyATAssistantrFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "ATAssistant");
	}
};

sealed class Loadout_ApplyMGGunnerFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "MGGunner");
	}
};

sealed class Loadout_ApplyMGAssistantFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "MGAssistant");
	}
};

sealed class Loadout_ApplyOtherOneFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "OtherOne");
	}
};

sealed class Loadout_ApplyOtherTwoFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "OtherTwo");
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

sealed class Loadout_ApplyOtherThreeFromAiAction : ScriptedUserAction
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

		Loadout_ApplyLoadout.ApplyLoadoutByName(pUserEntity, "OtherThree");
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

sealed class Loadout_OpenExportLoadoutAction : ScriptedUserAction
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

		Loadout_Export_Menu.Open(pUserEntity);
	}
};

sealed class Loadout_OpenImportLoadoutAction : ScriptedUserAction
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

		Loadout_Import_Menu.Open(pUserEntity);
	}
};
