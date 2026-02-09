class IRRU_ContactViewWeaponTrackerClass : ScriptComponentClass
{
}

class IRRU_ContactViewWeaponTracker : ScriptComponent
{
	protected BaseWeaponManagerComponent m_WeaponManager;
	protected IEntity m_CurrentWeaponEntity;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		m_WeaponManager = BaseWeaponManagerComponent.Cast(owner.FindComponent(BaseWeaponManagerComponent));
		if (!m_WeaponManager)
			return;

		m_WeaponManager.m_OnWeaponChangeCompleteInvoker.Insert(OnWeaponChanged);
		HookCurrentWeapon();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (m_WeaponManager)
			m_WeaponManager.m_OnWeaponChangeCompleteInvoker.Remove(OnWeaponChanged);

		UnhookCurrentWeapon();
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWeaponChanged(BaseWeaponComponent newWeapon)
	{
		UnhookCurrentWeapon();
		HookCurrentWeapon();
	}

	//------------------------------------------------------------------------------------------------
	protected void HookCurrentWeapon()
	{
		if (!m_WeaponManager)
			return;

		BaseWeaponComponent currentWeapon = m_WeaponManager.GetCurrentWeapon();
		if (!currentWeapon)
			return;

		IEntity weaponEntity = currentWeapon.GetOwner();
		if (!weaponEntity)
			return;

		SCR_MuzzleEffectComponent muzzleEffect = SCR_MuzzleEffectComponent.Cast(weaponEntity.FindComponent(SCR_MuzzleEffectComponent));
		if (muzzleEffect)
		{
			muzzleEffect.GetOnWeaponFired().Insert(OnMuzzleFired);
			m_CurrentWeaponEntity = weaponEntity;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UnhookCurrentWeapon()
	{
		if (!m_CurrentWeaponEntity)
			return;

		SCR_MuzzleEffectComponent muzzleEffect = SCR_MuzzleEffectComponent.Cast(m_CurrentWeaponEntity.FindComponent(SCR_MuzzleEffectComponent));
		if (muzzleEffect)
			muzzleEffect.GetOnWeaponFired().Remove(OnMuzzleFired);

		m_CurrentWeaponEntity = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMuzzleFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(GetOwner());
		if (playerId > 0)
			IRRU_ContactViewManager.GetInstance().OnPlayerFired(playerId);
	}
}
