//------------------------------------------------------------------------------------------------
modded class SCR_ChatComponent : BaseChatComponent
{
	protected const string LOG_PREFIX_CUSTOM_NAMES = "[CustomNames][Chat]";
	
	//------------------------------------------------------------------------------------------------
	override void OnNewMessage(string msg, int channelId, int senderId)
	{
		PlayerController localPlayerController = GetGame().GetPlayerController();
		if (localPlayerController)
		{
			int localPlayerId = localPlayerController.GetPlayerId();
			
			if (localPlayerId == senderId && senderId > 0 && msg && !msg.IsEmpty())
			{
				if (IsCustomNameCommand(msg))
				{
					ProcessCustomNameCommand(msg, senderId);
					Rpc(RpcSrv_ProcessCustomNameCommand, msg, senderId);
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
		Print(string.Format("%1 SERVER RPC: Processing command from sender %2: '%3'", 
			LOG_PREFIX_CUSTOM_NAMES, senderId, msg), LogLevel.NORMAL);
		
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
			Print(string.Format("%1 SERVER: %2 players connected when processing command", 
				LOG_PREFIX_CUSTOM_NAMES, players.Count()), LogLevel.NORMAL);
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
			
			Print(string.Format("%1 SERVER: Broadcasting name change: Player %2 -> '%3'", 
				LOG_PREFIX_CUSTOM_NAMES, senderId, newName), LogLevel.NORMAL);
			BroadcastNameUpdateToAllClients(senderId.ToString(), newName);
			
			Print(string.Format("%1 SERVER: Broadcast complete for Player %2 -> '%3'", 
				LOG_PREFIX_CUSTOM_NAMES, senderId, newName), LogLevel.NORMAL);
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
		Print(string.Format("%1 Broadcasting name update", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		RpcAll_UpdateCustomName(playerId, customName);
		Print(string.Format("%1 RPC sent for Player %2 -> '%3'", 
			LOG_PREFIX_CUSTOM_NAMES, playerId, customName), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_UpdateCustomName(string playerId, string customName)
	{
		PlayerController localPC = GetGame().GetPlayerController();
		int localPlayerId = -1;
		if (localPC)
			localPlayerId = localPC.GetPlayerId();
		
		bool isServer = Replication.IsServer();
		bool isHost = (isServer && localPlayerId > 0);
		
		Print(string.Format("%1 RPC RECEIVED: Player %2 -> '%3' | LocalID: %4, IsServer: %5, IsHost: %6", 
			LOG_PREFIX_CUSTOM_NAMES, playerId, customName, localPlayerId, isServer, isHost), LogLevel.NORMAL);
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			manager.UpdateCustomNameLocal(playerId, customName);
			Print(string.Format("%1 CLIENT: Updated Player %2 -> '%3'", 
				LOG_PREFIX_CUSTOM_NAMES, playerId, customName), LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("%1 CLIENT: No CustomNamesManager available!", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
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