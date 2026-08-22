modded class SCR_InspectCasualtyUserAction : ScriptedUserAction
{
	//! Vanilla hides the action on undamaged characters; we keep it available so resilience
	//! and CPR state can be read on anyone. The same-vehicle rule is vanilla's.
	override bool CanBeShownScript(IEntity user)
	{
		ChimeraCharacter userChar = ChimeraCharacter.Cast(user);
		ChimeraCharacter ownerChar = ChimeraCharacter.Cast(GetOwner());
		if (!userChar || !ownerChar || userChar == ownerChar)
			return false;

		CompartmentAccessComponent userCompAccessComp = userChar.GetCompartmentAccessComponent();
		CompartmentAccessComponent targetCompAccessComp = ownerChar.GetCompartmentAccessComponent();
		if (!userCompAccessComp || !targetCompAccessComp)
			return false;

		if (userCompAccessComp.GetVehicleIn(userChar) != targetCompAccessComp.GetVehicleIn(ownerChar))
			return false;

		return CanBePerformedScript(user);
	}
}
