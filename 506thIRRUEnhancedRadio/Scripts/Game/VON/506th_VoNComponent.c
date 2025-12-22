modded class SCR_VoNComponent : VoNComponent
{
    protected static const string EAR_ROUTING_CONFIG = "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf";
    protected static bool s_bInitialized = false;
    protected static bool s_bVariableValid = false;

    //------------------------------------------------------------------------------------------------
    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {
        if (!s_bInitialized)
        {
            s_bInitialized = true;
            
            Print("[IRRU] ========================================");
            Print("[IRRU] Enhanced Radio Mod - Initializing");
            Print("[IRRU] Config path: " + EAR_ROUTING_CONFIG);
            
            int varId = AudioSystem.GetVariableIDByName("EarRouting", EAR_ROUTING_CONFIG);
            Print("[IRRU] EarRouting Variable ID: " + varId);
            
            s_bVariableValid = (varId != -1);
            
            if (!s_bVariableValid)
                Print("[IRRU] ERROR: EarRouting variable not found!");
            else
                Print("[IRRU] SUCCESS: Audio variable found and ready.");
            
            Print("[IRRU] ========================================");
        }
        
        if (s_bVariableValid)
        {
            float earRouting = GetEarRoutingForTransceiver(receiver);
            AudioSystem.SetVariableByName("EarRouting", earRouting, EAR_ROUTING_CONFIG);
            
            float currentValue = AudioSystem.GetVariableValue("EarRouting", EAR_ROUTING_CONFIG);
            Print("[IRRU] EarRouting set to: " + earRouting + " | Verified: " + currentValue);
        }
        
        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
    }

    //------------------------------------------------------------------------------------------------
    protected float GetEarRoutingForTransceiver(BaseTransceiver transceiver)
    {
        SCR_IRRURadioEarSettings settings = SCR_IRRURadioEarSettings.GetInstance();
        IRRUEarRouting routing = settings.GetRouting(transceiver);
        return routing;
    }
}