//! Shared beep playback for radio transmissions, per-channel style aware.
//! TX beeps are the local key-up/release confirmation heard by the operator
//! (sidetone / talk-permit). RX beeps are the squelch open/close effects heard
//! when receiving someone else's transmission (squelch tail / roger beep).
class IRRU_RadioBeepHelper
{
    static const string BEEP_CONFIG = "{CFD40D355E0717B6}Sounds/VON/506th_beep.acp";
    static const string EAR_ROUTING_CONFIG = "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf";

    static const string EVENT_BEEP_HIGH = "IRRU_BEEP_HIGH";
    static const string EVENT_BEEP_LOW = "IRRU_BEEP_LOW";
    static const string EVENT_CLICK_OFF = "IRRU_CLICK_OFF";
    static const string EVENT_GRS_START = "IRRU_GRS_START";
    static const string EVENT_GRS_END = "IRRU_GRS_END";
    //! Sound node must exist with this exact name in 506th_beep.acp
    static const string EVENT_SQUELCH_TAIL = "IRRU_SQUELCH_TAIL";

    static void PlayTxStart(BaseTransceiver transceiver)
    {
        Play(transceiver, false, true);
    }

    static void PlayTxEnd(BaseTransceiver transceiver)
    {
        Play(transceiver, false, false);
    }

    static void PlayRxOpen(BaseTransceiver transceiver)
    {
        Play(transceiver, true, true);
    }

    static void PlayRxClose(BaseTransceiver transceiver)
    {
        Play(transceiver, true, false);
    }

    //! Sound matrix per channel style. RX is deliberately asymmetric: subtle
    //! squelch on open, the style's prominent sound on close.
    protected static void Play(BaseTransceiver transceiver, bool receiving, bool opening)
    {
        if (!transceiver)
            return;

        if (receiving && !IRRU_RadioUserSettings.GetInstance().AreRxBeepsEnabled())
            return;

        IRRUBeepType beepType = SCR_IRRURadioEarSettings.GetInstance().GetBeepType(transceiver);

        string eventName;
        switch (beepType)
        {
            case IRRUBeepType.ACE_HIGH:
            case IRRUBeepType.ACE_LOW:
                if (receiving && opening)
                    eventName = EVENT_SQUELCH_TAIL;
                else if (!receiving && !opening)
                    eventName = EVENT_CLICK_OFF;
                else if (beepType == IRRUBeepType.ACE_HIGH)
                    eventName = EVENT_BEEP_HIGH;
                else
                    eventName = EVENT_BEEP_LOW;
                break;
            case IRRUBeepType.GRS:
                if (opening)
                    eventName = EVENT_GRS_START;
                else
                    eventName = EVENT_GRS_END;
                break;
            default:
                return;
        }

        PlayRouted(eventName, transceiver);
    }

    //! Write a transceiver's routing/volume into the shared audio variables.
    //! ChannelVolume is set even though 506th_beep.acp only consumes EarRouting
    //! today, so beeps scale with per-channel volume once the variable is wired
    //! into the audio project in Workbench.
    static void ApplyChannelAudioVariables(BaseTransceiver transceiver)
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        IRRUEarRouting routing = settings.GetRouting(transceiver);

        AudioSystem.SetVariableByName("EarRouting", routing, EAR_ROUTING_CONFIG);

        float volume = Math.Pow(settings.GetVolume(transceiver), 2.5);
        AudioSystem.SetVariableByName("ChannelVolume", volume, EAR_ROUTING_CONFIG);
    }

    protected static void PlayRouted(string eventName, BaseTransceiver transceiver)
    {
        // The beep latches its values at PlayEvent within this same call, so it
        // always renders correctly; restoring the authoritative stream's values
        // right after keeps an asynchronous voice-start latch from ever catching
        // beep values (the engine latches variables at sound start).
        ApplyChannelAudioVariables(transceiver);

        vector mat[4];
        Math3D.MatrixIdentity4(mat);

        AudioSystem.PlayEvent(BEEP_CONFIG, eventName, mat);

        IRRU_RadioRxSquelch.GetInstance().RestoreAuthoritativeAudioVariables();
    }
}
