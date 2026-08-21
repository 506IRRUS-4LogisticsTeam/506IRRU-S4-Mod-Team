//! Cached per-sender RF results; the terrain walk behind signal quality is far
//! too heavy to repeat on every voice packet.
class IRRU_SenderSignalCache
{
    float m_fSignalQuality;
    float m_fJamStrength;
    float m_fComputedAtMs;
    int m_iFrequency;
}

modded class SCR_VoNComponent : VoNComponent
{
    //! Voice packets feed IRRU_RadioRxSquelch as the fallback squelch trigger
    //! (key-state RPCs are the primary); this component only hosts the timeout
    //! ticker, which is all the state machine needs while voice is flowing.
    protected static const int IRRU_RX_WATCHDOG_TICK_MS = 150;
    protected static const float IRRU_SIGNAL_REFRESH_MS = 500;

    protected bool m_bIRRU_RxWatchdogRunning = false;
    protected ref map<int, ref IRRU_SenderSignalCache> m_mIRRU_SignalCache = new map<int, ref IRRU_SenderSignalCache>();

    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {
        // Direct speech arrives here too (receiver == null); only radio traffic
        // drives the squelch state machine and the audio variables.
        if (receiver)
        {
            PlayerController playerController = GetGame().GetPlayerController();
            if (!playerController || playerController.GetPlayerId() != playerId)
                IRRU_TrackIncomingTransmission(receiver, frequency);

            // The audio variables are single global slots the engine latches at
            // sound start, so only the arbitrated stream may write them.
            if (IRRU_RadioRxSquelch.GetInstance().ShouldDriveAudioVariables(frequency))
                IRRU_ApplyAudioVariables(playerId, receiver, frequency, playerController);
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

    protected void IRRU_ApplyAudioVariables(int senderPlayerId, BaseTransceiver receiver, int frequency, PlayerController playerController)
    {
        IRRU_RadioBeepHelper.ApplyChannelAudioVariables(receiver);

        vector receiverPos = vector.Zero;
        if (playerController)
        {
            IEntity receiverEntity = playerController.GetControlledEntity();
            if (receiverEntity)
                receiverPos = receiverEntity.GetOrigin();
        }

        IRRU_SenderSignalCache cache = IRRU_GetSenderSignals(senderPlayerId, frequency, receiverPos);
        AudioSystem.SetVariableByName("SignalQuality", cache.m_fSignalQuality, IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
        AudioSystem.SetVariableByName("JamStrength", cache.m_fJamStrength, IRRU_RadioBeepHelper.EAR_ROUTING_CONFIG);
    }

    protected IRRU_SenderSignalCache IRRU_GetSenderSignals(int senderPlayerId, int frequency, vector receiverPos)
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();

        IRRU_SenderSignalCache cache;
        if (m_mIRRU_SignalCache.Find(senderPlayerId, cache) && cache.m_iFrequency == frequency && nowMs - cache.m_fComputedAtMs < IRRU_SIGNAL_REFRESH_MS)
            return cache;

        if (!cache)
        {
            cache = new IRRU_SenderSignalCache();
            m_mIRRU_SignalCache.Set(senderPlayerId, cache);
        }

        cache.m_iFrequency = frequency;
        cache.m_fComputedAtMs = nowMs;
        cache.m_fSignalQuality = IRRU_ComputeSignalQuality(senderPlayerId, frequency, receiverPos);
        // The audio graph expects 1 = clean, 0 = fully jammed
        cache.m_fJamStrength = 1.0 - IRRU_JammerManager.GetInstance().CalculateJammerDegradation(receiverPos);
        return cache;
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
