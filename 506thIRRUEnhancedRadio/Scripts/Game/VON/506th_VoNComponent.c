modded class SCR_VoNComponent : VoNComponent
{
    protected static const string EAR_ROUTING_CONFIG = "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf";
    protected static bool s_bInitialized = false;
    protected static bool s_bEarRoutingValid = false;
    protected static bool s_bSignalQualityValid = false;
    protected static bool s_bJamStrengthValid = false;
    protected static bool s_bChannelVolumeValid = false;

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

        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
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