[ComponentEditorProps(category: "GameScripted/AIStealth", description: "Drops the character's perceivable state to 'disarmed' when they carry the stealth item and no AI is within the detection range, causing AI group perception to skip registering them as targets.")]
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

		if (!Replication.IsServer() && Replication.IsRunning())
			return;

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

		bool shouldBeStealth = hasItem && !hasAINearby;

		if (shouldBeStealth != m_bStealthActive)
		{
			m_bStealthActive = shouldBeStealth;

			if (m_Perceivable)
				m_Perceivable.SetDisarmed(m_bStealthActive);

			if (m_Rpl)
				Replication.BumpMe();

			if (IRRU_StealthSettings.IsDebugEnabled())
			{
				string reason;
				if (shouldBeStealth)
					reason = "item present, no AI within range";
				else if (!hasItem)
					reason = "no item";
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
	protected void OnStealthStateChanged()
	{
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
