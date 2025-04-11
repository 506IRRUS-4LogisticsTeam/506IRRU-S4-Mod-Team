class VONRoutingComponentClass : ScriptComponentClass {}

enum EVONAudioRouting
{
	CENTER,
	LEFT,
	RIGHT
}

class VONRoutingComponent : ScriptComponent
{
	void ApplyRouting(EVONAudioRouting routing)
	{
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