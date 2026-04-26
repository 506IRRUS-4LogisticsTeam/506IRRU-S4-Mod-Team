[ComponentEditorProps(category: "GameScripted/AIStealth", description: "Drops the character's perceivable state to 'disarmed' when they carry the stealth item and no AI is within the detection range, causing AI group perception to skip registering them as targets. Firing a weapon breaks stealth for a configurable period.")]
class IRRU_StealthComponentClass : ScriptComponentClass
{
}

class IRRU_StealthComponent : ScriptComponent
{
	[RplProp(onRplName: "OnStealthStateChanged")]
	protected bool m_bStealthActive = false;

	protected RplComponent m_Rpl;
	protected SCR_InventoryStorageManagerComponent m_StorageManager;
	protected PerceivableComponent m_Perceivable;
	protected SCR_CharacterControllerComponent m_Ctrl;
	protected bool m_bScanScheduled = false;

	// Server-authoritative suppression timer. Set on weapon fire (via RPC from owning
	// client or directly on server). Decremented each tick. While > 0, stealth is forced off.
	protected float m_fSuppressionRemainingSec = 0;

	// Weapon tracking — owning client hooks muzzle fire events and RPCs to server.
	protected BaseWeaponManagerComponent m_WeaponManager;
	protected IEntity m_CurrentWeaponEntity;
	protected bool m_bWeaponHooksInitialized = false;

	// Local-only HUD indicator: small blue dot bottom-right while stealth is active.
	protected static const ResourceName INDICATOR_LAYOUT = "{38BD5C4E20000001}UI/layouts/IRRU_StealthIndicator.layout";
	protected Widget m_wIndicatorRoot;
	protected bool m_bIndicatorInitialized = false;

	// Sphere-query scratch state. Enfusion QueryEntitiesBySphere callbacks don't carry
	// context, so the callback reads this static state. The component tick is single-
	// threaded, so concurrent access isn't a concern.
	protected static vector s_vQueryOrigin;
	protected static float s_fQueryRadiusSq;
	protected static IEntity s_QuerySelf;
	protected static bool s_bQueryFoundAI;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_StorageManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		m_Perceivable = PerceivableComponent.Cast(owner.FindComponent(PerceivableComponent));
		m_Ctrl = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));

		// Server drives the stealth state and its tick loop.
		if (!Replication.IsRunning() || Replication.IsServer())
			InitServerSide(owner);

		// Weapon fire detection: server hooks directly, clients wait for ownership.
		if (Replication.IsServer())
			InitWeaponHooks();
		else
			GetGame().GetCallqueue().CallLater(TryInitWeaponHooksAsOwner, 500, true);

		// HUD indicator runs on every peer that has a local player. The ownership
		// check inside filters out dedicated servers (no PlayerController) and
		// other players' characters.
		GetGame().GetCallqueue().CallLater(TryInitIndicatorAsOwner, 500, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void InitServerSide(IEntity owner)
	{
		if (!m_StorageManager)
		{
			if (IRRU_StealthSettings.IsDebugEnabled())
				Print(string.Format("[AIStealth] %1: no SCR_InventoryStorageManagerComponent, stealth disabled", GetDebugName(owner)), LogLevel.WARNING);
			return;
		}

		if (!m_Perceivable)
		{
			if (IRRU_StealthSettings.IsDebugEnabled())
				Print(string.Format("[AIStealth] %1: no PerceivableComponent, stealth disabled", GetDebugName(owner)), LogLevel.WARNING);
			return;
		}

		ScheduleScan();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(UpdateStealthState);
		GetGame().GetCallqueue().Remove(TryInitWeaponHooksAsOwner);
		GetGame().GetCallqueue().Remove(TryInitIndicatorAsOwner);

		if (m_WeaponManager)
			m_WeaponManager.m_OnWeaponChangeCompleteInvoker.Remove(OnWeaponChanged);

		UnhookCurrentWeapon();
		DestroyIndicator();
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	bool IsStealthActive()
	{
		return m_bStealthActive;
	}

	//------------------------------------------------------------------------------------------------
	protected void ScheduleScan()
	{
		if (m_bScanScheduled)
			return;

		m_bScanScheduled = true;
		GetGame().GetCallqueue().CallLater(UpdateStealthState, IRRU_StealthSettings.GetCheckIntervalMs(), false);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateStealthState()
	{
		m_bScanScheduled = false;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		if (Replication.IsRunning() && !Replication.IsServer())
			return;

		// Decrement suppression timer using this tick's interval.
		float tickSec = IRRU_StealthSettings.GetCheckIntervalMs() / 1000.0;
		if (m_fSuppressionRemainingSec > 0)
		{
			m_fSuppressionRemainingSec -= tickSec;
			if (m_fSuppressionRemainingSec < 0)
				m_fSuppressionRemainingSec = 0;
		}

		// Only drive disarmed state while alive. SCR_CharacterPerceivableComponent's own
		// life-state handler manages disarmed on incap/death; stepping on that would undo
		// its work.
		if (m_Ctrl && m_Ctrl.GetLifeState() != ECharacterLifeState.ALIVE)
		{
			ScheduleScan();
			return;
		}

		bool hasItem = HasStealthItem();
		bool hasAINearby = false;
		if (hasItem)
			hasAINearby = HasAINearby(owner, IRRU_StealthSettings.GetDetectionRange());

		bool isSuppressed = (m_fSuppressionRemainingSec > 0);
		bool shouldBeStealth = hasItem && !hasAINearby && !isSuppressed;

		if (shouldBeStealth != m_bStealthActive)
		{
			m_bStealthActive = shouldBeStealth;

			if (m_Perceivable)
				m_Perceivable.SetDisarmed(m_bStealthActive);

			if (m_Rpl)
				Replication.BumpMe();

			// RplProp callbacks fire on receivers only, so the host (server == local
			// player) never gets OnStealthStateChanged. Refresh the local HUD here.
			UpdateIndicatorVisibility();

			if (IRRU_StealthSettings.IsDebugEnabled())
			{
				string reason;
				if (shouldBeStealth)
					reason = "item present, no AI within range";
				else if (!hasItem)
					reason = "no item";
				else if (isSuppressed)
					reason = string.Format("fire suppression (%1s remaining)", m_fSuppressionRemainingSec);
				else
					reason = "AI within range";

				string state;
				if (shouldBeStealth)
					state = "ACTIVE";
				else
					state = "INACTIVE";

				Print(string.Format("[AIStealth] %1: stealth %2 (%3)", GetDebugName(owner), state, reason));
			}
		}

		ScheduleScan();
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasStealthItem()
	{
		if (!m_StorageManager)
			return false;

		ResourceName target = IRRU_StealthSettings.GetStealthItemPrefab();
		if (target == ResourceName.Empty)
			return false;

		array<BaseInventoryStorageComponent> storages = {};
		m_StorageManager.GetStorages(storages);

		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			array<IEntity> items = {};
			storage.GetAll(items);
			foreach (IEntity item : items)
			{
				if (!item)
					continue;

				EntityPrefabData prefabData = item.GetPrefabData();
				if (!prefabData)
					continue;

				if (prefabData.GetPrefabName() == target)
					return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Sphere query around the player for any AI character (SCR_AICombatComponent).
	//! Returns true if any AI is within threshold, meaning stealth should break.
	protected bool HasAINearby(notnull IEntity owner, float threshold)
	{
		s_vQueryOrigin = owner.GetOrigin();
		s_fQueryRadiusSq = threshold * threshold;
		s_QuerySelf = owner;
		s_bQueryFoundAI = false;

		GetGame().GetWorld().QueryEntitiesBySphere(s_vQueryOrigin, threshold, OnQueryEntity, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);

		bool result = s_bQueryFoundAI;
		s_QuerySelf = null;
		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Return true to keep iterating, false to stop.
	protected static bool OnQueryEntity(IEntity e)
	{
		if (!e || e == s_QuerySelf)
			return true;

		if (vector.DistanceSq(e.GetOrigin(), s_vQueryOrigin) > s_fQueryRadiusSq)
			return true;

		SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(e.FindComponent(SCR_AICombatComponent));
		if (!combat)
			return true;

		s_bQueryFoundAI = true;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Weapon fire detection (client-side listener → RPC to server). Pattern adapted from
	// IRRU_ContactViewWeaponTracker in 506IRRU GM Contact View.

	//------------------------------------------------------------------------------------------------
	protected void TryInitWeaponHooksAsOwner()
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			GetGame().GetCallqueue().Remove(TryInitWeaponHooksAsOwner);
			return;
		}

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		int localPlayerId = pc.GetPlayerId();
		int entityPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);

		if (entityPlayerId != localPlayerId)
		{
			GetGame().GetCallqueue().Remove(TryInitWeaponHooksAsOwner);
			return;
		}

		GetGame().GetCallqueue().Remove(TryInitWeaponHooksAsOwner);
		InitWeaponHooks();
	}

	//------------------------------------------------------------------------------------------------
	protected void InitWeaponHooks()
	{
		if (m_bWeaponHooksInitialized)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		m_WeaponManager = BaseWeaponManagerComponent.Cast(owner.FindComponent(BaseWeaponManagerComponent));
		if (!m_WeaponManager)
			return;

		m_WeaponManager.m_OnWeaponChangeCompleteInvoker.Insert(OnWeaponChanged);
		HookCurrentWeapon();
		m_bWeaponHooksInitialized = true;
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
		if (Replication.IsServer())
		{
			ApplyFireSuppression();
			return;
		}

		Rpc(RpcAsk_FireSuppression);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_FireSuppression()
	{
		ApplyFireSuppression();
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyFireSuppression()
	{
		float duration = IRRU_StealthSettings.GetFireSuppressionSec();
		if (duration <= 0)
			return;

		m_fSuppressionRemainingSec = duration;

		if (IRRU_StealthSettings.IsDebugEnabled())
			Print(string.Format("[AIStealth] %1: fire suppression armed for %2s", GetDebugName(GetOwner()), duration));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnStealthStateChanged()
	{
		UpdateIndicatorVisibility();
	}

	//------------------------------------------------------------------------------------------------
	// Local HUD indicator. Same ownership-poll pattern as TryInitWeaponHooksAsOwner —
	// at OnPostInit time, RplComponent.IsOwner() and GetPlayerController() aren't yet
	// reliable, so we poll until ownership is established.
	protected void TryInitIndicatorAsOwner()
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			GetGame().GetCallqueue().Remove(TryInitIndicatorAsOwner);
			return;
		}

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		int localPlayerId = pc.GetPlayerId();
		int entityPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);

		if (entityPlayerId != localPlayerId)
		{
			GetGame().GetCallqueue().Remove(TryInitIndicatorAsOwner);
			return;
		}

		GetGame().GetCallqueue().Remove(TryInitIndicatorAsOwner);
		InitIndicator();
	}

	//------------------------------------------------------------------------------------------------
	protected void InitIndicator()
	{
		if (m_bIndicatorInitialized)
			return;

		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return;

		m_wIndicatorRoot = ws.CreateWidgets(INDICATOR_LAYOUT);
		if (!m_wIndicatorRoot)
		{
			if (IRRU_StealthSettings.IsDebugEnabled())
				Print("[AIStealth] Failed to load stealth indicator layout", LogLevel.WARNING);
			return;
		}

		m_bIndicatorInitialized = true;
		UpdateIndicatorVisibility();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateIndicatorVisibility()
	{
		if (!m_wIndicatorRoot)
			return;

		m_wIndicatorRoot.SetVisible(m_bStealthActive);
	}

	//------------------------------------------------------------------------------------------------
	protected void DestroyIndicator()
	{
		if (m_wIndicatorRoot)
		{
			m_wIndicatorRoot.RemoveFromHierarchy();
			m_wIndicatorRoot = null;
		}
		m_bIndicatorInitialized = false;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetDebugName(IEntity e)
	{
		if (!e)
			return "UnknownEntity";

		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			int pid = pm.GetPlayerIdFromControlledEntity(e);
			if (pid > 0)
			{
				string n = pm.GetPlayerName(pid);
				if (!n.IsEmpty())
					return n;
			}
		}
		return e.ToString();
	}
}
