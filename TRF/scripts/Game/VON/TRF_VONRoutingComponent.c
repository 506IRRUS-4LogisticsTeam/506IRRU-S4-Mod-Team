// Enum for audio routing directions
enum EVONAudioRouting
{
    LEFT = 0,
    RIGHT = 1,
    CENTER = 2
};

class VONRoutingComponentClass : ScriptComponentClass {}

class VONRoutingComponent : ScriptComponent
{
    protected EVONAudioRouting m_currentRouting;
    protected RplComponent m_Rpl;
    protected bool m_routingInitialized = false;
    ref map<string, EVONAudioRouting> m_channelRouting = new map<string, EVONAudioRouting>();

    // Runs every frame to initialize routing if needed
    override void EOnFrame(IEntity owner, float timeSlice)
    {
        if (!m_Rpl)
            m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));

        if (Replication.IsServer())
            Replication.BumpMe();

        if (!m_routingInitialized)
        {
            ApplyRouting(EVONAudioRouting.CENTER);  // Default to CENTER if not initialized
            m_routingInitialized = true;
        }
    }

    // Applies routing for a specific VON entry
    void ApplyRoutingForEntry(SCR_VONEntry entry)
    {
        if (!entry)
            return;

        string entryId = entry.GetUniqueId();
        EVONAudioRouting routing = EVONAudioRouting.CENTER;

        if (m_channelRouting.Contains(entryId))
            routing = m_channelRouting[entryId];

        ApplyRouting(routing);
    }

    // Applies routing based on the direction (LEFT, RIGHT, or CENTER)
    void ApplyRouting(EVONAudioRouting routing)
    {
        m_currentRouting = routing;

        switch (routing)
        {
            case EVONAudioRouting.LEFT:
                AudioSystem.SetVariableByName("VON_LEFT", 3.0, "TRF:Sounds/VON/VON_DIRECTION.conf");
                AudioSystem.SetVariableByName("VON_RIGHT", 0.0, "TRF:Sounds/VON/VON_DIRECTION.conf");
                ShowRoutingHint("VON routed to LEFT");
                break;

            case EVONAudioRouting.RIGHT:
                AudioSystem.SetVariableByName("VON_LEFT", 0.0, "TRF:Sounds/VON/VON_DIRECTION.conf");
                AudioSystem.SetVariableByName("VON_RIGHT", 3.0, "TRF:Sounds/VON/VON_DIRECTION.conf");
                ShowRoutingHint("VON routed to RIGHT");
                break;

            case EVONAudioRouting.CENTER:
                AudioSystem.SetVariableByName("VON_LEFT", 1.5, "TRF:Sounds/VON/VON_DIRECTION.conf");
                AudioSystem.SetVariableByName("VON_RIGHT", 1.5, "TRF:Sounds/VON/VON_DIRECTION.conf");
                ShowRoutingHint("VON routed to CENTER");
                break;
        }

        if (Replication.IsServer())
            Replication.BumpMe();
    }

    // Helper to cycle through the routing options
    EVONAudioRouting GetNextRouting(EVONAudioRouting current)
    {
        int nextIndex = ((int)current + 1) % 3;
        switch (nextIndex)
        {
            case 0: return EVONAudioRouting.LEFT;
            case 1: return EVONAudioRouting.RIGHT;
            case 2: return EVONAudioRouting.CENTER;
        }
        return EVONAudioRouting.CENTER;
    }

    // Cycle routing for a specific VON entry
    void CycleRoutingForEntry(SCR_VONEntry entry)
    {
        if (!entry)
            return;

        string entryId = entry.GetUniqueId();
        EVONAudioRouting current = EVONAudioRouting.CENTER;

        if (m_channelRouting.Contains(entryId))
            current = m_channelRouting[entryId];

        EVONAudioRouting next = GetNextRouting(current);
        m_channelRouting[entryId] = next;

        ApplyRouting(next);
        ShowRoutingHint("VON channel routed to " + RoutingToString(next));
    }

    // Converts the routing enum to a string for display
	string RoutingToString(EVONAudioRouting routing)
	{
		switch (routing)
		{
			case EVONAudioRouting.LEFT: return "LEFT";
			case EVONAudioRouting.RIGHT: return "RIGHT";
			case EVONAudioRouting.CENTER: return "CENTER";
			default: return "UNKNOWN";
		}
		return "UNKNOWN";
	}

    // Displays a routing hint message to the player
    void ShowRoutingHint(string msg)
    {
        SCR_HintManagerComponent.ShowCustomHint(msg, "VON Routing", 4);
    }

    // Gets the current routing direction
    EVONAudioRouting GetCurrentRouting()
    {
        return m_currentRouting;
    }
}
