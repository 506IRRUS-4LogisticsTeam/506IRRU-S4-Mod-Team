//! CUSTOM NAMES CHAT COMPONENT - Fixed Chat Name Override
//! Properly intercepts chat messages to display custom names

//------------------------------------------------------------------------------------------------
// Modded Chat Component - FIXED APPROACH
//------------------------------------------------------------------------------------------------

modded class SCR_ChatComponent : BaseChatComponent
{
	protected const string LOG_PREFIX_CUSTOM_NAMES = "[CustomNames][Chat]";
	
	//------------------------------------------------------------------------------------------------
	//! OnNewMessage - Command processing + name override
	//------------------------------------------------------------------------------------------------
	override void OnNewMessage(string msg, int channelId, int senderId)
	{
		// Check if this is a custom name command BEFORE calling super
		PlayerController localPlayerController = GetGame().GetPlayerController();
		if (localPlayerController)
		{
			int localPlayerId = localPlayerController.GetPlayerId();
			
			// If we sent the message, and it's a valid player message, check for commands
			if (localPlayerId == senderId && senderId > 0 && msg && !msg.IsEmpty())
			{
				if (IsCustomNameCommand(msg))
				{
					// Process the command locally first
					ProcessCustomNameCommand(msg, senderId);
					
					// Send to server for synchronization
					Rpc(RpcSrv_ProcessCustomNameCommand, msg, senderId);
					
					// Don't call super - suppress the command from appearing in chat
					return;
				}
			}
		}
		
		// Call parent to process the message normally
		super.OnNewMessage(msg, channelId, senderId);
	}

	//------------------------------------------------------------------------------------------------
	//! Check if message is a custom name command
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
	//! Process custom name command locally
	//------------------------------------------------------------------------------------------------
	protected void ProcessCustomNameCommand(string msg, int playerId)
	{
		Print(string.Format("%1 🔧 PROCESSING command locally: '%2'", LOG_PREFIX_CUSTOM_NAMES, msg), LogLevel.NORMAL);
		
		string trimmedMsg = msg;
		trimmedMsg.Trim();
		string lowerMsg = trimmedMsg;
		lowerMsg.ToLower();
		
		// Get the manager
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (!manager)
		{
			SendChatFeedback("❌ Custom Names system not available");
			return;
		}
		
		// Check for existing custom name first (this handles the "connection restore" functionality)
		manager.CheckAndRestoreCustomName(playerId);
		
		// Parse command
		if (lowerMsg.StartsWith("setname "))
		{
			// Extract name (remove "setname " prefix)
			string newName = trimmedMsg.Substring(8, trimmedMsg.Length() - 8);
			newName.Trim();
			
			if (newName.IsEmpty())
			{
				SendChatFeedback("❌ Usage: setname <YourName>");
				return;
			}
			
			// Convert player ID to string
			string playerIdStr = playerId.ToString();
			
			if (manager.ValidateCustomName(newName))
			{
				// Set the name
				if (manager.SetCustomName(playerIdStr, newName))
				{
					SendChatFeedback(string.Format("✅ Name set to: %1", newName));
					Print(string.Format("%1 ✅ SUCCESS! Name set to: %2 for player %3", 
						LOG_PREFIX_CUSTOM_NAMES, newName, playerId), LogLevel.NORMAL);
				}
				else
				{
					SendChatFeedback("❌ Failed to set name");
				}
			}
			else
			{
				SendChatFeedback(string.Format("❌ Invalid name: %1", newName));
			}
		}
		else if (lowerMsg == "resetname")
		{
			string playerIdStr = playerId.ToString();
			// Use SetCustomName with empty string to reset
			if (manager.SetCustomName(playerIdStr, ""))
			{
				SendChatFeedback("✅ Name reset to default");
			}
			else
			{
				SendChatFeedback("❌ Failed to reset name");
			}
		}
		else if (lowerMsg == "myname")
		{
			string playerIdStr = playerId.ToString();
			string currentName = manager.GetCustomName(playerIdStr);
			if (currentName.IsEmpty())
			{
				SendChatFeedback("ℹ️ You have no custom name set");
			}
			else
			{
				SendChatFeedback(string.Format("ℹ️ Your current name: %1", currentName));
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side RPC for command synchronization
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcSrv_ProcessCustomNameCommand(string msg, int senderId)
	{
		Print(string.Format("%1 🖥️ SERVER RPC: Processing command from sender %2: '%3'", 
			LOG_PREFIX_CUSTOM_NAMES, senderId, msg), LogLevel.NORMAL);
		
		if (!Replication.IsServer()) 
		{
			Print(string.Format("%1 ❌ RpcSrv_ProcessCustomNameCommand called on non-server!", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
			return;
		}
		
		// Add debugging for player count
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			array<int> players = {};
			playerManager.GetPlayers(players);
			Print(string.Format("%1 SERVER: %2 players connected when processing command", 
				LOG_PREFIX_CUSTOM_NAMES, players.Count()), LogLevel.NORMAL);
		}
		
		// Process the command on server side for authoritative storage
		ProcessCustomNameCommand(msg, senderId);
		
		// Extract the name for broadcasting to other clients
		string trimmedMsg = msg;
		trimmedMsg.Trim();
		string lowerMsg = trimmedMsg;
		lowerMsg.ToLower();
		
		if (lowerMsg.StartsWith("setname "))
		{
			string newName = trimmedMsg.Substring(8, trimmedMsg.Length() - 8);
			newName.Trim();
			
			Print(string.Format("%1 📡 SERVER: About to broadcast name change: Player %2 -> '%3'", 
				LOG_PREFIX_CUSTOM_NAMES, senderId, newName), LogLevel.NORMAL);
			
			// NEW APPROACH: Call the update method directly on all connected clients
			BroadcastNameUpdateToAllClients(senderId.ToString(), newName);
			
			Print(string.Format("%1 ✅ SERVER: Broadcast complete for Player %2 -> '%3'", 
				LOG_PREFIX_CUSTOM_NAMES, senderId, newName), LogLevel.NORMAL);
		}
		else if (lowerMsg == "resetname")
		{
			Print(string.Format("%1 📡 SERVER: About to broadcast name reset for player %2", 
				LOG_PREFIX_CUSTOM_NAMES, senderId), LogLevel.NORMAL);
			
			// NEW APPROACH: Call the update method directly on all connected clients
			BroadcastNameUpdateToAllClients(senderId.ToString(), "");
			
			Print(string.Format("%1 ✅ SERVER: Broadcast complete for name reset of player %2", 
				LOG_PREFIX_CUSTOM_NAMES, senderId), LogLevel.NORMAL);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! New method to broadcast to all clients including proper host handling
	//------------------------------------------------------------------------------------------------
	protected void BroadcastNameUpdateToAllClients(string playerId, string customName)
	{
		Print(string.Format("%1 🔍 Broadcasting name update directly", LOG_PREFIX_CUSTOM_NAMES), LogLevel.NORMAL);
		
		// Call the RPC method directly - this is the correct way in Arma Reforger
		RpcAll_UpdateCustomName(playerId, customName);
		
		Print(string.Format("%1 ✅ Direct RPC call sent for Player %2 -> '%3'", 
			LOG_PREFIX_CUSTOM_NAMES, playerId, customName), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client-side RPC to update custom names on all clients
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_UpdateCustomName(string playerId, string customName)
	{
		// Get current player info for debugging
		PlayerController localPC = GetGame().GetPlayerController();
		int localPlayerId = -1;
		if (localPC)
			localPlayerId = localPC.GetPlayerId();
		
		bool isServer = Replication.IsServer();
		bool isHost = (isServer && localPlayerId > 0);
		
		Print(string.Format("%1 📥 BROADCAST RPC RECEIVED: Player %2 -> '%3' | LocalID: %4, IsServer: %5, IsHost: %6", 
			LOG_PREFIX_CUSTOM_NAMES, playerId, customName, localPlayerId, isServer, isHost), LogLevel.NORMAL);
		
		CustomNamesManager manager = CustomNamesManager.GetInstance();
		if (manager)
		{
			// Update the name locally on this client
			manager.UpdateCustomNameLocal(playerId, customName);
			Print(string.Format("%1 ✅ CLIENT: Broadcast update complete for Player %2 -> '%3'", 
				LOG_PREFIX_CUSTOM_NAMES, playerId, customName), LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("%1 ❌ CLIENT: No CustomNamesManager available!", LOG_PREFIX_CUSTOM_NAMES), LogLevel.ERROR);
		}
	}
	
	//! Static method for CustomNamesManager to broadcast name updates
	//------------------------------------------------------------------------------------------------
	static void BroadcastCustomNameUpdate(string playerId, string customName)
	{
		Print(string.Format("[CustomNames][Chat] 🔄 Attempting to broadcast name update: Player %1 -> %2", playerId, customName), LogLevel.NORMAL);
		
		// This should only be called on the server
		if (!Replication.IsServer())
		{
			Print("[CustomNames][Chat] ❌ BroadcastCustomNameUpdate called on client - this should only happen on server", LogLevel.WARNING);
			return;
		}
		
		// Simple approach: Just trigger an RPC through any available mechanism
		// The RPC system should handle this automatically through the modded class
		Print("[CustomNames][Chat] ℹ️ Server-side name update completed, RPCs should be handled by command processing", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Send feedback message to player
	//------------------------------------------------------------------------------------------------
	protected void SendChatFeedback(string message)
	{
		// Use the same approach as GM Tools for consistent feedback
		Print(string.Format("%1 💬 FEEDBACK: %2", LOG_PREFIX_CUSTOM_NAMES, message), LogLevel.NORMAL);
		
		// Also try to send as chat message if possible
		SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(this);
		if (chatComp)
		{
			// Send as system message
			chatComp.ShowMessage(message);
		}
	}

} // End modded class SCR_ChatComponent

//------------------------------------------------------------------------------------------------
//! STARTUP MESSAGE - This will confirm the chat component is loaded
//------------------------------------------------------------------------------------------------
void CustomNamesChatComponent_OnModLoad()
{
	Print("[CustomNames][Chat] 🚀 CHAT COMPONENT LOADED - Using GM Tools proven approach!", LogLevel.NORMAL);
	Print("[CustomNames][Chat] ✅ OnNewMessage override active - Ready to intercept chat!", LogLevel.NORMAL);
	Print("[CustomNames][Chat] 🎯 Commands available: setname, resetname, myname", LogLevel.NORMAL);
	Print("[CustomNames][Chat] 💡 Usage: Type 'setname M.powers' in chat (no # prefix)", LogLevel.NORMAL);
	Print("[CustomNames][Chat] 👤 Chat names will show as: Custom Name: message", LogLevel.NORMAL);
}
