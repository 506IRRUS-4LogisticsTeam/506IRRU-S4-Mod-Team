modded class SCR_InspectCasualtyUserAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		ChimeraCharacter userChar = ChimeraCharacter.Cast(user);
		ChimeraCharacter ownerChar = ChimeraCharacter.Cast(GetOwner());
		if (!userChar || !ownerChar)
			return false;

		if (userChar == ownerChar)
			return false;

		CompartmentAccessComponent userCompAccessComp = userChar.GetCompartmentAccessComponent();
		CompartmentAccessComponent targetCompAccessComp = ownerChar.GetCompartmentAccessComponent();
		if (!userCompAccessComp || !targetCompAccessComp)
			return false;

		IEntity userVeh = userCompAccessComp.GetVehicleIn(userChar);
		IEntity targetVeh = targetCompAccessComp.GetVehicleIn(ownerChar);

		if (!userVeh && !targetVeh)
			return CanBePerformedScript(user);

		if (userVeh && targetVeh && targetVeh == userVeh)
			return CanBePerformedScript(user);

		return false;
	}
}
