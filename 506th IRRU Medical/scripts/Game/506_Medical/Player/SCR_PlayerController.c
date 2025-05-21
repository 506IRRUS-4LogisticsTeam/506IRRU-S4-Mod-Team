// ============================================================================
//  NoInstantDeath_PlayerController.c
//
//  Mirrors ACE-Medical’s pattern: only the server-side player controller
//  flags the character as eligible for No-Instant-Death logic.
// ============================================================================

modded class SCR_PlayerController : SCR_PlayerController
{
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);

		// ── run ONLY on the server (same safeguard ACE uses)
		if (Replication.IsRunning() && !Replication.IsServer())
			return;

		//! Skip when the controller is “possessing” an AI (e.g. GM mode)
		if (!to || IsPossessing())
			return;

		NoInstantDeathComponent nid =
			NoInstantDeathComponent.Cast(to.FindComponent(NoInstantDeathComponent));
		if (nid)
			nid.NID_Initialize();   // flips the init flag & hooks callbacks
	}
}
