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
					Rpc(RpcSrv_ProcessCustomNameCommand, msg, senderId);
					
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
			
			string playerIdStr = playerId.ToString();
			
			if (manager.ValidateCustomName(newName))
			{
				if (manager.SetCustomName(playerIdStr, newName))
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
			string playerIdStr = playerId.ToString();
			if (manager.SetCustomName(playerIdStr, ""))
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
			string playerIdStr = playerId.ToString();
			string currentName = manager.GetCustomName(playerIdStr);
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
		
		RpcAll_UpdateCustomName(playerId, customName);
		
		Print(string.Format("%1 [DEBUG] RpcAll_UpdateCustomName has been called", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		Print(string.Format("%1 [DEBUG] Broadcast RPC should now be propagating to all clients", 
			LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_UpdateCustomName(string playerId, string customName)
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

}