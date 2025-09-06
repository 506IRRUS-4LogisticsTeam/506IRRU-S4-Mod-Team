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
					Print(string.Format("%1 [CLIENT->SERVER] ====== COMMAND DETECTED ======", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
					Print(string.Format("%1 [CLIENT->SERVER] Command: '%2'", LOG_PREFIX_CUSTOM_NAMES, msg), LogLevel.NORMAL);
					Print(string.Format("%1 [CLIENT->SERVER] Local Player ID: %2", LOG_PREFIX_CUSTOM_NAMES, localPlayerId), LogLevel.NORMAL);
					Print(string.Format("%1 [CLIENT->SERVER] Is Server: %2", LOG_PREFIX_CUSTOM_NAMES, Replication.IsServer()), LogLevel.NORMAL);
					
					ProcessCustomNameCommand(msg, senderId);
					
					Print(string.Format("%1 [CLIENT->SERVER] SENDING RPC TO SERVER", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
					Rpc(RpcSrv_ProcessCustomNameCommand, msg, senderId);
					Print(string.Format("%1 [CLIENT->SERVER] RPC SENT TO SERVER", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
					Print(string.Format("%1 [CLIENT->SERVER] ==============================", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
					
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
		Print(string.Format("%1 [SERVER-RECEIVE] >>>>>>> SERVER RPC RECEIVED <<<<<<<<", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		Print(string.Format("%1 [SERVER-RECEIVE] Command: '%2'", LOG_PREFIX_CUSTOM_NAMES, msg), LogLevel.NORMAL);
		Print(string.Format("%1 [SERVER-RECEIVE] From Player ID: %2", LOG_PREFIX_CUSTOM_NAMES, senderId), LogLevel.NORMAL);
		Print(string.Format("%1 [SERVER-RECEIVE] Is Server: %2", LOG_PREFIX_CUSTOM_NAMES, Replication.IsServer()), LogLevel.NORMAL);
		Print(string.Format("%1 [SERVER-RECEIVE] Time: %2", LOG_PREFIX_CUSTOM_NAMES, System.GetUnixTime()), LogLevel.NORMAL);
		Print(string.Format("%1 [SERVER-RECEIVE] =======================================", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		
		if (!Replication.IsServer()) 
		{
			Print(string.Format("%1 RpcSrv_ProcessCustomNameCommand called on non-server!", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
			return;
		}
		
		// Delegate to NetworkEntity for proper handling and broadcasting
		CustomNamesNetworkEntity net = CustomNamesNetworkEntity.Get();
		if (net)
		{
			Print(string.Format("%1 [SERVER-PROCESS] Delegating to NetworkEntity", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
			Print(string.Format("%1 [SERVER-PROCESS] NetworkEntity instance: %2", LOG_PREFIX_CUSTOM_NAMES, net), LogLevel.NORMAL);
			net.RpcSrv_ProcessCustomNameCommand(msg, senderId);
			Print(string.Format("%1 [SERVER-PROCESS] NetworkEntity processing complete", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("%1 [SERVER-ERROR] !!! NetworkEntity NOT AVAILABLE !!!", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
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

//------------------------------------------------------------------------------------------------
// STARTUP DEBUG MESSAGE
//------------------------------------------------------------------------------------------------
void CustomNamesChatComponent_Init()
{
	Print("[CustomNames] ################################################", LogLevel.NORMAL);
	Print("[CustomNames] ##  CUSTOM NAMES MOD LOADED - DEBUG ENABLED  ##", LogLevel.NORMAL);
	Print("[CustomNames] ##  Commands: setname, resetname, myname     ##", LogLevel.NORMAL);
	Print("[CustomNames] ##  Watch console for replication flow       ##", LogLevel.NORMAL);
	Print("[CustomNames] ################################################", LogLevel.NORMAL);
}