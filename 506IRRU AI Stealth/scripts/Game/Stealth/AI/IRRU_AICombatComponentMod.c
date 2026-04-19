//! Overrides per-AI target visibility check so that AI ignores stealthed
//! characters beyond a configurable distance. Each AI has its own SCR_AICombatComponent
//! and its own world position via GetOwner(), so we can do a precise distance gate.
//! Close stealthed targets remain visible; distant ones are treated as "not seen".
modded class SCR_AICombatComponent
{
	//------------------------------------------------------------------------------------------------
	override bool IsTargetVisible(notnull BaseTarget target)
	{
		if (IRRU_IsStealthedTargetOutOfRange(target))
		{
			if (IRRU_StealthSettings.IsDebugEnabled())
				Print(string.Format("[AIStealth] AI %1: suppressing stealthed target %2 (out of range)", GetOwner(), target.GetTargetEntity()));
			return false;
		}

		return super.IsTargetVisible(target);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns true if the target carries an active IRRU_StealthComponent and is beyond
	//! the configured detection range from this AI.
	protected bool IRRU_IsStealthedTargetOutOfRange(BaseTarget target)
	{
		if (!target)
			return false;

		IEntity targetEntity = target.GetTargetEntity();
		if (!targetEntity)
			return false;

		IRRU_StealthComponent stealth = IRRU_StealthComponent.Cast(targetEntity.FindComponent(IRRU_StealthComponent));
		if (!stealth || !stealth.IsStealthActive())
			return false;

		IEntity aiOwner = GetOwner();
		if (!aiOwner)
			return false;

		float threshold = IRRU_StealthSettings.GetDetectionRange();
		float distSq = vector.DistanceSq(aiOwner.GetOrigin(), targetEntity.GetOrigin());
		return distSq > (threshold * threshold);
	}
}
