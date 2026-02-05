//! Chat command handler for ticket system admin commands.
//! Commands (Ares/Zeus only):
//!   ticketreset - Reset tickets to 0
//!   ticketset X - Set tickets to X

modded class SCR_ChatComponent : BaseChatComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnNewMessage(string msg, int channelId, int senderId)
	{
		PlayerController localPlayerController = GetGame().GetPlayerController();
		if (localPlayerController)
		{
			int localPlayerId = localPlayerController.GetPlayerId();

			if (localPlayerId == senderId && senderId > 0 && msg && !msg.IsEmpty())
			{
				if (IsTicketCommand(msg))
				{
					ProcessTicketCommand(msg, senderId);
					return;
				}
			}
		}

		super.OnNewMessage(msg, channelId, senderId);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsTicketCommand(string msg)
	{
		if (!msg || msg.IsEmpty())
			return false;

		string trimmedMsg = msg;
		trimmedMsg.Trim();
		trimmedMsg.ToLower();

		return (trimmedMsg == "ticketreset" || trimmedMsg.StartsWith("ticketset "));
	}

	//------------------------------------------------------------------------------------------------
	protected void ProcessTicketCommand(string msg, int playerId)
	{
		// Check if player has editor (Ares/Zeus) access
		if (!HasEditorAccess(playerId))
		{
			SendTicketChatFeedback("You do not have permission to use ticket commands");
			return;
		}

		string trimmedMsg = msg;
		trimmedMsg.Trim();
		string lowerMsg = trimmedMsg;
		lowerMsg.ToLower();

		if (lowerMsg == "ticketreset")
		{
			Rpc(RpcSrv_TicketReset, playerId);
			SendTicketChatFeedback("Ticket reset command sent");
		}
		else if (lowerMsg.StartsWith("ticketset "))
		{
			string valueStr = trimmedMsg.Substring(10, trimmedMsg.Length() - 10);
			valueStr.Trim();

			int value = valueStr.ToInt();
			if (value < 0)
			{
				SendTicketChatFeedback("Invalid value. Usage: ticketset <number>");
				return;
			}

			Rpc(RpcSrv_TicketSet, playerId, value);
			SendTicketChatFeedback(string.Format("Ticket set command sent: %1", value));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasEditorAccess(int playerId)
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity editorManager = core.GetEditorManager();
		if (!editorManager)
			return false;

		// Check if player has editor access (is a Game Master)
		return editorManager.IsLimited() == false;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcSrv_TicketReset(int senderId)
	{
		if (!Replication.IsServer())
			return;

		// Verify sender has editor access on server side
		if (!ServerVerifyEditorAccess(senderId))
		{
			Print(string.Format("[TicketSystem] Player %1 attempted ticketreset without permission", senderId), LogLevel.WARNING);
			return;
		}

		IRRU_TicketSystemGameModeComponent ticketSystem = IRRU_TicketSystemGameModeComponent.GetInstance();
		if (ticketSystem)
		{
			string playerName = GetPlayerName(senderId);
			ticketSystem.ResetTickets(string.Format("reset by %1", playerName));
			Print(string.Format("[TicketSystem] Tickets reset by %1", playerName));
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcSrv_TicketSet(int senderId, int value)
	{
		if (!Replication.IsServer())
			return;

		// Verify sender has editor access on server side
		if (!ServerVerifyEditorAccess(senderId))
		{
			Print(string.Format("[TicketSystem] Player %1 attempted ticketset without permission", senderId), LogLevel.WARNING);
			return;
		}

		IRRU_TicketSystemGameModeComponent ticketSystem = IRRU_TicketSystemGameModeComponent.GetInstance();
		if (ticketSystem)
		{
			string playerName = GetPlayerName(senderId);
			ticketSystem.SetTickets(value, string.Format("set by %1", playerName));
			Print(string.Format("[TicketSystem] Tickets set to %1 by %2", value, playerName));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool ServerVerifyEditorAccess(int playerId)
	{
		// On server, check if player is a Game Master
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		// Get the editor manager for the specific player
		array<SCR_EditorManagerEntity> editors = {};
		core.GetEditorEntities(editors);

		foreach (SCR_EditorManagerEntity editor : editors)
		{
			if (editor && editor.GetPlayerID() == playerId)
				return editor.IsLimited() == false;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetPlayerName(int playerId)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			string name = pm.GetPlayerName(playerId);
			if (!name.IsEmpty())
				return name;
		}
		return string.Format("Player%1", playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendTicketChatFeedback(string message)
	{
		SCR_ChatComponent chatComp = SCR_ChatComponent.Cast(this);
		if (chatComp)
			chatComp.ShowMessage(string.Format("[Tickets] %1", message));
	}
}
