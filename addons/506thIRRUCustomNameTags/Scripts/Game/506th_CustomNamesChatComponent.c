//------------------------------------------------------------------------------------------------
modded class SCR_ChatComponent : BaseChatComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		IRRU_CustomNamesChatCommands.EnsureRegistered();
	}

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
				if (IsCustomNameCommand(msg))
				{
					SendChatFeedback(string.Format("Name commands moved: use %1setname <YourName>, %1resetname or %1myname - with the %1 prefix they stay out of everyone's chat",
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
		ProcessCustomNameCommand(msg, senderId);
		Rpc(RpcSrv_ProcessCustomNameCommand, msg, senderId);
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
		string trimmedMsg = msg;
		trimmedMsg.Trim();
		string lowerMsg = trimmedMsg;
		lowerMsg.ToLower();
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
		{
			SendChatFeedback("Custom Names system not available");
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
				SendChatFeedback("Usage: setname <YourName>");
				return;
			}
			
			if (manager.ValidateCustomName(newName))
			{
				if (manager.SetCustomName(playerId, newName))
				{
					SendChatFeedback(string.Format("Name set to: %1", newName));
				}
				else
				{
					SendChatFeedback("Failed to set name");
					Print(string.Format("[CustomNames] Failed to set custom name '%1' for player %2", newName, playerId), LogLevel.WARNING);
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
				Print(string.Format("[CustomNames] Failed to reset name for player %1", playerId), LogLevel.WARNING);
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
		if (!Replication.IsServer()) 
		{
			Print("[CustomNames] RpcSrv_ProcessCustomNameCommand called on non-server", LogLevel.WARNING);
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
	static void BroadcastCustomNameUpdate(string playerId, string customName)
	{
		if (!Replication.IsServer())
		{
			Print("[CustomNames] BroadcastCustomNameUpdate called on client", LogLevel.WARNING);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SendChatFeedback(string message)
	{
		SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(this);
		if (chatComp)
		{
			chatComp.ShowMessage(message);
		}
	}
}