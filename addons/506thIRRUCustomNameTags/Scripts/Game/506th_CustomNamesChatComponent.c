//------------------------------------------------------------------------------------------------
modded class SCR_ChatComponent : BaseChatComponent
{
	//------------------------------------------------------------------------------------------------
	//! Bare-word name commands no longer execute - by the time this runs the
	//! message has already broadcast to every client, so executing here trained
	//! users into leaking their commands. Swallow locally and teach the prefix.
	override void OnNewMessage(string msg, int channelId, int senderId)
	{
		PlayerController localPlayerController = GetGame().GetPlayerController();
		if (localPlayerController)
		{
			int localPlayerId = localPlayerController.GetPlayerId();

			if (localPlayerId == senderId && senderId > 0 && msg && !msg.IsEmpty())
			{
				if (IRRU_IsCustomNameCommand(msg))
				{
					IRRU_SendChatFeedback(string.Format("Name commands moved: use %1setname <YourName>, %1resetname or %1myname",
						SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER));
					return;
				}
			}
		}

		super.OnNewMessage(msg, channelId, senderId);
	}

	//------------------------------------------------------------------------------------------------
	//! Entry point for the prefixed chat commands: same local processing,
	//! validation feedback and server RPC the bare-word path used to trigger.
	void IRRU_ProcessLocalCommand(string msg, int senderId)
	{
		IRRU_ProcessCustomNameCommand(msg, senderId);
		Rpc(RpcSrv_IRRU_ProcessCustomNameCommand, msg, senderId);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IRRU_IsCustomNameCommand(string msg)
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
	protected void IRRU_ProcessCustomNameCommand(string msg, int playerId)
	{
		string trimmedMsg = msg;
		trimmedMsg.Trim();
		string lowerMsg = trimmedMsg;
		lowerMsg.ToLower();
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
		{
			IRRU_SendChatFeedback("Custom Names system not available");
			Print("[CustomNames] Manager instance not available for command processing", LogLevel.WARNING);
			return;
		}
		
		manager.CheckAndRestoreCustomName(playerId);
		
		if (lowerMsg.StartsWith("setname "))
		{
			string newName = trimmedMsg.Substring(8, trimmedMsg.Length() - 8);
			newName.Trim();
			
			if (newName.IsEmpty())
			{
				IRRU_SendChatFeedback("Usage: setname <YourName>");
				return;
			}
			
			if (manager.ValidateCustomName(newName))
			{
				if (manager.SetCustomName(playerId, newName))
				{
					IRRU_SendChatFeedback(string.Format("Name set to: %1", newName));
				}
				else
				{
					IRRU_SendChatFeedback("Failed to set name");
					Print(string.Format("[CustomNames] Failed to set custom name '%1' for player %2", newName, playerId), LogLevel.WARNING);
				}
			}
			else
			{
				IRRU_SendChatFeedback(string.Format("Invalid name: %1", newName));
			}
		}
		else if (lowerMsg == "resetname")
		{
			if (manager.SetCustomName(playerId, ""))
			{
				IRRU_SendChatFeedback("Name reset to default");
			}
			else
			{
				IRRU_SendChatFeedback("Failed to reset name");
				Print(string.Format("[CustomNames] Failed to reset name for player %1", playerId), LogLevel.WARNING);
			}
		}
		else if (lowerMsg == "myname")
		{
			string currentName = manager.GetCustomName(playerId);
			if (currentName.IsEmpty())
			{
				IRRU_SendChatFeedback("You have no custom name set");
			}
			else
			{
				IRRU_SendChatFeedback(string.Format("Your current name: %1", currentName));
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcSrv_IRRU_ProcessCustomNameCommand(string msg, int senderId)
	{
		if (!Replication.IsServer())
		{
			Print("[CustomNames] RpcSrv_IRRU_ProcessCustomNameCommand called on non-server", LogLevel.WARNING);
			return;
		}
		
		// Delegate to NetworkEntity for proper handling and broadcasting
		CustomNamesNetworkEntity net = CustomNamesNetworkEntity.Get();
		if (net)
		{
			net.RpcSrv_ProcessCustomNameCommand(msg, senderId);
		}
		else
		{
			Print("[CustomNames] NetworkEntity not available for command processing", LogLevel.WARNING);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	static void IRRU_BroadcastCustomNameUpdate(string playerId, string customName)
	{
		if (!Replication.IsServer())
		{
			Print("[CustomNames] IRRU_BroadcastCustomNameUpdate called on client", LogLevel.WARNING);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_SendChatFeedback(string message)
	{
		SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(this);
		if (chatComp)
		{
			chatComp.ShowMessage(message);
		}
	}
}