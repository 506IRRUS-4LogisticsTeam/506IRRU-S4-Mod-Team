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
	protected ref CustomNamesManager m_CustomNamesManager;

	// Optional: tune if you expect many entries (prevents oversized RPCs)
	protected const int MAX_SYNC_ENTRIES_PER_RPC = 200;

	// Called when the GameMode becomes active
	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		s_Instance = this;
		
		// Initialize the CustomNamesManager once and keep it
		if (Replication.IsServer())
		{
			Print("[CustomNames][Network] Server initializing CustomNamesManager", LogLevel.NORMAL);
			m_CustomNamesManager = CustomNamesManager.GetInstance();
			Print(string.Format("[CustomNames][Network] CustomNamesManager instance: %1", m_CustomNamesManager), LogLevel.NORMAL);
		}
		
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
		Print(string.Format("[CustomNames][Network] [SERVER-NET] ======= NETWORK ENTITY PROCESSING =======", senderId), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][Network] [SERVER-NET] Msg: '%1', Sender: %2", msg, senderId), LogLevel.NORMAL);
		
		// Use the stored manager instance
		CustomNamesManager mgr = m_CustomNamesManager;
		if (!mgr)
			mgr = CustomNamesManager.GetInstance();
		if (!mgr) 
		{
			Print("[CustomNames][Network] [SERVER-NET] ERROR: Manager not available!", LogLevel.ERROR);
			return;
		}

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
				if (mgr.SetCustomName(senderId, newName))
				{
					Print(string.Format("[CustomNames][Network] [SERVER-NET] Name validated and set: '%1' for player %2", newName, senderId), LogLevel.NORMAL);
					Print(string.Format("[CustomNames][Network] [SERVER-BROADCAST] >>> BROADCASTING TO ALL CLIENTS <<<", newName), LogLevel.NORMAL);
					Rpc(RpcAll_UpdateCustomName, senderId, newName);
					Print(string.Format("[CustomNames][Network] [SERVER-BROADCAST] Broadcast complete for name '%1'", newName), LogLevel.NORMAL);
				}
				else
				{
					Print(string.Format("[CustomNames][Network] [SERVER-NET] Failed to set name '%1'", newName), LogLevel.ERROR);
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
			if (mgr.SetCustomName(senderId, ""))
			{
				Print(string.Format("[CustomNames][Network] Reset name for player %1", senderId), LogLevel.NORMAL);
				Rpc(RpcAll_UpdateCustomName, senderId, "");
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
	void RpcAll_UpdateCustomName(int playerId, string customName)
	{
		PlayerController localPC = GetGame().GetPlayerController();
		int localPlayerId = -1;
		if (localPC)
			localPlayerId = localPC.GetPlayerId();
		
		Print(string.Format("[CustomNames][Network] [CLIENT-RECEIVE] ##### BROADCAST RECEIVED #####"), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][Network] [CLIENT-RECEIVE] Player %1 -> '%2'", playerId, customName), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][Network] [CLIENT-RECEIVE] Local Player ID: %1", localPlayerId), LogLevel.NORMAL);
		Print(string.Format("[CustomNames][Network] [CLIENT-RECEIVE] Is this update for me? %1", (localPlayerId == playerId)), LogLevel.NORMAL);
		
		CustomNamesManager mgr = CustomNamesManager.GetInstance();
		if (mgr)
		{
			mgr.UpdateCustomNameLocal(playerId, customName);
			Print(string.Format("[CustomNames][Network] [CLIENT-RECEIVE] Local cache updated for player %1", playerId), LogLevel.NORMAL);
			
			// Verify the update worked
			string verifyName = mgr.GetCustomName(playerId);
			if (verifyName == customName)
			{
				Print(string.Format("[CustomNames][Network] [VERIFY] ✅ VERIFICATION SUCCESS! Name correctly stored as '%1'", verifyName), LogLevel.NORMAL);
			}
			else
			{
				Print(string.Format("[CustomNames][Network] [VERIFY] ❌ VERIFICATION FAILED! Expected '%1' but got '%2'", customName, verifyName), LogLevel.ERROR);
			}
		}
		else
		{
			Print("[CustomNames][Network] [CLIENT-RECEIVE] ERROR: Manager not available!", LogLevel.ERROR);
		}
	}

	//--------------------------------------------------------------------------------------------
	// SERVER -> ALL CLIENTS: send custom names (arrays to avoid map replication)
	// Note: Broadcasting to all is simpler and more reliable than unicast in current Arma
	//--------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAll_ReceiveAllCustomNames(array<int> playerIds, array<string> names)
	{
		if (!playerIds || !names) return;

		CustomNamesManager mgr = CustomNamesManager.GetInstance();
		if (!mgr) return;

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

	//--------------------------------------------------------------------------------------------
	// SERVER helper: unicast all names to a specific player, in chunks
	//--------------------------------------------------------------------------------------------
	void SendAllCustomNamesToClient(int playerId)
	{
		if (!Replication.IsServer()) return;

		// Use the stored manager instance
		CustomNamesManager mgr = m_CustomNamesManager;
		if (!mgr)
			mgr = CustomNamesManager.GetInstance();
		if (!mgr) return;

		// Get all currently connected players and their custom names
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager) return;
		
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
				
				// Chunk if necessary
				if (playerIds.Count() >= MAX_SYNC_ENTRIES_PER_RPC)
				{
					// Broadcast to all - simpler and more reliable
					Rpc(RpcAll_ReceiveAllCustomNames, playerIds, names);
					playerIds.Clear();
					names.Clear();
				}
			}
		}

		// Send any remainder
		if (playerIds.Count() > 0)
		{
			// For now, broadcast to all clients - they'll filter locally
			// TODO: Implement proper unicast when Arma supports it better
			Rpc(RpcAll_ReceiveAllCustomNames, playerIds, names);
		}
	}
	
	//--------------------------------------------------------------------------------------------
	// Handle player connection with identity retry
	//--------------------------------------------------------------------------------------------
	void OnPlayerConnectedWithRetry(int playerId, int attemptNumber = 0)
	{
		Print(string.Format("[CustomNames][Network] Attempt %1 to handle player %2 connection", attemptNumber + 1, playerId), LogLevel.NORMAL);
		
		if (!Replication.IsServer()) return;
		
		// Use the stored manager instance
		CustomNamesManager mgr = m_CustomNamesManager;
		if (!mgr)
			mgr = CustomNamesManager.GetInstance();
		if (!mgr) return;
		
		// Try to get the real identity ID (not fallback)
		string identityId = "";
		BackendApi backendApi = GetGame().GetBackendApi();
		if (backendApi)
		{
			identityId = backendApi.GetPlayerIdentityId(playerId);
			Print(string.Format("[CustomNames][Network] GetPlayerIdentityId returned: '%1'", identityId), LogLevel.NORMAL);
		}
		
		if (!identityId.IsEmpty())
		{
			// Success - we got the real identity
			Print(string.Format("[CustomNames][Network] SUCCESS: Got identity '%1' for player %2", identityId, playerId), LogLevel.NORMAL);
			
			// First send all existing custom names to the new player
			GetGame().GetCallqueue().CallLater(SendAllCustomNamesToClient, 500, false, playerId);
			
			// Then check if this player has their own custom name to restore
			string customName = mgr.GetCustomNameByUID(identityId);
			if (!customName.IsEmpty())
			{
				Print(string.Format("[CustomNames][Network] *** RESTORING PERSISTED NAME ***", customName), LogLevel.NORMAL);
				Print(string.Format("[CustomNames][Network] Player %1 had custom name '%2' in persistence file", playerId, customName), LogLevel.NORMAL);
				
				// Update the manager's local cache with the restored name
				mgr.UpdateCustomNameLocal(playerId, customName);
				
				// Broadcast their restored name to all clients
				Print(string.Format("[CustomNames][Network] Broadcasting restored name to all clients", customName), LogLevel.NORMAL);
				GetGame().GetCallqueue().CallLater(BroadcastNameDelayed, 1000, false, playerId, customName);
				
				// Send chat notification to the player
				SendNameRestorationNotification(playerId, customName);
			}
			else
			{
				Print(string.Format("[CustomNames][Network] No persisted custom name found for identity %1", identityId), LogLevel.NORMAL);
				
				// Send chat notification that no name was found
				SendNameRestorationNotification(playerId, "");
			}
		}
		else if (attemptNumber < 4)
		{
			// Identity not ready yet, schedule retry
			int delays[5] = {0, 100, 250, 500, 1000};
			int nextDelay = delays[attemptNumber + 1];
			
			Print(string.Format("[CustomNames][Network] Identity not ready, scheduling retry %1 in %2ms", 
				attemptNumber + 2, nextDelay), LogLevel.NORMAL);
			
			GetGame().GetCallqueue().CallLater(OnPlayerConnectedWithRetry, nextDelay, false, 
				playerId, attemptNumber + 1);
		}
		else
		{
			// All retries exhausted - just send existing names without restore
			Print(string.Format("[CustomNames][Network] Failed to get identity after %1 attempts, sending names anyway", 
				attemptNumber + 1), LogLevel.WARNING);
			SendAllCustomNamesToClient(playerId);
		}
	}
	
	void BroadcastNameDelayed(int playerId, string customName)
	{
		Rpc(RpcAll_UpdateCustomName, playerId, customName);
	}
	
	//--------------------------------------------------------------------------------------------
	// Send chat notification to a specific player about their custom name status
	//--------------------------------------------------------------------------------------------
	void SendNameRestorationNotification(int playerId, string customName)
	{
		if (!customName.IsEmpty())
		{
			// Name was restored
			string message = string.Format("Your custom name '%1' has been restored", customName);
			GetGame().GetCallqueue().CallLater(SendDelayedChatNotification, 5000, false, playerId, message);
		}
		else
		{
			// No custom name found
			string message = "No custom name found. Use 'setname <YourName>' to set one";
			GetGame().GetCallqueue().CallLater(SendDelayedChatNotification, 5000, false, playerId, message);
		}
	}
	
	void SendDelayedChatNotification(int playerId, string message)
	{
		// Send to specific player
		Rpc(RpcDo_ShowChatNotification, message, playerId);
	}
	
	//--------------------------------------------------------------------------------------------
	// CLIENT: Receive and show chat notification
	//--------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_ShowChatNotification(string message, int targetPlayerId)
	{
		PlayerController localPC = GetGame().GetPlayerController();
		if (!localPC) return;
		
		int localPlayerId = localPC.GetPlayerId();
		if (localPlayerId != targetPlayerId) return;
		
		// Show the message to the player
		SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(localPC.FindComponent(SCR_ChatComponent));
		if (chatComp)
		{
			chatComp.ShowMessage(message);
		}
		
		Print(string.Format("[CustomNames][Network] Chat notification shown: %1", message), LogLevel.NORMAL);
	}
}

