//------------------------------------------------------------------------------------------------
// CustomNamesNetworkEntity
// Attach this as a ScriptComponent on GameMode_Base.et
// Handles replication of custom name changes + full sync to new clients
//------------------------------------------------------------------------------------------------
class CustomNamesNetworkEntityClass : SCR_BaseGameModeComponentClass
{
};


class CustomNamesNetworkEntity : SCR_BaseGameModeComponent
{
	protected static CustomNamesNetworkEntity s_Instance;

	// Optional: tune if you expect many entries (prevents oversized RPCs)
	protected const int MAX_SYNC_ENTRIES_PER_RPC = 200;

	// Called when the GameMode becomes active
	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		s_Instance = this;
		Print("[CustomNames][Network] Initialized (OnGameModeStart)", LogLevel.NORMAL);
	}

	static CustomNamesNetworkEntity Get()
	{
		return s_Instance;
	}

	//--------------------------------------------------------------------------------------------
	// CLIENT -> SERVER: receive chat command (e.g., "setname Foo", "resetname", "myname")
	//--------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcSrv_ProcessCustomNameCommand(string msg, int senderId)
	{
		CustomNamesManager mgr = CustomNamesManager.GetInstance();
		if (!mgr) return;

		// Normalize
		string trimmed = msg; trimmed.Trim();
		string lower = trimmed; lower.ToLower();

		// setname <Name>
		if (lower.StartsWith("setname "))
		{
			string newName = trimmed.Substring(8, trimmed.Length() - 8);
			newName.Trim();
			if (newName.IsEmpty()) return;

			if (mgr.ValidateCustomName(newName))
			{
				if (mgr.SetCustomName(senderId.ToString(), newName))
				{
					Print(string.Format("[CustomNames][Network] Set '%1' for player %2", newName, senderId), LogLevel.NORMAL);
					Rpc(RpcAll_UpdateCustomName, senderId.ToString(), newName);
				}
			}
			else
			{
				Print(string.Format("[CustomNames][Network] Rejected invalid name '%1'", newName), LogLevel.WARNING);
			}
		}
		// resetname
		else if (lower == "resetname")
		{
			if (mgr.SetCustomName(senderId.ToString(), ""))
			{
				Print(string.Format("[CustomNames][Network] Reset name for player %1", senderId), LogLevel.NORMAL);
				Rpc(RpcAll_UpdateCustomName, senderId.ToString(), "");
			}
		}
		// myname (handled locally by your chat code; no broadcast needed)
		else if (lower == "myname")
		{
			Print(string.Format("[CustomNames][Network] myname from %1 (no broadcast)", senderId), LogLevel.NORMAL);
		}
	}

	//--------------------------------------------------------------------------------------------
	// SERVER -> ALL: broadcast one player’s name update
	//--------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_UpdateCustomName(string playerId, string customName)
	{
		CustomNamesManager mgr = CustomNamesManager.GetInstance();
		if (mgr)
			mgr.UpdateCustomNameLocal(playerId, customName);
	}

	//--------------------------------------------------------------------------------------------
	// SERVER -> ONE CLIENT: send *all* custom names (arrays to avoid map replication)
	//--------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcCl_ReceiveAllCustomNames(array<string> ids, array<string> names)
	{
		if (!ids || !names) return;

		CustomNamesManager mgr = CustomNamesManager.GetInstance();
		if (!mgr) return;

		int count = ids.Count();
		int n = names.Count();
		int limit;
		if (count < n)
			limit = count;
		else
			limit = n;


		for (int i = 0; i < limit; i++)
		{
			string id = ids[i];
			string nm = names[i];
			if (!id.IsEmpty() && !nm.IsEmpty())
				mgr.UpdateCustomNameLocal(id, nm);
		}
	}

	//--------------------------------------------------------------------------------------------
	// SERVER helper: unicast all names to a specific player, in chunks
	//--------------------------------------------------------------------------------------------
	void SendAllCustomNamesToClient(int playerId)
	{
		if (!Replication.IsServer()) return;

		CustomNamesManager mgr = CustomNamesManager.GetInstance();
		if (!mgr) return;

		// Get the server-side map (internal; we won't RPC it directly)
		map<string, string> allMap = mgr.GetAllCustomNames();
		if (!allMap || allMap.IsEmpty())
			return;

		array<string> ids = {};
		array<string> names = {};
		ids.Reserve(allMap.Count());
		names.Reserve(allMap.Count());

		// Flatten map -> parallel arrays
		foreach (string k, string v : allMap)
		{
			if (v.IsEmpty()) continue; // skip empty (means reset / default)
			ids.Insert(k);
			names.Insert(v);

			// Chunk if necessary
			if (ids.Count() >= MAX_SYNC_ENTRIES_PER_RPC)
			{
				Rpc(RpcCl_ReceiveAllCustomNames, ids, names, playerId); // unicast via playerId
				ids.Clear();
				names.Clear();
			}
		}

		// Send any remainder
		if (ids.Count() > 0)
			Rpc(RpcCl_ReceiveAllCustomNames, ids, names, playerId);
	}
}

