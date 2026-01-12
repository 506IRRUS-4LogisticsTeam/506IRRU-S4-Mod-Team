//! Component that tracks weapon firing for Contact View
//! Attach to player character entities

class IRRU_ContactViewWeaponTrackerClass : ScriptComponentClass
{
}

class IRRU_ContactViewWeaponTracker : ScriptComponent
{
	protected int m_iPlayerId = -1;
	protected bool m_bInitialized = false;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		// Get the player ID for this character
		m_iPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);
		m_bInitialized = true;

		// Hook into all muzzle effect components on this character
		HookMuzzleEffects(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void HookMuzzleEffects(IEntity owner)
	{
		if (!owner)
			return;

		// Find all SCR_MuzzleEffectComponent components
		SCR_MuzzleEffectComponent muzzleEffect = SCR_MuzzleEffectComponent.Cast(owner.FindComponent(SCR_MuzzleEffectComponent));
		if (muzzleEffect)
		{
			muzzleEffect.GetOnWeaponFired().Insert(OnMuzzleFired);

			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print(string.Format("[ContactView] Hooked weapon firing for player %1", m_iPlayerId));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Called when muzzle effect component fires
	void OnMuzzleFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		if (!m_bInitialized || m_iPlayerId <= 0)
			return;

		IRRU_ContactViewManager.GetInstance().OnPlayerFired(m_iPlayerId);

		if (IRRU_ContactViewSettings.IsDebugEnabled())
			Print(string.Format("[ContactView] Player %1 fired weapon", m_iPlayerId));
	}

	//------------------------------------------------------------------------------------------------
	int GetPlayerId()
	{
		return m_iPlayerId;
	}
}
