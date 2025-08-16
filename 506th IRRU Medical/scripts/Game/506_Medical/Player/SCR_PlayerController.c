//! Player controller extension for medical system initialization

modded class SCR_PlayerController : SCR_PlayerController
{
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);

		if (Replication.IsRunning() && !Replication.IsServer())
			return;

		if (!to || IsPossessing())
			return;

		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(
			to.FindComponent(IRRU_NoInstantDeathComponent));
		if (nid)
			nid.Initialize();
	}
}
