//! Enhanced Radio chat commands via the game's native command system
//! (SCR_ChatPanelManager invokers): prefixed commands dispatch locally on the
//! typing client and are NEVER transmitted to other players. Bare unprefixed
//! words are deliberately NOT intercepted - they broadcast like any chat
//! message, so the commands only exist behind the prefix. The registration
//! log prints the prefix character.
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
            else
                Print("[EnhancedRadio] Chat command registration gave up - radio commands unavailable this session", LogLevel.WARNING);
            return;
        }

        m_bRegistered = true;
        manager.GetCommandInvoker("radiobeeps").Insert(OnRadioBeepsCommand);
        manager.GetCommandInvoker("radiocheck").Insert(OnRadioCheckCommand);
        manager.GetCommandInvoker("radiosettings").Insert(OnRadioSettingsCommand);

        Print(string.Format("[EnhancedRadio] Chat commands registered - prefix character is '%1' (e.g. %1radiosettings)",
            SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER));
    }

    //------------------------------------------------------------------------------------------------
    protected void OnRadioBeepsCommand(SCR_ChatPanel panel, string data)
    {
        HandleToggle(data, true);
    }

    //------------------------------------------------------------------------------------------------
    protected void OnRadioCheckCommand(SCR_ChatPanel panel, string data)
    {
        HandleToggle(data, false);
    }

    //------------------------------------------------------------------------------------------------
    protected void HandleToggle(string data, bool rxBeeps)
    {
        string argument = data;
        argument.Trim();
        argument.ToLower();

        string command = "radiocheck";
        string label = "spawn radio check";
        if (rxBeeps)
        {
            command = "radiobeeps";
            label = "incoming squelch/beeps";
        }

        if (argument != "on" && argument != "off")
        {
            Feedback(string.Format("Usage: %1%2 on|off", SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER, command));
            return;
        }

        bool enabled = argument == "on";
        IRRU_RadioUserSettings settings = IRRU_RadioUserSettings.GetInstance();
        if (rxBeeps)
            settings.SetRxBeepsEnabled(enabled);
        else
            settings.SetRadioCheckEnabled(enabled);

        Feedback(string.Format("Enhanced Radio: %1 %2", label, OnOffText(enabled)));
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
