modded class SCR_PlayerLoadoutComponent
{
	ref ScriptInvoker m_IRRU_OnKitPermissions = new ScriptInvoker();
	ref ScriptInvoker m_IRRU_OnSharedKitList = new ScriptInvoker();
	ref ScriptInvoker m_IRRU_OnKitApplyResult = new ScriptInvoker();
	ref ScriptInvoker m_IRRU_OnSharedKitWriteResult = new ScriptInvoker();
	ref ScriptInvoker m_IRRU_OnPersonalKitCapture = new ScriptInvoker();

	void IRRU_RequestKitPermissions()
	{
		Rpc(IRRU_RpcAskKitPermissions);
	}

	void IRRU_RequestSharedKitList()
	{
		Rpc(IRRU_RpcAskSharedKitList);
	}

	void IRRU_RequestLoadSharedKit(string name)
	{
		Rpc(IRRU_RpcAskLoadSharedKit, name);
	}

	void IRRU_RequestSaveSharedKit(string name, bool overwrite)
	{
		Rpc(IRRU_RpcAskSaveSharedKit, name, overwrite);
	}

	void IRRU_RequestPersonalKitCapture(string name)
	{
		Rpc(IRRU_RpcAskPersonalKitCapture, name);
	}

	void IRRU_RequestRenameSharedKit(string oldName, string newName)
	{
		Rpc(IRRU_RpcAskRenameSharedKit, oldName, newName);
	}

	void IRRU_RequestDeleteSharedKit(string name)
	{
		Rpc(IRRU_RpcAskDeleteSharedKit, name);
	}

	void IRRU_RequestApplyLoadout(string name, string loadout)
	{
		Rpc(IRRU_RpcAskApplyLoadout, name, loadout);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void IRRU_RpcAskKitPermissions()
	{
		int playerId = IRRU_GetAuthenticatedPlayerId();
		Rpc(IRRU_RpcDoKitPermissions, playerId > 0 && IRRU_CanManageSharedKits(playerId));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void IRRU_RpcDoKitPermissions(bool canManage)
	{
		m_IRRU_OnKitPermissions.Invoke(canManage);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void IRRU_RpcAskSharedKitList()
	{
		array<string> names;
		string error;
		bool ok = Replication.IsServer() && IRRU_LoadoutManagerSystem.ListKits(true, names, error);
		Rpc(IRRU_RpcDoSharedKitList, ok, IRRU_PackKitNames(names), error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void IRRU_RpcDoSharedKitList(bool ok, string packedNames, string error)
	{
		array<string> names = {};
		if (!packedNames.IsEmpty())
			packedNames.Split("\n", names, true);
		m_IRRU_OnSharedKitList.Invoke(ok, names, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void IRRU_RpcAskLoadSharedKit(string name)
	{
		int playerId = IRRU_GetAuthenticatedPlayerId();
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		string loadout;
		string error;
		bool ok = Replication.IsServer() && character && IRRU_LoadoutManagerSystem.LoadKit(true, name, loadout, error);
		if (ok)
			ok = IRRU_LoadoutManagerSystem.Apply(character, loadout, error);
		if (!ok && error.IsEmpty())
			error = "apply-failed";
		Rpc(IRRU_RpcDoKitApplyResult, ok, name, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void IRRU_RpcAskApplyLoadout(string name, string loadout)
	{
		int playerId = IRRU_GetAuthenticatedPlayerId();
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		string error = "";
		bool ok = Replication.IsServer() && playerId > 0 && character && IRRU_LoadoutManagerSystem.Apply(character, loadout, error);
		if (!ok && error.IsEmpty())
			error = "apply-failed";
		Rpc(IRRU_RpcDoKitApplyResult, ok, name, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void IRRU_RpcAskPersonalKitCapture(string name)
	{
		int playerId = IRRU_GetAuthenticatedPlayerId();
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		string loadout;
		string error = "";
		bool ok = Replication.IsServer() && playerId > 0 && character && IRRU_LoadoutManagerSystem.Capture(character, loadout);
		if (!ok)
			error = "capture-failed";
		Rpc(IRRU_RpcDoPersonalKitCapture, ok, name, loadout, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void IRRU_RpcDoPersonalKitCapture(bool ok, string name, string loadout, string error)
	{
		m_IRRU_OnPersonalKitCapture.Invoke(ok, name, loadout, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void IRRU_RpcDoKitApplyResult(bool ok, string name, string error)
	{
		m_IRRU_OnKitApplyResult.Invoke(ok, name, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void IRRU_RpcAskSaveSharedKit(string name, bool overwrite)
	{
		int playerId = IRRU_GetAuthenticatedPlayerId();
		if (playerId <= 0 || !IRRU_CanManageSharedKits(playerId))
		{
			Rpc(IRRU_RpcDoSharedKitWriteResult, "SAVE", false, name, "permission-denied");
			return;
		}

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		string loadout;
		string error;
		bool ok = character && IRRU_LoadoutManagerSystem.Capture(character, loadout);
		if (!ok)
			error = "capture-failed";
		if (ok)
			ok = IRRU_LoadoutManagerSystem.SaveKit(true, name, loadout, overwrite, error);
		Rpc(IRRU_RpcDoSharedKitWriteResult, "SAVE", ok, name, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void IRRU_RpcAskRenameSharedKit(string oldName, string newName)
	{
		int playerId = IRRU_GetAuthenticatedPlayerId();
		string error = "";
		bool ok = playerId > 0 && IRRU_CanManageSharedKits(playerId);
		if (!ok)
			error = "permission-denied";
		if (ok)
			ok = IRRU_LoadoutManagerSystem.RenameKit(true, oldName, newName, error);
		Rpc(IRRU_RpcDoSharedKitWriteResult, "RENAME", ok, newName, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void IRRU_RpcAskDeleteSharedKit(string name)
	{
		int playerId = IRRU_GetAuthenticatedPlayerId();
		string error = "";
		bool ok = playerId > 0 && IRRU_CanManageSharedKits(playerId);
		if (!ok)
			error = "permission-denied";
		if (ok)
			ok = IRRU_LoadoutManagerSystem.DeleteKit(true, name, error);
		Rpc(IRRU_RpcDoSharedKitWriteResult, "DELETE", ok, name, error);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void IRRU_RpcDoSharedKitWriteResult(string operation, bool ok, string name, string error)
	{
		m_IRRU_OnSharedKitWriteResult.Invoke(operation, ok, name, error);
	}

	protected int IRRU_GetAuthenticatedPlayerId()
	{
		if (!Replication.IsServer())
			return 0;
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetOwner());
		if (!playerController)
			return 0;
		return playerController.GetPlayerId();
	}

	protected bool IRRU_CanManageSharedKits(int playerId)
	{
		if (SCR_Global.IsAdmin(playerId))
			return true;

		SCR_EditorManagerCore editorCore = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!editorCore)
			return false;

		array<SCR_EditorManagerEntity> editorEntities = {};
		editorCore.GetEditorEntities(editorEntities);
		foreach (SCR_EditorManagerEntity editorEntity : editorEntities)
		{
			if (editorEntity && editorEntity.GetPlayerID() == playerId && !editorEntity.IsLimited())
				return true;
		}
		return false;
	}

	protected string IRRU_PackKitNames(array<string> names)
	{
		string packed = "";
		if (!names)
			return packed;
		foreach (string name : names)
		{
			if (!packed.IsEmpty())
				packed += "\n";
			packed += name;
		}
		return packed;
	}
}
