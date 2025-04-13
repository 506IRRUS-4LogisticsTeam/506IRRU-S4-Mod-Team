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

		// Get RplComponent if needed
		if (!m_Rpl)
			m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));

		// Server check and replication bump
		if (Replication.IsServer())
			Replication.BumpMe();

		// Only apply routing once
		if (!m_routingInitialized)
		{
			ApplyRouting(EVONAudioRouting.CENTER);
			m_routingInitialized = true;
		}
	}
	
	void ApplyRouting(EVONAudioRouting routing)
	{
		m_currentRouting = routing;

		if (!m_Owner)
			return;

		m_Rpl = RplComponent.Cast(m_Owner.FindComponent(RplComponent));

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
			
			if (Replication.IsServer())
				Replication.BumpMe();
		}
	}

	void ShowRoutingHint(string msg)
	{
		SCR_HintManagerComponent.ShowCustomHint(msg, "VON Routing", 4);
	}
}
