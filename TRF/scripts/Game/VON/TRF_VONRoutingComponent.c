// ──────────────────────────────────────────────────────────────────────────────
//  VON‑routing component with no m_Owner dependency
// ──────────────────────────────────────────────────────────────────────────────

enum EVONAudioRouting
{
	LEFT   = 0,
	RIGHT  = 1,
	CENTER = 2
};

class VONRoutingComponentClass : ScriptComponentClass {}

class VONRoutingComponent : ScriptComponent
{
	// ─────────────────────────── state
	protected EVONAudioRouting   m_currentRouting;
	protected RplComponent       m_Rpl;
	protected bool               m_routingInitialized = false;
	ref map<string, EVONAudioRouting> m_channelRouting = new map<string, EVONAudioRouting>();

	// ─────────────────────────── frame tick
	override void EOnFrame(IEntity owner, float timeSlice)
	{
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

	// ─────────────────────────── public API
	void ApplyRoutingForEntry(SCR_VONEntry entry)
	{
		if (!entry) return;

		EVONAudioRouting routing;
			string id = entry.GetUniqueId();

	if (m_channelRouting.Contains(id))
		routing = m_channelRouting[id];
	else
		routing = EVONAudioRouting.CENTER;

		ApplyRouting(routing);
	}

	void CycleRoutingForEntry(SCR_VONEntry entry)
	{
		if (!entry) return;

		string id = entry.GetUniqueId();
		EVONAudioRouting next;
		
		if (m_channelRouting.Contains(id))
			next = GetNextRouting(m_channelRouting[id]);
		else
			next = GetNextRouting(EVONAudioRouting.CENTER);

		m_channelRouting[id] = next;
		ApplyRouting(next);
		ShowRoutingHint("VON channel routed to " + RoutingToString(next));
	}

	EVONAudioRouting GetCurrentRouting() { return m_currentRouting; }

	EVONAudioRouting GetNextRouting(EVONAudioRouting current)
	{
		return EVONAudioRouting(((int)current + 1) % 3);
	}

	string RoutingToString(EVONAudioRouting routing)
	{
		switch (routing)
		{
			case EVONAudioRouting.LEFT:   return "LEFT";
			case EVONAudioRouting.RIGHT:  return "RIGHT";
			case EVONAudioRouting.CENTER: return "CENTER";
		}
		return "UNKNOWN";
	}

	// ─────────────────────────── internals
	void ApplyRouting(EVONAudioRouting routing)
	{
		m_currentRouting = routing;

		switch (routing)
		{
			case EVONAudioRouting.LEFT:
				AudioSystem.SetVariableByName("VON_LEFT", 3.0,  "TRF:Sounds/VON/VON_DIRECTION.conf");
				AudioSystem.SetVariableByName("VON_RIGHT", 0.0, "TRF:Sounds/VON/VON_DIRECTION.conf");
				ShowRoutingHint("VON routed to LEFT");
				break;

			case EVONAudioRouting.RIGHT:
				AudioSystem.SetVariableByName("VON_LEFT", 0.0,  "TRF:Sounds/VON/VON_DIRECTION.conf");
				AudioSystem.SetVariableByName("VON_RIGHT", 3.0, "TRF:Sounds/VON/VON_DIRECTION.conf");
				ShowRoutingHint("VON routed to RIGHT");
				break;

			default: // CENTER
				AudioSystem.SetVariableByName("VON_LEFT", 1.5, "TRF:Sounds/VON/VON_DIRECTION.conf");
				AudioSystem.SetVariableByName("VON_RIGHT",1.5, "TRF:Sounds/VON/VON_DIRECTION.conf");
				ShowRoutingHint("VON routed to CENTER");
		}

		if (Replication.IsServer())
			Replication.BumpMe();
	}

	void ShowRoutingHint(string msg)
	{
		SCR_HintManagerComponent.ShowCustomHint(msg, "VON Routing", 4);
	}
}
