//! Per-sender receive state: stream timing plus cached RF results (the
//! terrain walk behind signal quality is far too heavy to repeat per packet).
class IRRU_SenderStream
{
    float m_fLastPacketMs;
    float m_fSignalQuality;
    float m_fJamStrength;
    float m_fComputedAtMs;
    int m_iFrequency;
}

enum IRRU_EStreamOpening
{
    NONE,
    //! Resume inside the verified keep-alive window: the engine may not
    //! actually restart the sound event on this packet
    RESUME,
    //! First packet, frequency change or long gap: the engine starts (or
    //! restarts) a sound event on this packet
    START
}

modded class SCR_VoNComponent : VoNComponent
{
    //! Voice packets feed IRRU_RadioRxSquelch as the fallback squelch trigger
    //! (key-state RPCs are the primary); this component only hosts the timeout
    //! ticker, which is all the state machine needs while voice is flowing.
    protected static const int IRRU_RX_WATCHDOG_TICK_MS = 150;
    protected static const float IRRU_SIGNAL_REFRESH_MS = 500;
    //! Packets arrive every ~20-60ms; any gap above this is treated as a
    //! possible restart (a PTT release + re-key inside the keep-alive window
    //! is unprobed, so it stays well below it).
    protected static const float IRRU_STREAM_GAP_MS = 400;
    //! In-game probe: one sound event survived a 0.8s mid-key silence and
    //! restarted after 4.9s, so a resume below this is known not to restart.
    protected static const float IRRU_VERIFIED_KEEPALIVE_MS = 800;

    protected bool m_bIRRU_RxWatchdogRunning = false;
    protected ref map<int, ref IRRU_SenderStream> m_mIRRU_Streams = new map<int, ref IRRU_SenderStream>();

    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {
        // DO NOT TOUCH DIRECT VOICE PACKETS, IT WILL MESS UP THE STATE MACHINE - RADIO TRANSMISSIONS ONLY
        if (receiver)
        {
            PlayerController playerController = GetGame().GetPlayerController();
            if (!playerController || playerController.GetPlayerId() != playerId)
            {
                IRRU_TrackIncomingTransmission(receiver, frequency);

                // The audio variables are single global slots, but the engine
                // reads them once per sound event, in the frame of the packet
                // that starts it, and restarts the event after a silence gap
                // (verified in-game: later writes are ignored). So each stream
                // writes its own values only on its opening/resume packet -
                // steady-state writes would just race other streams' opening
                // writes in the same frame.
                IRRU_EStreamOpening opening = IRRU_GetStreamOpening(playerId, frequency);
                if (opening != IRRU_EStreamOpening.NONE)
                    IRRU_ApplyAudioVariables(playerId, receiver, frequency, playerController, opening == IRRU_EStreamOpening.START);
            }
        }

        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
    }

    protected void IRRU_TrackIncomingTransmission(BaseTransceiver receiver, int frequency)
    {
        IRRU_RadioRxSquelch.GetInstance().OnVoicePacket(frequency, receiver);

        if (!m_bIRRU_RxWatchdogRunning)
        {
            m_bIRRU_RxWatchdogRunning = true;
            GetGame().GetCallqueue().CallLater(IRRU_RxWatchdogTick, IRRU_RX_WATCHDOG_TICK_MS, false);
        }
    }

    protected void IRRU_RxWatchdogTick()
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();

        if (IRRU_RadioRxSquelch.GetInstance().Tick(nowMs))
            GetGame().GetCallqueue().CallLater(IRRU_RxWatchdogTick, IRRU_RX_WATCHDOG_TICK_MS, false);
        else
            m_bIRRU_RxWatchdogRunning = false;
    }

    //! Classifies a packet by whether the engine may start a sound event on it.
    //! A resume known not to restart the event yields to a genuine start that
    //! another stream already wrote this frame, instead of overwriting it
    //! before the engine reads it.
    protected IRRU_EStreamOpening IRRU_GetStreamOpening(int senderPlayerId, int frequency)
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();
        IRRU_SenderStream stream = IRRU_GetSenderStream(senderPlayerId);
        float gapMs = nowMs - stream.m_fLastPacketMs;
        stream.m_fLastPacketMs = nowMs;

        if (stream.m_iFrequency != frequency || gapMs > IRRU_VERIFIED_KEEPALIVE_MS)
            return IRRU_EStreamOpening.START;

        if (gapMs <= IRRU_STREAM_GAP_MS)
            return IRRU_EStreamOpening.NONE;

        if (IRRU_RadioRxSquelch.GetInstance().WasAudioSlotStartedThisFrame())
            return IRRU_EStreamOpening.NONE;

        return IRRU_EStreamOpening.RESUME;
    }

    protected void IRRU_ApplyAudioVariables(int senderPlayerId, BaseTransceiver receiver, int frequency, PlayerController playerController, bool eventStart)
    {
        IRRU_RadioBeepHelper.ApplyChannelAudioVariables(receiver);
        IRRU_RadioRxSquelch.GetInstance().OnAudioSlotWritten(frequency, eventStart);

        vector receiverPos = vector.Zero;
        if (playerController)
        {
            IEntity receiverEntity = playerController.GetControlledEntity();
            if (receiverEntity)
                receiverPos = receiverEntity.GetOrigin();
        }

        IRRU_SenderStream stream = IRRU_GetSenderSignals(senderPlayerId, frequency, receiverPos);
        AudioSystem.SetVariableByName("SignalQuality", stream.m_fSignalQuality, IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
        AudioSystem.SetVariableByName("JamStrength", stream.m_fJamStrength, IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
    }

    protected IRRU_SenderStream IRRU_GetSenderStream(int senderPlayerId)
    {
        IRRU_SenderStream stream;
        if (m_mIRRU_Streams.Find(senderPlayerId, stream))
            return stream;

        stream = new IRRU_SenderStream();
        m_mIRRU_Streams.Set(senderPlayerId, stream);
        return stream;
    }

    protected IRRU_SenderStream IRRU_GetSenderSignals(int senderPlayerId, int frequency, vector receiverPos)
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();

        IRRU_SenderStream stream = IRRU_GetSenderStream(senderPlayerId);
        if (stream.m_iFrequency == frequency && nowMs - stream.m_fComputedAtMs < IRRU_SIGNAL_REFRESH_MS)
            return stream;

        stream.m_iFrequency = frequency;
        stream.m_fComputedAtMs = nowMs;
        stream.m_fSignalQuality = IRRU_ComputeSignalQuality(senderPlayerId, frequency, receiverPos);
        // The audio graph expects 1 = clean, 0 = fully jammed
        stream.m_fJamStrength = 1.0 - IRRU_JammerManager.GetInstance().CalculateJammerDegradation(receiverPos);
        return stream;
    }

    protected float IRRU_ComputeSignalQuality(int senderPlayerId, int frequencyKHz, vector receiverPos)
    {
        if (!IRRU_RFPropagationNetworkComponent.IsRFPropagationEnabled())
            return 1.0;

        IEntity transmitter = GetGame().GetPlayerManager().GetPlayerControlledEntity(senderPlayerId);
        if (!transmitter)
            return 1.0;

        return IRRU_RFPropagationModel.GetInstance().CalculateSignalQuality(transmitter.GetOrigin(), receiverPos, frequencyKHz);
    }
}
