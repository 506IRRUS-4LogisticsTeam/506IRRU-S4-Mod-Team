enum EVONAudioRouting
{
    LEFT = 0,    // Audio routed to the left channel
    RIGHT = 1,   // Audio routed to the right channel
    CENTER = 2   // Audio routed to the center channel
};

// Define the VONRoutingComponentClass that inherits from RplComponentClass
class VONRoutingComponentClass : ScriptComponentClass {}

// Define the VONRoutingComponent class that inherits from RplComponent
class VONRoutingComponent : ScriptComponent
{
    // Replicated property for routing
    [RplProp()]
    protected int m_currentRouting;

    void ApplyRouting(EVONAudioRouting routing)
    {
        m_currentRouting = routing;  // Store enum as integer

        switch (routing)
        {
            case EVONAudioRouting.LEFT:
                AudioSystem.SetVariableByName("VON_LEFT", 3.0, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
                AudioSystem.SetVariableByName("VON_RIGHT", 0.0, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
                ShowRoutingHint("VON routed to LEFT");
                break;

            case EVONAudioRouting.RIGHT:
                AudioSystem.SetVariableByName("VON_LEFT", 0.0, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
                AudioSystem.SetVariableByName("VON_RIGHT", 3.0, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
                ShowRoutingHint("VON routed to RIGHT");
                break;

            case EVONAudioRouting.CENTER:
                AudioSystem.SetVariableByName("VON_LEFT", 1.5, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
                AudioSystem.SetVariableByName("VON_RIGHT", 1.5, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
                ShowRoutingHint("VON routed to CENTER");
                break;
        }
    }

    void ShowRoutingHint(string msg)
    {
        SCR_HintManagerComponent.ShowCustomHint(msg, "VON Routing", 4);
    }
}
