modded class SCR_AvailableActionsDisplay
{
	override protected bool CanBeShown(IEntity controlledEntity)
	{
		if (IRRU_LoadoutManagerSystemMenu.IsOpen())
			return false;

		return super.CanBeShown(controlledEntity);
	}

}

modded class SCR_ActionMenuInteractionDisplay
{
	override void Show(bool show, float speed = UIConstants.FADE_RATE_INSTANT, EAnimationCurve curve = EAnimationCurve.LINEAR)
	{
		if (IRRU_LoadoutManagerSystemMenu.IsOpen())
		{
			super.Show(false, UIConstants.FADE_RATE_INSTANT, curve);
			return;
		}

		super.Show(show, speed, curve);
	}

	override void DisplayUpdate(IEntity owner, float timeSlice)
	{
		if (IRRU_LoadoutManagerSystemMenu.IsOpen())
		{
			Show(false);
			return;
		}

		super.DisplayUpdate(owner, timeSlice);
	}
}
