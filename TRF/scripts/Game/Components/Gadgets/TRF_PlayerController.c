modded class SCR_PlayerController
{
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);

		// Delay to allow components to initialize
		GetGame().GetCallqueue().CallLater(ApplyVONRouting, 100, false, to);
	}

	void ApplyVONRouting(IEntity playerEntity)
	{
		if (!playerEntity)
			return;

		IEntity child = playerEntity.GetChildren();
		while (child)
		{
			SCR_RadioComponent scrRadio = SCR_RadioComponent.Cast(child.FindComponent(SCR_RadioComponent));
			if (!scrRadio)
			{
				child = child.GetSibling();
				continue;
			}

			int radioType = scrRadio.GetRadioType();

			// Stereo routing
			if (radioType == ERadioType.ANPRC68)
			{
				AudioSystem.SetVariableByName("VON_LEFT", 3.0, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
				AudioSystem.SetVariableByName("VON_RIGHT", 0.0, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
				SCR_HintManagerComponent.ShowCustomHint("VON routed to LEFT (ANPRC68)", "Audio Routing", 4);
			}
			else if (radioType == ERadioType.ANPRC77)
			{
				AudioSystem.SetVariableByName("VON_LEFT", 0.0, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
				AudioSystem.SetVariableByName("VON_RIGHT", 3.0, "ToastyRadios:Sounds/VON/VON_DIRECTION.conf");
				SCR_HintManagerComponent.ShowCustomHint("VON routed to RIGHT (ANPRC77)", "Audio Routing", 4);
			}

			child = child.GetSibling(); // keep searching in case there are multiple radios
		}
	}
}
