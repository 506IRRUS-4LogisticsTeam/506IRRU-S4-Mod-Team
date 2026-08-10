modded class SCR_VoNComponent : VoNComponent
{
    protected static const string EAR_ROUTING_CONFIG = "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf";
    protected static bool s_bInitialized = false;
    protected static bool s_bEarRoutingValid = false;
    protected static bool s_bSignalQualityValid = false;
    protected static bool s_bJamStrengthValid = false;
    protected static bool s_bChannelVolumeValid = false;

    //! No "transmission ended" event exists on the receive side, so incoming
    //! traffic is tracked per receiving radio and closed by packet-silence timeout.
    //! VON capture does silence detection, so a long mid-PTT pause stops the packet
    //! stream: the timeout must ride through natural speech pauses, and the reopen
    //! grace keeps a borderline pause from replaying the open beep.
    protected static const float IRRU_RX_SILENCE_TIMEOUT_MS = 600;
    protected static const int IRRU_RX_WATCHDOG_TICK_MS = 150;
    protected static const float IRRU_RX_REOPEN_GRACE_MS = 500;

    protected ref map<BaseTransceiver, float> m_mIRRU_RxLastPacketMs = new map<BaseTransceiver, float>();
    protected ref map<BaseTransceiver, float> m_mIRRU_RxClosedAtMs = new map<BaseTransceiver, float>();
    protected bool m_bIRRU_RxWatchdogRunning = false;

    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {
        if (!s_bInitialized)
        {
            s_bInitialized = true;
            s_bEarRoutingValid = (AudioSystem.GetVariableIDByName("EarRouting", EAR_ROUTING_CONFIG) != -1);
            s_bSignalQualityValid = (AudioSystem.GetVariableIDByName("SignalQuality", EAR_ROUTING_CONFIG) != -1);
            s_bJamStrengthValid = (AudioSystem.GetVariableIDByName("JamStrength", EAR_ROUTING_CONFIG) != -1);
            s_bChannelVolumeValid = (AudioSystem.GetVariableIDByName("ChannelVolume", EAR_ROUTING_CONFIG) != -1);
            IRRU_RFPropagationSettings.GetInstance();
        }

        if (s_bEarRoutingValid)
        {
            float earRouting = GetEarRoutingForTransceiver(receiver);
            AudioSystem.SetVariableByName("EarRouting", earRouting, EAR_ROUTING_CONFIG);
        }

        vector receiverPos = vector.Zero;
        PlayerController playerController = GetGame().GetPlayerController();
        if (playerController)
        {
            IEntity receiverEntity = playerController.GetControlledEntity();
            if (receiverEntity)
                receiverPos = receiverEntity.GetOrigin();
        }

        if (s_bSignalQualityValid)
        {
            float signalQuality = GetSignalQuality(playerId, frequency, receiverPos);
            AudioSystem.SetVariableByName("SignalQuality", signalQuality, EAR_ROUTING_CONFIG);
        }

        if (s_bJamStrengthValid)
        {
            float jamStrength = GetJamStrength(receiverPos);
            AudioSystem.SetVariableByName("JamStrength", jamStrength, EAR_ROUTING_CONFIG);
        }

        if (s_bChannelVolumeValid)
        {
            float channelVolume = GetChannelVolumeForTransceiver(receiver);
            AudioSystem.SetVariableByName("ChannelVolume", channelVolume, EAR_ROUTING_CONFIG);
        }

        if (receiver)
            IRRU_TrackIncomingTransmission(receiver, playerId);

        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
    }

    protected void IRRU_TrackIncomingTransmission(BaseTransceiver receiver, int senderPlayerId)
    {
        PlayerController playerController = GetGame().GetPlayerController();
        if (playerController && playerController.GetPlayerId() == senderPlayerId)
            return;

        float nowMs = GetGame().GetWorld().GetWorldTime();

        if (m_mIRRU_RxLastPacketMs.Contains(receiver))
        {
            m_mIRRU_RxLastPacketMs.Set(receiver, nowMs);
            return;
        }

        m_mIRRU_RxLastPacketMs.Set(receiver, nowMs);

        // A packet gap slightly past the silence timeout would otherwise replay
        // the open beep mid-conversation; reopening within the grace window stays silent.
        float closedAtMs;
        bool withinReopenGrace = m_mIRRU_RxClosedAtMs.Find(receiver, closedAtMs) && (nowMs - closedAtMs < IRRU_RX_REOPEN_GRACE_MS);
        m_mIRRU_RxClosedAtMs.Remove(receiver);
        if (!withinReopenGrace)
            IRRU_RadioBeepHelper.PlayRxOpen(receiver);

        if (!m_bIRRU_RxWatchdogRunning)
        {
            m_bIRRU_RxWatchdogRunning = true;
            GetGame().GetCallqueue().CallLater(IRRU_RxWatchdogTick, IRRU_RX_WATCHDOG_TICK_MS, false);
        }
    }

    protected void IRRU_RxWatchdogTick()
    {
        float nowMs = GetGame().GetWorld().GetWorldTime();

        array<BaseTransceiver> ended = {};
        foreach (BaseTransceiver transceiver, float lastPacketMs : m_mIRRU_RxLastPacketMs)
        {
            if (nowMs - lastPacketMs > IRRU_RX_SILENCE_TIMEOUT_MS)
                ended.Insert(transceiver);
        }

        foreach (BaseTransceiver transceiver : ended)
        {
            m_mIRRU_RxLastPacketMs.Remove(transceiver);
            m_mIRRU_RxClosedAtMs.Set(transceiver, nowMs);
            IRRU_RadioBeepHelper.PlayRxClose(transceiver);
        }

        array<BaseTransceiver> staleClosed = {};
        foreach (BaseTransceiver transceiver, float closedAtMs : m_mIRRU_RxClosedAtMs)
        {
            if (nowMs - closedAtMs >= IRRU_RX_REOPEN_GRACE_MS)
                staleClosed.Insert(transceiver);
        }
        foreach (BaseTransceiver transceiver : staleClosed)
            m_mIRRU_RxClosedAtMs.Remove(transceiver);

        if (m_mIRRU_RxLastPacketMs.Count() > 0)
            GetGame().GetCallqueue().CallLater(IRRU_RxWatchdogTick, IRRU_RX_WATCHDOG_TICK_MS, false);
        else
            m_bIRRU_RxWatchdogRunning = false;
    }

    protected float GetEarRoutingForTransceiver(BaseTransceiver transceiver)
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        IRRUEarRouting routing = settings.GetRouting(transceiver);
        return routing;
    }

    protected float GetChannelVolumeForTransceiver(BaseTransceiver transceiver)
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        float volume = settings.GetVolume(transceiver);
        // Apply exponential curve for better volume sensitivity
        return Math.Pow(volume, 2.5);
    }

    protected float GetSignalQuality(int senderId, int frequencyKHz, vector receiverPos)
    {
        if (!IRRU_RFPropagationNetworkComponent.IsRFPropagationEnabled())
            return 1.0;

        IEntity transmitter = GetGame().GetPlayerManager().GetPlayerControlledEntity(senderId);
        if (!transmitter)
            return 1.0;

        vector transmitterPos = transmitter.GetOrigin();

        IRRU_SignalManager signalManager = IRRU_SignalManager.GetInstance();
        return signalManager.GetSignalQuality(transmitterPos, receiverPos, frequencyKHz);
    }

    protected float GetJamStrength(vector receiverPos)
    {
        IRRU_SignalManager signalManager = IRRU_SignalManager.GetInstance();
        float jammerDegradation = signalManager.GetJammerStrength(receiverPos);
        // CAREFUL THIS IS INVERTED!!!!
        return 1.0 - jammerDegradation;
    }
}