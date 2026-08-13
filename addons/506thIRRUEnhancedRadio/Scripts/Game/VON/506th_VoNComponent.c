modded class SCR_VoNComponent : VoNComponent
{
    protected static const string EAR_ROUTING_CONFIG = "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf";
    protected static bool s_bInitialized = false;
    protected static bool s_bEarRoutingValid = false;
    protected static bool s_bSignalQualityValid = false;
    protected static bool s_bJamStrengthValid = false;
    protected static bool s_bChannelVolumeValid = false;

    //! Voice packets feed IRRU_RadioRxSquelch as the fallback squelch trigger
    //! (key-state RPCs are the primary); this component only hosts the timeout
    //! ticker, which is all the state machine needs while voice is flowing.
    protected static const int IRRU_RX_WATCHDOG_TICK_MS = 150;

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
            float earRouting = IRRU_GetEarRoutingForTransceiver(receiver);
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
            float signalQuality = IRRU_GetSignalQuality(playerId, frequency, receiverPos);
            AudioSystem.SetVariableByName("SignalQuality", signalQuality, EAR_ROUTING_CONFIG);
        }

        if (s_bJamStrengthValid)
        {
            float jamStrength = IRRU_GetJamStrength(receiverPos);
            AudioSystem.SetVariableByName("JamStrength", jamStrength, EAR_ROUTING_CONFIG);
        }

        if (s_bChannelVolumeValid)
        {
            float channelVolume = IRRU_GetChannelVolumeForTransceiver(receiver);
            AudioSystem.SetVariableByName("ChannelVolume", channelVolume, EAR_ROUTING_CONFIG);
        }

        if (receiver)
            IRRU_TrackIncomingTransmission(receiver, frequency, playerId);

        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
    }

    protected void IRRU_TrackIncomingTransmission(BaseTransceiver receiver, int frequency, int senderPlayerId)
    {
        PlayerController playerController = GetGame().GetPlayerController();
        if (playerController && playerController.GetPlayerId() == senderPlayerId)
            return;

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

    protected float IRRU_GetEarRoutingForTransceiver(BaseTransceiver transceiver)
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        IRRUEarRouting routing = settings.GetRouting(transceiver);
        return routing;
    }

    protected float IRRU_GetChannelVolumeForTransceiver(BaseTransceiver transceiver)
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        float volume = settings.GetVolume(transceiver);
        // Apply exponential curve for better volume sensitivity
        return Math.Pow(volume, 2.5);
    }

    protected float IRRU_GetSignalQuality(int senderId, int frequencyKHz, vector receiverPos)
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

    protected float IRRU_GetJamStrength(vector receiverPos)
    {
        IRRU_SignalManager signalManager = IRRU_SignalManager.GetInstance();
        float jammerDegradation = signalManager.GetJammerStrength(receiverPos);
        // CAREFUL THIS IS INVERTED!!!!
        return 1.0 - jammerDegradation;
    }
}