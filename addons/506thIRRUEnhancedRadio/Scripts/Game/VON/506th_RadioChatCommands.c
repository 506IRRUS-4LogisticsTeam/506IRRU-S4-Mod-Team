//! Registers Enhanced Radio chat commands with the game's native chat command
//! system (SCR_ChatPanelManager). Prefixed commands are dispatched locally on
//! the typing client and never transmitted - unlike plain chat messages, which
//! broadcast to everyone before any display-side interception can hide them.
class IRRU_RadioChatCommands
{
    protected static const int REGISTER_RETRY_MS = 1000;
    protected static const int REGISTER_MAX_TRIES = 30;

    private static ref IRRU_RadioChatCommands s_Instance;

    protected bool m_bRegistered = false;
    protected int m_iTries = 0;

    //------------------------------------------------------------------------------------------------
    static void EnsureRegistered()
    {
        if (!s_Instance)
            s_Instance = new IRRU_RadioChatCommands();

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
            return;
        }

        m_bRegistered = true;
        manager.GetCommandInvoker("radiobeeps").Insert(OnRadioBeepsCommand);
        manager.GetCommandInvoker("radiocheck").Insert(OnRadioCheckCommand);
        manager.GetCommandInvoker("radiosettings").Insert(OnRadioSettingsCommand);
    }

    //------------------------------------------------------------------------------------------------
    protected void OnRadioBeepsCommand(SCR_ChatPanel panel, string data)
    {
        string argument = data;
        argument.Trim();
        argument.ToLower();

        IRRU_RadioUserSettings settings = IRRU_RadioUserSettings.GetInstance();

        if (argument == "on")
        {
            settings.SetRxBeepsEnabled(true);
            Feedback("Enhanced Radio: incoming squelch/beeps ON");
        }
        else if (argument == "off")
        {
            settings.SetRxBeepsEnabled(false);
            Feedback("Enhanced Radio: incoming squelch/beeps OFF");
        }
        else
        {
            Feedback(string.Format("Usage: %1radiobeeps on|off", SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER));
        }
    }

    //------------------------------------------------------------------------------------------------
    protected void OnRadioCheckCommand(SCR_ChatPanel panel, string data)
    {
        string argument = data;
        argument.Trim();
        argument.ToLower();

        IRRU_RadioUserSettings settings = IRRU_RadioUserSettings.GetInstance();

        if (argument == "on")
        {
            settings.SetRadioCheckEnabled(true);
            Feedback("Enhanced Radio: spawn radio check ON");
        }
        else if (argument == "off")
        {
            settings.SetRadioCheckEnabled(false);
            Feedback("Enhanced Radio: spawn radio check OFF");
        }
        else
        {
            Feedback(string.Format("Usage: %1radiocheck on|off", SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER));
        }
    }

    //------------------------------------------------------------------------------------------------
    protected void OnRadioSettingsCommand(SCR_ChatPanel panel, string data)
    {
        IRRU_RadioUserSettings settings = IRRU_RadioUserSettings.GetInstance();
        Feedback(string.Format("Enhanced Radio: incoming squelch/beeps %1, spawn radio check %2",
            OnOffText(settings.AreRxBeepsEnabled()), OnOffText(settings.IsRadioCheckEnabled())));
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

    //------------------------------------------------------------------------------------------------
    protected string OnOffText(bool value)
    {
        if (value)
            return "ON";

        return "OFF";
    }
}
