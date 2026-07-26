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

		// Server initializes for all characters
		if (Replication.IsServer())
		{
			InitWeaponHooks();
			return;
		}

		// Client: delay init to wait for ownership to be established
		GetGame().GetCallqueue().CallLater(TryInitAsOwner, 500, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void TryInitAsOwner()
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			GetGame().GetCallqueue().Remove(TryInitAsOwner);
			return;
		}

		// Check if this is the local player's controlled entity
		int localPlayerId = GetGame().GetPlayerController().GetPlayerId();
		int entityPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);

		if (entityPlayerId != localPlayerId)
		{
			// Not our character, stop trying
			GetGame().GetCallqueue().Remove(TryInitAsOwner);
			return;
		}

		// This is our character, initialize
		GetGame().GetCallqueue().Remove(TryInitAsOwner);
		InitWeaponHooks();
	}

	//------------------------------------------------------------------------------------------------
	protected void InitWeaponHooks()
	{
		IEntity owner = GetOwner();
		if (!owner)
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
		if (playerId <= 0)
			return;

		// If we're the server, register directly
		if (Replication.IsServer())
		{
			IRRU_ContactViewManager.GetInstance().OnPlayerFired(playerId);
			return;
		}

		// Client: send RPC to server
		Rpc(RpcAsk_PlayerFired, playerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_PlayerFired(int playerId)
	{
		IRRU_ContactViewManager.GetInstance().OnPlayerFired(playerId);
	}
}
