modded class SCR_InspectCasualtyUserAction : ScriptedUserAction
{
	//! Only inspect casualties on foot or sharing your vehicle
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
