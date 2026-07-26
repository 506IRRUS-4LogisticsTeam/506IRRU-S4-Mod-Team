//------------------------------------------------------------------------------------------------
class CustomNamesNetworkEntityClass : SCR_BaseGameModeComponentClass
{
};

class CustomNamesNetworkEntity : SCR_BaseGameModeComponent
{
	protected static CustomNamesNetworkEntity s_Instance;
	protected ref CustomNamesManager m_CustomNamesManager;
	protected bool m_bPeriodicSyncActive = false;

	protected const int MAX_SYNC_ENTRIES_PER_RPC = 200;
	protected const int PERIODIC_SYNC_INTERVAL_MS = 10000; // Sync every 10 seconds

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		s_Instance = this;
		
		if (Replication.IsServer())
		{
			m_CustomNamesManager = CustomNamesManager.GetInstance();
			if (!m_CustomNamesManager)
			{
				Print("[CustomNames] Failed to initialize CustomNamesManager", LogLevel.ERROR);
			}
			else
			{
				// Start periodic synchronization for all connected players
				StartPeriodicSync();
			}
		}
	}

	static CustomNamesNetworkEntity Get()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcSrv_ProcessCustomNameCommand(string msg, int senderId)
	{
		CustomNamesManager mgr = m_CustomNamesManager;
		if (!mgr)
			mgr = CustomNamesManager.GetInstance();
		if (!mgr)
		{
			Print("[CustomNames] Manager not available for command processing", LogLevel.WARNING);
			return;
		}

		string trimmed = msg; trimmed.Trim();
		string lower = trimmed; lower.ToLower();

		if (lower.StartsWith("setname "))
		{
			string newName = trimmed.Substring(8, trimmed.Length() - 8);
			newName.Trim();
			if (newName.IsEmpty()) return;

			if (mgr.ValidateCustomName(newName))
			{
				if (mgr.SetCustomName(senderId, newName))
				{
					Rpc(RpcAll_UpdateCustomName, senderId, newName);
				}
				else
				{
					Print(string.Format("[CustomNames] Failed to set name '%1' for player %2", newName, senderId), LogLevel.WARNING);
				}
			}
			else
			{
				Print(string.Format("[CustomNames] Rejected invalid name '%1' from player %2", newName, senderId), LogLevel.WARNING);
			}
		}
		else if (lower == "resetname")
		{
			if (mgr.SetCustomName(senderId, ""))
			{
				Rpc(RpcAll_UpdateCustomName, senderId, "");
			}
			else
			{
				Print(string.Format("[CustomNames] Failed to reset name for player %1", senderId), LogLevel.WARNING);
			}
		}
		else if (lower == "myname")
		{
			// No action needed - handled locally by chat component
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_UpdateCustomName(int playerId, string customName)
	{
		CustomNamesManager mgr = CustomNamesManager.GetInstance();
		if (mgr)
		{
			mgr.UpdateCustomNameLocal(playerId, customName);
		}
		else
		{
			Print("[CustomNames] Manager not available for name update", LogLevel.WARNING);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_ReceiveAllCustomNames(array<int> playerIds, array<string> names)
	{
		if (!playerIds || !names) return;

		CustomNamesManager mgr = CustomNamesManager.GetInstance();
		if (!mgr)
		{
			Print("[CustomNames] Manager not available for name sync", LogLevel.WARNING);
			return;
		}

		int count = playerIds.Count();
		int n = names.Count();
		int limit;
		if (count < n)
			limit = count;
		else
			limit = n;

		for (int i = 0; i < limit; i++)
		{
			int playerId = playerIds[i];
			string nm = names[i];
			if (playerId > 0 && !nm.IsEmpty())
				mgr.UpdateCustomNameLocal(playerId, nm);
		}
	}

	//------------------------------------------------------------------------------------------------
	void SendAllCustomNamesToClient(int playerId)
	{
		if (!Replication.IsServer()) return;

		CustomNamesManager mgr = m_CustomNamesManager;
		if (!mgr)
			mgr = CustomNamesManager.GetInstance();
		if (!mgr)
		{
			Print("[CustomNames] Manager not available for name sync", LogLevel.WARNING);
			return;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			Print("[CustomNames] PlayerManager not available for name sync", LogLevel.WARNING);
			return;
		}
		
		array<int> playerIds = {};
		array<string> names = {};
		
		array<int> allPlayers = {};
		playerManager.GetPlayers(allPlayers);
		
		foreach (int pId : allPlayers)
		{
			string customName = mgr.GetCustomName(pId);
			if (!customName.IsEmpty())
			{
				playerIds.Insert(pId);
				names.Insert(customName);
				
				if (playerIds.Count() >= MAX_SYNC_ENTRIES_PER_RPC)
				{
					Rpc(RpcAll_ReceiveAllCustomNames, playerIds, names);
					playerIds.Clear();
					names.Clear();
				}
			}
		}

		if (playerIds.Count() > 0)
		{
			Rpc(RpcAll_ReceiveAllCustomNames, playerIds, names);
		}

		// Schedule periodic sync to ensure all names stay updated
		GetGame().GetCallqueue().CallLater(SendPeriodicSync, 5000, false, playerId);
	}

	//------------------------------------------------------------------------------------------------
	void StartPeriodicSync()
	{
		if (!Replication.IsServer() || m_bPeriodicSyncActive) return;

		m_bPeriodicSyncActive = true;
		GetGame().GetCallqueue().CallLater(PeriodicSyncAllNames, PERIODIC_SYNC_INTERVAL_MS, true);
		Print("[CustomNames] Started periodic synchronization system", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	void PeriodicSyncAllNames()
	{
		if (!Replication.IsServer()) return;

		CustomNamesManager mgr = m_CustomNamesManager;
		if (!mgr)
			mgr = CustomNamesManager.GetInstance();
		if (!mgr) return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager) return;

		array<int> allPlayers = {};
		playerManager.GetPlayers(allPlayers);

		// Build list of all custom names to broadcast
		array<int> playerIds = {};
		array<string> names = {};

		foreach (int pId : allPlayers)
		{
			string customName = mgr.GetCustomName(pId);
			if (!customName.IsEmpty())
			{
				playerIds.Insert(pId);
				names.Insert(customName);

				if (playerIds.Count() >= MAX_SYNC_ENTRIES_PER_RPC)
				{
					Rpc(RpcAll_ReceiveAllCustomNames, playerIds, names);
					playerIds.Clear();
					names.Clear();
				}
			}
		}

		if (playerIds.Count() > 0)
		{
			Rpc(RpcAll_ReceiveAllCustomNames, playerIds, names);
		}
	}

	//------------------------------------------------------------------------------------------------
	void SendPeriodicSync(int targetPlayerId)
	{
		if (!Replication.IsServer()) return;

		CustomNamesManager mgr = m_CustomNamesManager;
		if (!mgr)
			mgr = CustomNamesManager.GetInstance();
		if (!mgr) return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager) return;

		array<int> allPlayers = {};
		playerManager.GetPlayers(allPlayers);

		// Broadcast all active custom names to ensure sync
		foreach (int pId : allPlayers)
		{
			string customName = mgr.GetCustomName(pId);
			if (!customName.IsEmpty())
			{
				Rpc(RpcAll_UpdateCustomName, pId, customName);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerConnectedWithRetry(int playerId, int attemptNumber = 0)
	{
		if (!Replication.IsServer()) return;

		CustomNamesManager mgr = m_CustomNamesManager;
		if (!mgr)
			mgr = CustomNamesManager.GetInstance();
		if (!mgr)
		{
			Print("[CustomNames] Manager not available for player connection", LogLevel.WARNING);
			return;
		}
		
		string identityId = "";
		BackendApi backendApi = GetGame().GetBackendApi();
		if (backendApi)
		{
			identityId = backendApi.GetPlayerIdentityId(playerId);
		}
		else
		{
			Print("[CustomNames] BackendApi not available for identity lookup", LogLevel.WARNING);
		}
		
		if (!identityId.IsEmpty())
		{
			// First, send all existing custom names to the newly connected player
			GetGame().GetCallqueue().CallLater(SendAllCustomNamesToClient, 500, false, playerId);

			string customName = mgr.GetCustomNameByUID(identityId);
			if (!customName.IsEmpty())
			{
				// Restore the player's custom name locally on server
				mgr.UpdateCustomNameLocal(playerId, customName);
				// Broadcast this player's custom name to ALL connected players
				GetGame().GetCallqueue().CallLater(BroadcastNameDelayed, 1000, false, playerId, customName);
				// Send additional broadcast after a delay to ensure all clients receive it
				GetGame().GetCallqueue().CallLater(BroadcastNameDelayed, 3000, false, playerId, customName);
				SendNameRestorationNotification(playerId, customName);
			}
			else
			{
				SendNameRestorationNotification(playerId, "");
			}
		}
		else if (attemptNumber < 4)
		{
			int delays[5] = {0, 100, 250, 500, 1000};
			int nextDelay = delays[attemptNumber + 1];
			
			GetGame().GetCallqueue().CallLater(OnPlayerConnectedWithRetry, nextDelay, false, 
				playerId, attemptNumber + 1);
		}
		else
		{
			Print(string.Format("[CustomNames] Failed to get player identity after %1 attempts for player %2", 
				attemptNumber + 1, playerId), LogLevel.WARNING);
			SendAllCustomNamesToClient(playerId);
		}
	}
	
	void BroadcastNameDelayed(int playerId, string customName)
	{
		Rpc(RpcAll_UpdateCustomName, playerId, customName);
	}
	
	//------------------------------------------------------------------------------------------------
	void SendNameRestorationNotification(int playerId, string customName)
	{
		if (!customName.IsEmpty())
		{
			string message = string.Format("Your custom name '%1' has been restored", customName);
			GetGame().GetCallqueue().CallLater(SendDelayedChatNotification, 5000, false, playerId, message);
		}
		else
		{
			string message = "No custom name found. Use 'setname <YourName>' to set one";
			GetGame().GetCallqueue().CallLater(SendDelayedChatNotification, 5000, false, playerId, message);
		}
	}
	
	void SendDelayedChatNotification(int playerId, string message)
	{
		Rpc(RpcDo_ShowChatNotification, message, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_ShowChatNotification(string message, int targetPlayerId)
	{
		PlayerController localPC = GetGame().GetPlayerController();
		if (!localPC) return;
		
		int localPlayerId = localPC.GetPlayerId();
		if (localPlayerId != targetPlayerId) return;
		
		SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(localPC.FindComponent(SCR_ChatComponent));
		if (chatComp)
		{
			chatComp.ShowMessage(message);
		}
		else
		{
			Print("[CustomNames] Chat component not available for notification", LogLevel.WARNING);
		}
	}
}