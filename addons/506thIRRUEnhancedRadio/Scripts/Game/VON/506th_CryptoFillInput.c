//! Crypto fill entry modal. Reuses the frequency-input layout (same widget
//! set, different texts) so no new UI resource is needed.
//!
//! Commit rules differ from IRRU_FrequencyInput on purpose: there, leaving
//! write mode commits, which is safe because empty input is rejected and the
//! prefill equals the current value. Here "clear" is a real, destructive
//! outcome, so ONLY a confirmed entry commits: the edit box handler flags
//! OnChange(finished=true) (fires when editing is confirmed), and Poll()
//! resolves when write mode ends - confirm seen -> commit, otherwise cancel
//! with no state change (Esc, focus loss, radial closing). Clearing is
//! explicit: type 0 and confirm. Set and clear produce distinct feedback.
class IRRU_CryptoFillEditHandler : ScriptedWidgetEventHandler
{
    protected IRRU_CryptoFillInput m_Owner;

    void SetOwner(IRRU_CryptoFillInput owner)
    {
        m_Owner = owner;
    }

    override bool OnChange(Widget w, bool finished)
    {
        if (finished && m_Owner)
            m_Owner.OnEditConfirmed();

        return false;
    }
}

//------------------------------------------------------------------------------------------------
class IRRU_CryptoFillInput
{
    protected static const ResourceName LAYOUT_PATH = "{EBBA4E985FE4D2DE}UI/layouts/IRRU_FrequencyInput.layout";
    protected static const int MAX_FILL_LENGTH = 8;

    // Self-contained sound refs (same wavs the VON controller uses)
    protected static const string SOUND_SET = "{8382F79979658ABA}Sounds/VON/GL_Sounds/RadioCycle.wav";
    protected static const string SOUND_CLEAR = "{F60574D50A8FA527}Sounds/VON/GL_Sounds/RadioLocalOff.wav";
    protected static const string SOUND_ERROR = "{BB24E9E96BDD524F}Sounds/IRRU_Sound/errorbeep.wav";

    protected BaseTransceiver m_Transceiver;
    protected SCR_VONEntryRadio m_RadioEntry;

    protected Widget m_wRoot;
    protected EditBoxWidget m_wFillEdit;
    protected ref IRRU_CryptoFillEditHandler m_EditHandler;

    protected bool m_bIsOpen;
    protected bool m_bConfirmSeen;
    protected string m_sOriginalFill;

    //------------------------------------------------------------------------------------------------
    void Open(BaseTransceiver transceiver, SCR_VONEntryRadio radioEntry)
    {
        if (m_bIsOpen)
            return;

        m_Transceiver = transceiver;
        m_RadioEntry = radioEntry;
        m_bConfirmSeen = false;

        m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT_PATH);
        if (!m_wRoot)
        {
            Print("[IRRU] Failed to load crypto fill input layout!", LogLevel.ERROR);
            return;
        }

        m_wFillEdit = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("FrequencyEdit"));
        if (!m_wFillEdit)
        {
            Print("[IRRU] Crypto fill edit widget not found!", LogLevel.ERROR);
            Cleanup();
            return;
        }

        int frequency = 0;
        if (m_Transceiver)
            frequency = m_Transceiver.GetFrequency();

        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();

        TextWidget title = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
        if (title)
            title.SetText("CRYPTO FILL");

        TextWidget rangeText = TextWidget.Cast(m_wRoot.FindAnyWidget("RangeText"));
        if (rangeText)
            rangeText.SetText(string.Format("Channel %1 MHz - loaded: %2", FormatFrequency(frequency), settings.GetFillDisplayText(frequency)));

        TextWidget hintText = TextWidget.Cast(m_wRoot.FindAnyWidget("HintText"));
        if (hintText)
            hintText.SetText("Enter = Confirm | Esc = Cancel | 0 = Clear to plaintext");

        m_sOriginalFill = settings.GetFill(frequency);
        m_wFillEdit.SetText(m_sOriginalFill);

        if (!m_EditHandler)
        {
            m_EditHandler = new IRRU_CryptoFillEditHandler();
            m_EditHandler.SetOwner(this);
        }
        m_wFillEdit.AddHandler(m_EditHandler);

        GetGame().GetWorkspace().SetFocusedWidget(m_wFillEdit);
        m_wFillEdit.ActivateWriteMode();

        m_bIsOpen = true;
    }

    //------------------------------------------------------------------------------------------------
    //! Handler callback: the edit box reported a CONFIRMED change (Enter)
    void OnEditConfirmed()
    {
        m_bConfirmSeen = true;
    }

    //------------------------------------------------------------------------------------------------
    //! Driven every frame by SCR_VONController.Update() while open. Resolves
    //! when write mode ends: confirmed -> commit, anything else -> cancel.
    void Poll()
    {
        if (!m_bIsOpen)
            return;

        if (m_wFillEdit && m_wFillEdit.IsInWriteMode())
            return;

        if (m_bConfirmSeen)
            Commit(CurrentText());

        Cleanup();
    }

    //------------------------------------------------------------------------------------------------
    //! External cancel (radial menu closed under the modal)
    void Cancel()
    {
        if (!m_bIsOpen)
            return;

        Cleanup();
    }

    //------------------------------------------------------------------------------------------------
    protected string CurrentText()
    {
        if (!m_wFillEdit)
            return string.Empty;

        string text = m_wFillEdit.GetText();
        text.TrimInPlace();
        return text;
    }

    //------------------------------------------------------------------------------------------------
    protected void Commit(string input)
    {
        if (!m_Transceiver)
            return;

        int frequency = m_Transceiver.GetFrequency();
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();

        // Confirmed-empty is still a no-op: clearing must be the explicit "0"
        if (input.IsEmpty() || input == m_sOriginalFill)
            return;

        if (input == "0")
        {
            if (!settings.GetFill(frequency).IsEmpty())
            {
                settings.ClearFill(frequency);
                AudioSystem.PlaySound(SOUND_CLEAR);
                Feedback(string.Format("[COMSEC] %1 MHz: FILL CLEARED - PLAINTEXT", FormatFrequency(frequency)));
                RefreshEntry();
            }
            return;
        }

        if (!SCR_IRRURadioEarSettings.IRRU_IsDigits(input) || input.Length() > MAX_FILL_LENGTH)
        {
            AudioSystem.PlaySound(SOUND_ERROR);
            Feedback(string.Format("[COMSEC] Invalid fill - 1 to %1 digits (0 clears)", MAX_FILL_LENGTH));
            return;
        }

        settings.SetFill(frequency, input);
        AudioSystem.PlaySound(SOUND_SET);
        Feedback(string.Format("[COMSEC] %1 MHz: FILL SET [%2]", FormatFrequency(frequency), settings.GetFillDisplayText(frequency)));
        RefreshEntry();
    }

    //------------------------------------------------------------------------------------------------
    protected void RefreshEntry()
    {
        if (m_RadioEntry)
            m_RadioEntry.Update();
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
    protected void Cleanup()
    {
        if (m_wRoot)
        {
            m_wRoot.RemoveFromHierarchy();
            m_wRoot = null;
        }

        m_wFillEdit = null;
        m_Transceiver = null;
        m_RadioEntry = null;
        m_sOriginalFill = string.Empty;
        m_bConfirmSeen = false;
        m_bIsOpen = false;
    }

    //------------------------------------------------------------------------------------------------
    protected string FormatFrequency(int freqKHz)
    {
        int wholeMHz = freqKHz / 1000;
        int decimalKHz = (freqKHz % 1000) / 100;

        return string.Format("%1.%2", wholeMHz, decimalKHz);
    }

    //------------------------------------------------------------------------------------------------
    bool IsOpen()
    {
        return m_bIsOpen;
    }
}

//------------------------------------------------------------------------------------------------
//! Control-hint condition: the crypto fill hint only shows while the server
//! has encryption enabled (with it off, the feature has zero visible surface)
[BaseContainerProps()]
class IRRU_EncryptionEnabledActionCondition : SCR_AvailableActionCondition
{
    override bool IsAvailable(notnull SCR_AvailableActionsConditionData data)
    {
        return GetReturnResult(IRRU_RFPropagationNetworkComponent.IsEncryptionEnabled());
    }
}
