[ComponentEditorProps(category: "GameScripted/AIStealth", description: "Tracks whether this character is carrying the stealth item; used by the AI perception override to suppress detection beyond a configured range.")]
class IRRU_StealthComponentClass : ScriptComponentClass
{
}

class IRRU_StealthComponent : ScriptComponent
{
	[RplProp(onRplName: "OnStealthStateChanged")]
	protected bool m_bStealthActive = false;

	protected RplComponent m_Rpl;
	protected SCR_InventoryStorageManagerComponent m_StorageManager;
	protected bool m_bScanScheduled = false;

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

		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		if (!m_StorageManager)
		{
			if (IRRU_StealthSettings.IsDebugEnabled())
				Print(string.Format("[AIStealth] %1: no SCR_InventoryStorageManagerComponent, stealth disabled for this entity", GetDebugName(owner)), LogLevel.WARNING);
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

		bool shouldBeActive = HasStealthItem();

		if (shouldBeActive != m_bStealthActive)
		{
			m_bStealthActive = shouldBeActive;

			if (m_Rpl)
				Replication.BumpMe();

			if (IRRU_StealthSettings.IsDebugEnabled())
			{
				string state = "INACTIVE";
				if (m_bStealthActive)
					state = "ACTIVE";
				Print(string.Format("[AIStealth] %1: stealth %2", GetDebugName(owner), state));
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
