class IRRU_ContactViewWeaponTrackerClass : ScriptComponentClass
{
}

class IRRU_ContactViewWeaponTracker : ScriptComponent
{
	protected BaseWeaponManagerComponent m_WeaponManager;
	protected IEntity m_CurrentWeaponEntity;
	protected RplComponent m_Rpl;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));

		bool isOwner = !m_Rpl || m_Rpl.IsOwner();
		Print(string.Format("[ContactView] WeaponTracker OnPostInit - IsServer: %1, IsOwner: %2", Replication.IsServer(), isOwner));

		// Only initialize on the owning client (or server in singleplayer)
		if (m_Rpl && !m_Rpl.IsOwner())
		{
			Print("[ContactView] WeaponTracker - not owner, skipping init");
			return;
		}

		m_WeaponManager = BaseWeaponManagerComponent.Cast(owner.FindComponent(BaseWeaponManagerComponent));
		if (!m_WeaponManager)
		{
			Print("[ContactView] WeaponTracker - no weapon manager found");
			return;
		}

		Print("[ContactView] WeaponTracker - hooking weapon manager");
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
		Print(string.Format("[ContactView] OnMuzzleFired - playerId: %1, IsServer: %2", playerId, Replication.IsServer()));

		if (playerId <= 0)
			return;

		// If we're the server, register directly
		if (Replication.IsServer())
		{
			Print("[ContactView] Server - registering fire directly");
			IRRU_ContactViewManager.GetInstance().OnPlayerFired(playerId);
			return;
		}

		// Client: send RPC to server
		Print("[ContactView] Client - sending RPC to server");
		Rpc(RpcAsk_PlayerFired, playerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_PlayerFired(int playerId)
	{
		Print(string.Format("[ContactView] RpcAsk_PlayerFired received - playerId: %1", playerId));
		IRRU_ContactViewManager.GetInstance().OnPlayerFired(playerId);
	}
}
