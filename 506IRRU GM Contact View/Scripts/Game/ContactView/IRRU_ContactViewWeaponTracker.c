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

		m_iPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);
		m_bInitialized = true;

		HookMuzzleEffects(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void HookMuzzleEffects(IEntity owner)
	{
		if (!owner)
			return;

		SCR_MuzzleEffectComponent muzzleEffect = SCR_MuzzleEffectComponent.Cast(owner.FindComponent(SCR_MuzzleEffectComponent));
		if (muzzleEffect)
		{
			muzzleEffect.GetOnWeaponFired().Insert(OnMuzzleFired);

			if (IRRU_ContactViewSettings.IsDebugEnabled())
				Print(string.Format("[ContactView] Hooked weapon firing for player %1", m_iPlayerId));
		}
	}

	//------------------------------------------------------------------------------------------------
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
