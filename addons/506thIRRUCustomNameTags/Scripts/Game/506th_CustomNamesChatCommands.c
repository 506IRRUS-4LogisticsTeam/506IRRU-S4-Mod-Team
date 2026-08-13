//------------------------------------------------------------------------------------------------
//! Custom name chat commands via the game's native command system
//! (SCR_ChatPanelManager invokers): prefixed commands dispatch locally on the
//! typing client and are never transmitted to other players. Bare-word typing
//! (setname/resetname/myname) is intercepted by the chat component only to
//! hint at the prefixed form - it no longer executes.
//------------------------------------------------------------------------------------------------
class IRRU_CustomNamesChatCommands
{
	protected static const int REGISTER_RETRY_MS = 1000;
	protected static const int REGISTER_MAX_TRIES = 30;

	private static ref IRRU_CustomNamesChatCommands s_Instance;

	protected bool m_bRegistered = false;
	protected int m_iTries = 0;

	//------------------------------------------------------------------------------------------------
	static void EnsureRegistered()
	{
		if (!s_Instance)
			s_Instance = new IRRU_CustomNamesChatCommands();

		s_Instance.Register();
	}

	//------------------------------------------------------------------------------------------------
	protected void Register()
	{
		if (m_bRegistered)
			return;

		SCR_ChatPanelManager manager = SCR_ChatPanelManager.GetInstance();
		if (!manager || !GetGame().GetPlayerController())
		{
			m_iTries++;
			if (m_iTries < REGISTER_MAX_TRIES)
				GetGame().GetCallqueue().CallLater(Register, REGISTER_RETRY_MS, false);
			else
				Print("[CustomNames] Chat command registration gave up - prefixed name commands unavailable", LogLevel.WARNING);
			return;
		}

		m_bRegistered = true;
		manager.GetCommandInvoker("setname").Insert(OnSetNameCommand);
		manager.GetCommandInvoker("resetname").Insert(OnResetNameCommand);
		manager.GetCommandInvoker("myname").Insert(OnMyNameCommand);

		Print(string.Format("[CustomNames] Chat commands registered - prefix character is '%1' (e.g. %1setname <YourName>)",
			SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSetNameCommand(SCR_ChatPanel panel, string data)
	{
		string argument = data;
		argument.Trim();

		if (argument.IsEmpty())
		{
			Feedback(string.Format("Usage: %1setname <YourName>", SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER));
			return;
		}

		SendToComponent("setname " + argument);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnResetNameCommand(SCR_ChatPanel panel, string data)
	{
		SendToComponent("resetname");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMyNameCommand(SCR_ChatPanel panel, string data)
	{
		SendToComponent("myname");
	}

	//------------------------------------------------------------------------------------------------
	//! Route through the chat component so the existing local processing,
	//! validation feedback and server RPC are reused unchanged.
	protected void SendToComponent(string commandMsg)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(playerController.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;

		chatComponent.IRRU_ProcessLocalCommand(commandMsg, playerController.GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	protected void Feedback(string message)
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(playerController.FindComponent(SCR_ChatComponent));
		if (chatComponent)
			chatComponent.ShowMessage(message);
	}
}
