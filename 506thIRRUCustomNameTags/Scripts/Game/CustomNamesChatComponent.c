//------------------------------------------------------------------------------------------------
modded class SCR_ChatComponent : BaseChatComponent
{
	protected const string LOG_PREFIX_CUSTOM_NAMES = "[CustomNames][Chat]";
	
	//------------------------------------------------------------------------------------------------
	override void OnNewMessage(string msg, int channelId, int senderId)
	{
		Print(string.Format("%1 [DEBUG] OnNewMessage triggered - Channel: %2, Sender: %3, Message: '%4'", 
			LOG_PREFIX_CUSTOM_NAMES, channelId, senderId, msg), LogLevel.NORMAL);
		
		PlayerController localPlayerController = GetGame().GetPlayerController();
		if (localPlayerController)
		{
			int localPlayerId = localPlayerController.GetPlayerId();
			Print(string.Format("%1 [DEBUG] Local player ID: %2, Checking for command...", 
				LOG_PREFIX_CUSTOM_NAMES, localPlayerId), LogLevel.NORMAL);
			
			if (localPlayerId == senderId && senderId > 0 && msg && !msg.IsEmpty())
			{
				if (IsCustomNameCommand(msg))
				{
					Print(string.Format("%1 [DEBUG] Custom name command detected: '%2'", 
						LOG_PREFIX_CUSTOM_NAMES, msg), LogLevel.NORMAL);
					
					ProcessCustomNameCommand(msg, senderId);
					
					Print(string.Format("%1 [DEBUG] Sending RPC to server - RpcSrv_ProcessCustomNameCommand", 
						LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
					CustomNamesNetworkEntity net = CustomNamesNetworkEntity.Get();
					if (net)
					    net.Rpc(net.RpcSrv_ProcessCustomNameCommand, msg, senderId);
					
					Print(string.Format("%1 [DEBUG] RPC sent, returning from OnNewMessage", 
						LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
					return;
				}
			}
		}
		
		super.OnNewMessage(msg, channelId, senderId);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsCustomNameCommand(string msg)
	{
		if (!msg || msg.IsEmpty()) return false;
		
		string trimmedMsg = msg;
		trimmedMsg.Trim();
		trimmedMsg.ToLower();
		
		return (trimmedMsg.StartsWith("setname ") || 
		        trimmedMsg == "resetname" || 
		        trimmedMsg == "myname");
	}

	//------------------------------------------------------------------------------------------------
	protected void ProcessCustomNameCommand(string msg, int playerId)
	{
		Print(string.Format("%1 Processing command locally: '%2'", LOG_PREFIX_CUSTOM_NAMES, msg), LogLevel.NORMAL);
		
		string trimmedMsg = msg;
		trimmedMsg.Trim();
		string lowerMsg = trimmedMsg;
		lowerMsg.ToLower();
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
		{
			SendChatFeedback("Custom Names system not available");
			return;
		}
		
		manager.CheckAndRestoreCustomName(playerId);
		
		if (lowerMsg.StartsWith("setname "))
		{
			string newName = trimmedMsg.Substring(8, trimmedMsg.Length() - 8);
			newName.Trim();
			
			if (newName.IsEmpty())
			{
				SendChatFeedback("Usage: setname <YourName>");
				return;
			}
			
			if (manager.ValidateCustomName(newName))
			{
				if (manager.SetCustomName(playerId, newName))
				{
					SendChatFeedback(string.Format("Name set to: %1", newName));
					Print(string.Format("%1 Name set to: %2 for player %3", 
						LOG_PREFIX_CUSTOM_NAMES, newName, playerId), LogLevel.NORMAL);
				}
				else
				{
					SendChatFeedback("Failed to set name");
				}
			}
			else
			{
				SendChatFeedback(string.Format("Invalid name: %1", newName));
			}
		}
		else if (lowerMsg == "resetname")
		{
			if (manager.SetCustomName(playerId, ""))
			{
				SendChatFeedback("Name reset to default");
			}
			else
			{
				SendChatFeedback("Failed to reset name");
			}
		}
		else if (lowerMsg == "myname")
		{
			string currentName = manager.GetCustomName(playerId);
			if (currentName.IsEmpty())
			{
				SendChatFeedback("You have no custom name set");
			}
			else
			{
				SendChatFeedback(string.Format("Your current name: %1", currentName));
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcSrv_ProcessCustomNameCommand(string msg, int senderId)
	{
		Print(string.Format("%1 [DEBUG] ========= SERVER RPC RECEIVED ==========", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Command: '%2'", LOG_PREFIX_CUSTOM_NAMES, msg), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Sender ID: %2", LOG_PREFIX_CUSTOM_NAMES, senderId), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Is Server: %2", LOG_PREFIX_CUSTOM_NAMES, Replication.IsServer()), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] =======================================", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		
		if (!Replication.IsServer()) 
		{
			Print(string.Format("%1 RpcSrv_ProcessCustomNameCommand called on non-server!", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
			return;
		}
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			array<int> players = {};
			playerManager.GetPlayers(players);
			Print(string.Format("%1 [DEBUG] Connected players count: %2", 
				LOG_PREFIX_CUSTOM_NAMES, players.Count()), LogLevel.NORMAL);
			
			Print(string.Format("%1 [DEBUG] Connected player IDs:", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
			foreach (int pid : players)
			{
				string pname = playerManager.GetPlayerName(pid);
				Print(string.Format("%1 [DEBUG]   - Player %2: %3", LOG_PREFIX_CUSTOM_NAMES, pid, pname), LogLevel.NORMAL);
			}
		}
		
		ProcessCustomNameCommand(msg, senderId);
		
		string trimmedMsg = msg;
		trimmedMsg.Trim();
		string lowerMsg = trimmedMsg;
		lowerMsg.ToLower();
		
		if (lowerMsg.StartsWith("setname "))
		{
			string newName = trimmedMsg.Substring(8, trimmedMsg.Length() - 8);
			newName.Trim();
			
			Print(string.Format("%1 [DEBUG] *** BROADCASTING NAME CHANGE ***", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
			Print(string.Format("%1 [DEBUG] Player %2 -> '%3'", LOG_PREFIX_CUSTOM_NAMES, senderId, newName), LogLevel.NORMAL);
			
			BroadcastNameUpdateToAllClients(senderId.ToString(), newName);
			
			Print(string.Format("%1 [DEBUG] *** BROADCAST COMPLETE ***", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		}
		else if (lowerMsg == "resetname")
		{
			Print(string.Format("%1 SERVER: Broadcasting name reset for player %2", 
				LOG_PREFIX_CUSTOM_NAMES, senderId), LogLevel.NORMAL);
			BroadcastNameUpdateToAllClients(senderId.ToString(), "");
			
			Print(string.Format("%1 SERVER: Broadcast complete for name reset of player %2", 
				LOG_PREFIX_CUSTOM_NAMES, senderId), LogLevel.NORMAL);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void BroadcastNameUpdateToAllClients(string playerId, string customName)
	{
		Print(string.Format("%1 [DEBUG] BroadcastNameUpdateToAllClients called", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] About to call RpcAll_UpdateCustomName", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Parameters - PlayerId: %2, CustomName: '%3'", 
			LOG_PREFIX_CUSTOM_NAMES, playerId, customName), LogLevel.NORMAL);
		
		int playerIdInt = playerId.ToInt();
		RpcAll_UpdateCustomName(playerIdInt, customName);
		
		Print(string.Format("%1 [DEBUG] RpcAll_UpdateCustomName has been called", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Broadcast RPC should now be propagating to all clients", 
			LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_UpdateCustomName(int playerId, string customName)
	{
		Print(string.Format("%1 [DEBUG] ######### BROADCAST RPC RECEIVED #########", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		
		PlayerController localPC = GetGame().GetPlayerController();
		int localPlayerId = -1;
		if (localPC)
			localPlayerId = localPC.GetPlayerId();
		
		bool isServer = Replication.IsServer();
		bool isHost = (isServer && localPlayerId > 0);
		
		Print(string.Format("%1 [DEBUG] Received update for Player: %2", LOG_PREFIX_CUSTOM_NAMES, playerId), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] New custom name: '%2'", LOG_PREFIX_CUSTOM_NAMES, customName), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Local player ID: %2", LOG_PREFIX_CUSTOM_NAMES, localPlayerId), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Is this a server: %2", LOG_PREFIX_CUSTOM_NAMES, isServer), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Is this a host: %2", LOG_PREFIX_CUSTOM_NAMES, isHost), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] #########################################", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			Print(string.Format("%1 [DEBUG] CustomNamesManager found, updating local cache", 
				LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
			
			manager.UpdateCustomNameLocal(playerId, customName);
			
			Print(string.Format("%1 [DEBUG] Local cache updated successfully", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
			Print(string.Format("%1 [DEBUG] Player %2 custom name is now: '%3'", 
				LOG_PREFIX_CUSTOM_NAMES, playerId, customName), LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("%1 [ERROR] CustomNamesManager is NULL - cannot update!", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	static void BroadcastCustomNameUpdate(string playerId, string customName)
	{
		Print(string.Format("[CustomNames][Chat] Broadcasting name update: Player %1 -> %2", playerId, customName), LogLevel.NORMAL);
		
		if (!Replication.IsServer())
		{
			Print("[CustomNames][Chat] BroadcastCustomNameUpdate called on client - this should only happen on server", LogLevel.WARNING);
			return;
		}
		Print("[CustomNames][Chat] Server-side name update completed", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendChatFeedback(string message)
	{
		Print(string.Format("%1 FEEDBACK: %2", LOG_PREFIX_CUSTOM_NAMES, message), LogLevel.NORMAL);
		
		SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(this);
		if (chatComp)
		{
			chatComp.ShowMessage(message);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Send all custom names to a specific client (called when they join)
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_SyncAllCustomNames(array<int> playerIds, array<string> customNames)
	{
		Print(string.Format("%1 [SYNC] ========= RECEIVING ALL CUSTOM NAMES ==========", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		Print(string.Format("%1 [SYNC] Received %2 custom name entries", LOG_PREFIX_CUSTOM_NAMES, playerIds.Count()), LogLevel.NORMAL);
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
		{
			Print(string.Format("%1 [SYNC] ERROR: CustomNamesManager not available", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
			return;
		}
		
		for (int i = 0; i < playerIds.Count(); i++)
		{
			int playerId = playerIds[i];
			string customName = customNames[i];
			
			Print(string.Format("%1 [SYNC] Syncing: Player %2 => '%3'", 
				LOG_PREFIX_CUSTOM_NAMES, playerId, customName), LogLevel.NORMAL);
			
			manager.UpdateCustomNameLocal(playerId, customName);
		}
		
		Print(string.Format("%1 [SYNC] All custom names synchronized", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		Print(string.Format("%1 [SYNC] ================================================", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	// Server method to send all custom names to a specific player
	void SendAllCustomNamesToPlayer(int targetPlayerId)
	{
		if (!Replication.IsServer())
		{
			Print(string.Format("%1 [SYNC] ERROR: SendAllCustomNamesToPlayer called on client", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
			return;
		}
		
		Print(string.Format("%1 [SYNC] Preparing to send all custom names to player %2", 
			LOG_PREFIX_CUSTOM_NAMES, targetPlayerId), LogLevel.NORMAL);
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
		{
			Print(string.Format("%1 [SYNC] ERROR: CustomNamesManager not available", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
			return;
		}
		
		// Get all currently connected players and their custom names
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			Print(string.Format("%1 [SYNC] ERROR: PlayerManager not available", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
			return;
		}
		
		array<int> playerIds = {};
		array<string> customNames = {};
		
		array<int> allPlayers = {};
		playerManager.GetPlayers(allPlayers);
		
		foreach (int playerId : allPlayers)
		{
			string customName = manager.GetCustomName(playerId);
			if (!customName.IsEmpty())
			{
				playerIds.Insert(playerId);
				customNames.Insert(customName);
				Print(string.Format("%1 [SYNC] Adding to sync: Player %2 => '%3'", 
					LOG_PREFIX_CUSTOM_NAMES, playerId, customName), LogLevel.NORMAL);
			}
		}
		
		if (playerIds.Count() > 0)
		{
			Print(string.Format("%1 [SYNC] Sending %2 custom names to player %3", 
				LOG_PREFIX_CUSTOM_NAMES, playerIds.Count(), targetPlayerId), LogLevel.NORMAL);
			
			// Send to the specific player - we'll use broadcast for now
			// TODO: Implement proper unicast when Arma supports it better
			Rpc(RpcDo_SyncAllCustomNames, playerIds, customNames);
		}
		else
		{
			Print(string.Format("%1 [SYNC] No custom names to sync", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		}
	}

}