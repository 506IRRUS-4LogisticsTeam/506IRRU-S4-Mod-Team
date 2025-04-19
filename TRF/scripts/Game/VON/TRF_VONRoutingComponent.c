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
	protected IEntity m_Owner;

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		m_Owner = owner;

		if (!m_Rpl)
			m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));

		if (Replication.IsServer())
			Replication.BumpMe();

		if (!m_routingInitialized)
		{
			ApplyRouting(EVONAudioRouting.CENTER);
			m_routingInitialized = true;
		}
	}

	ref map<string, EVONAudioRouting> m_channelRouting = new map<string, EVONAudioRouting>();

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

	void ApplyRouting(EVONAudioRouting routing)
	{
		m_currentRouting = routing;

		if (!m_Owner)
			return;

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

		if (Replication.IsServer())
			Replication.BumpMe();
	}

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
	
	string RoutingToString(EVONAudioRouting routing)
	{
		switch (routing)
		{
			case EVONAudioRouting.LEFT: return "LEFT";
			case EVONAudioRouting.RIGHT: return "RIGHT";
			case EVONAudioRouting.CENTER: return "CENTER";
			default: return "UNKNOWN";
		}
	
		// Redundant but satisfies compiler
		return "UNKNOWN";
	}
	
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



	void ShowRoutingHint(string msg)
	{
		SCR_HintManagerComponent.ShowCustomHint(msg, "VON Routing", 4);
	}

	EVONAudioRouting GetCurrentRouting()
	{
		return m_currentRouting;
	}
}