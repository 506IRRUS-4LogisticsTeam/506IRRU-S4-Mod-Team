//built on hopes and dreams
// if this works this will be insanely diabolical

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
            int varId = AudioSystem.GetVariableIDByName("EarRouting", EAR_ROUTING_CONFIG);
            s_bVariableValid = (varId != -1);
            
            if (!s_bVariableValid)
                Print("[506thRadio] ERROR: EarRouting variable not found!");
        }
        
        if (s_bVariableValid)
        {
            float earRouting = GetEarRoutingForTransceiver(receiver);
            AudioSystem.SetVariableByName("EarRouting", earRouting, EAR_ROUTING_CONFIG);
        }
        
        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
    }

    //------------------------------------------------------------------------------------------------
    protected float GetEarRoutingForTransceiver(BaseTransceiver transceiver)
    {
        // TODO: Implement per-transceiver routing preferences
        // 0 = Center (both ears)
        // 1 = Right ear
        // 2 = Left ear
        return 2;
    }
}