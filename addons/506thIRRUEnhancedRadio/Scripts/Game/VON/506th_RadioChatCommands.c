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
        manager.GetCommandInvoker("radiofill").Insert(OnRadioFillCommand);
        manager.GetCommandInvoker("radiocrypto").Insert(OnRadioCryptoCommand);

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

        if (!IRRU_RFPropagationNetworkComponent.IsEncryptionEnabled())
            return;

        array<int> frequencies = {};
        settings.GetFillFrequencies(frequencies);
        if (frequencies.IsEmpty())
        {
            Feedback("COMSEC: no fills loaded (all channels plaintext)");
            return;
        }

        SCR_IRRURadioEarSettings earSettings = SCR_IRRURadioEarSettings.GetInstance();
        string fillList = "";
        foreach (int frequency : frequencies)
        {
            if (!fillList.IsEmpty())
                fillList = fillList + ", ";

            fillList = fillList + string.Format("%1 MHz [%2]", FormatFrequency(frequency), earSettings.GetFillDisplayText(frequency));
        }

        Feedback(string.Format("COMSEC fills: %1", fillList));
    }

    //------------------------------------------------------------------------------------------------
    //! radiofill <MHz> <digits|0> - set or clear a channel fill without the
    //! radial (also the keyboard-only fallback path)
    protected void OnRadioFillCommand(SCR_ChatPanel panel, string data)
    {
        if (!IRRU_RFPropagationNetworkComponent.IsEncryptionEnabled())
        {
            Feedback("COMSEC: encryption is not enabled on this server");
            return;
        }

        array<string> tokens = {};
        data.Split(" ", tokens, true);
        if (tokens.Count() != 2)
        {
            Feedback(string.Format("Usage: %1radiofill <MHz> <digits|0>   e.g. %1radiofill 38.0 738291", SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER));
            return;
        }

        int frequency = Math.Round(tokens[0].ToFloat() * 1000);
        if (frequency <= 0)
        {
            Feedback("COMSEC: invalid frequency");
            return;
        }

        SCR_IRRURadioEarSettings earSettings = SCR_IRRURadioEarSettings.GetInstance();
        string fill = tokens[1];

        if (fill == "0")
        {
            earSettings.ClearFill(frequency);
            Feedback(string.Format("COMSEC %1 MHz: FILL CLEARED - PLAINTEXT", FormatFrequency(frequency)));
            return;
        }

        if (!SCR_IRRURadioEarSettings.IRRU_IsDigits(fill) || fill.Length() > 8)
        {
            Feedback("COMSEC: fill must be 1 to 8 digits (0 clears)");
            return;
        }

        earSettings.SetFill(frequency, fill);
        Feedback(string.Format("COMSEC %1 MHz: FILL SET [%2]", FormatFrequency(frequency), earSettings.GetFillDisplayText(frequency)));

        // Fills are net-scoped, so pre-staging one for a frequency you are
        // not tuned to is legitimate - but warn, because a typo'd frequency
        // fails silently otherwise (the fill just never rides a key-up).
        PlayerController playerController = GetGame().GetPlayerController();
        if (playerController)
        {
            SCR_VONController vonController = SCR_VONController.Cast(playerController.FindComponent(SCR_VONController));
            if (vonController && !vonController.IRRU_FindEntryByFrequency(frequency))
                Feedback(string.Format("COMSEC note: no radio currently tuned to %1 MHz", FormatFrequency(frequency)));
        }
    }

    //------------------------------------------------------------------------------------------------
    //! radiocrypto on|off|status - runtime master switch (server verifies the
    //! requester holds full GM rights); status is available to everyone
    protected void OnRadioCryptoCommand(SCR_ChatPanel panel, string data)
    {
        string argument = data;
        argument.TrimInPlace();
        argument.ToLower();

        if (argument == "status")
        {
            Feedback(string.Format("COMSEC: encryption is %1 on this server", OnOffText(IRRU_RFPropagationNetworkComponent.IsEncryptionEnabled())));
            return;
        }

        if (argument != "on" && argument != "off")
        {
            Feedback(string.Format("Usage: %1radiocrypto on|off|status (on/off is GM-only)", SCR_ChatPanelManager.CHAT_COMMAND_CHARACTER));
            return;
        }

        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
            return;

        SCR_VONController vonController = SCR_VONController.Cast(playerController.FindComponent(SCR_VONController));
        if (!vonController)
            return;

        vonController.IRRU_RequestEncryptionToggle(argument == "on");
        Feedback("COMSEC: request sent (applies only if you have GM rights; everyone gets a chat notice on change)");
    }

    //------------------------------------------------------------------------------------------------
    protected string FormatFrequency(int freqKHz)
    {
        int wholeMHz = freqKHz / 1000;
        int decimalKHz = (freqKHz % 1000) / 100;

        return string.Format("%1.%2", wholeMHz, decimalKHz);
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
