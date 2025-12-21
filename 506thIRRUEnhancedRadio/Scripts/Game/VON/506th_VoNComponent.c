//built on hopes and dreams
// if this works this will be insanely diabolical

modded class SCR_VoNComponent : VoNComponent
{
    protected static const string EAR_ROUTING_CONFIG = "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf";

    //------------------------------------------------------------------------------------------------
    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {
        Print("OnReceive FIRED!!! playerid: " + playerId);
        
        float earRouting = GetEarRoutingForTransceiver(receiver);
        
        bool success = AudioSystem.SetVariableByName("EarRouting", earRouting, EAR_ROUTING_CONFIG);
        
        if (success)
            Print("LETS FUCKING GOOOOOO it worked");
        else
            Print("fuck it didnt work WHY");
        
        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);
    }

    //------------------------------------------------------------------------------------------------
    protected float GetEarRoutingForTransceiver(BaseTransceiver transceiver)
    {
        Print("returning 2 for left ear PLEASE WORK");
        return 2;
    }
}