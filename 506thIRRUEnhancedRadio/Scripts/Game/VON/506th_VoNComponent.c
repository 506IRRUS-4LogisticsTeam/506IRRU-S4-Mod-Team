modded class SCR_VoNComponent : VoNComponent
{
    protected static const string EAR_ROUTING_CONFIG = "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf";
    protected static bool s_bInitialized = false;
    protected static bool s_bEarRoutingValid = false;
    protected static bool s_bJamStrengthValid = false;

    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {
        if (!s_bInitialized)
        {
            s_bInitialized = true;
            s_bEarRoutingValid = (AudioSystem.GetVariableIDByName("EarRouting", EAR_ROUTING_CONFIG) != -1);
            s_bJamStrengthValid = (AudioSystem.GetVariableIDByName("JamStrength", EAR_ROUTING_CONFIG) != -1);
        }

        if (s_bEarRoutingValid)
        {
            float earRouting = GetEarRoutingForTransceiver(receiver);
            AudioSystem.SetVariableByName("EarRouting", earRouting, EAR_ROUTING_CONFIG);
        }

        if (s_bJamStrengthValid)
        {
            float signalQuality = GetSignalQuality(playerId);
            AudioSystem.SetVariableByName("JamStrength", signalQuality, EAR_ROUTING_CONFIG);
        }

        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
    }

    protected float GetEarRoutingForTransceiver(BaseTransceiver transceiver)
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        IRRUEarRouting routing = settings.GetRouting(transceiver);
        return routing;
    }

    protected float GetSignalQuality(int senderId)
    {
        // Get transmitter position
        IEntity transmitter = GetGame().GetPlayerManager().GetPlayerControlledEntity(senderId);
        if (!transmitter)
            return 1.0;

        vector transmitterPos = transmitter.GetOrigin();

        // Get receiver (local player) position
        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
            return 1.0;

        IEntity receiver = playerController.GetControlledEntity();
        if (!receiver)
            return 1.0;

        vector receiverPos = receiver.GetOrigin();

        // Get signal quality from central manager
        IRRU_SignalManager signalManager = IRRU_SignalManager.GetInstance();
        float signalQuality = signalManager.GetSignalQuality(transmitterPos, receiverPos);

        return signalQuality;
    }
}