modded class SCR_PlayerArsenalLoadout
{
	override protected static bool ShouldSaveStorage(BaseInventoryStorageComponent candidate)
	{
		if (!candidate)
			return false;
		if (super.ShouldSaveStorage(candidate))
			return true;

		foreach (typename storageType : ARSENALLOADOUT_COMPONENTS_TO_CHECK)
		{
			if (candidate.Type().IsInherited(storageType))
				return true;
		}
		return false;
	}
}

class IRRU_LoadoutManagerStorage : SCR_PlayerArsenalLoadout
{
	protected static void RecordFailure(string detail, out int failures, out string error)
	{
		failures++;
		if (error.IsEmpty())
			error = detail;
		Print("[IRRU Kits] " + detail, LogLevel.WARNING);
	}

	static bool ApplyComplete(IEntity character, string data, out string error)
	{
		error = "";
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromString(data))
		{
			error = "invalid-json";
			return false;
		}
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!manager)
		{
			error = "inventory-unavailable";
			return false;
		}

		if (!context.StartObject("arsenalLoadout"))
		{
			int version;
			if (!context.ReadValue("version", version))
			{
				error = "missing-arsenal-loadout";
				return false;
			}
			if (version != 2)
			{
				error = "unsupported-grs-kit-version";
				return false;
			}
			return ApplyGRS(character, context, manager, error);
		}

		int failures;
		if (!ApplyStorages(character, context, manager, failures, error))
		{
			if (error.IsEmpty())
				error = "invalid-storage-data";
			return false;
		}
		context.EndObject();
		if (failures > 0)
		{
			error = string.Format("partial-load: %1 failures; %2", failures, error);
			return false;
		}

		// Run the native entry point after reconciliation so addon metadata hooks still execute.
		JsonLoadContext nativeContext = new JsonLoadContext();
		if (!nativeContext.LoadFromString(data) || !SCR_PlayerArsenalLoadout.ApplyLoadoutString(character, nativeContext))
		{
			error = "native-loadout-metadata-or-apply-failed";
			return false;
		}
		return true;
	}

	protected static bool ApplyStorages(IEntity owner, LoadContext context, InventoryStorageManagerComponent manager, out int failures, out string error)
	{
		int count;
		if (!context.StartArray("storages", count))
			return true;

		set<BaseInventoryStorageComponent> candidates = new set<BaseInventoryStorageComponent>();
		FindStorageComponents(owner, candidates);
		for (int i = 0; i < count; i++)
		{
			if (!context.StartObject())
				return false;
			string id;
			if (!context.ReadValue("id", id))
				return false;
			BaseInventoryStorageComponent storage = FindStorage(owner, candidates, id);
			if (!storage)
				RecordFailure("storage-unavailable: " + id, failures, error);
			else if (!ApplySlots(storage, context, manager, failures, error))
				return false;
			if (!context.EndObject())
				return false;
		}
		return context.EndArray();
	}

	protected static BaseInventoryStorageComponent FindStorage(IEntity owner, set<BaseInventoryStorageComponent> candidates, string id)
	{
		foreach (BaseInventoryStorageComponent candidate : candidates)
		{
			if (candidate && GetComponentIdentifier(owner, candidate) == id)
				return candidate;
		}
		return null;
	}

	protected static bool ApplySlots(BaseInventoryStorageComponent storage, LoadContext context, InventoryStorageManagerComponent manager, out int failures, out string error)
	{
		int count;
		if (!context.StartMap("slots", count))
			return false;

		array<string> keys = {};
		for (int i = 0; i < count; i++)
		{
			string key;
			if (!context.ReadMapKey(i, key) || key.ToInt(-1) < 0)
				return false;
			keys.Insert(key);
		}

		// Remove obsolete items before spawning: old attachments and cargo can block replacements.
		array<InventoryItemComponent> existing = {};
		storage.GetOwnedItems(existing, false);
		foreach (InventoryItemComponent item : existing)
		{
			int slotId = item.GetParentSlot().GetID();
			if (keys.Find(slotId.ToString()) < 0 && !manager.TryDeleteItem(item.GetOwner()))
				RecordFailure(string.Format("remove-failed: %1 slot %2", storage.ClassName(), slotId), failures, error);
		}

		foreach (string slotKey : keys)
		{
			if (!context.StartObject(slotKey))
				return false;
			if (!ApplyItem(storage, slotKey.ToInt(), context, manager, failures, error))
				return false;
			if (!context.EndObject())
				return false;
		}
		return context.EndMap();
	}

	protected static bool ApplyItem(BaseInventoryStorageComponent storage, int slotId, LoadContext context, InventoryStorageManagerComponent manager, out int failures, out string error)
	{
		ResourceName prefab;
		if (!context.ReadValue("prefab", prefab))
			return false;
		IEntity entity = storage.Get(slotId);
		if (!entity || SCR_ResourceNameUtils.GetPrefabName(entity) != prefab)
		{
			Resource resource = Resource.Load(prefab);
			if (!resource || !resource.IsValid())
			{
				RecordFailure("prefab-unavailable: " + prefab, failures, error);
				return true;
			}
			if (entity && !manager.TryDeleteItem(entity))
			{
				RecordFailure("replace-delete-failed: " + prefab, failures, error);
				return true;
			}
			if (!manager.TrySpawnPrefabToStorage(prefab, storage, slotId))
			{
				RecordFailure(string.Format("spawn-failed: %1 in %2 slot %3", prefab, storage.ClassName(), slotId), failures, error);
				return true;
			}
			entity = storage.Get(slotId);
			if (!entity)
			{
				RecordFailure("spawned-item-unavailable: " + prefab, failures, error);
				return true;
			}
		}
		return ApplyStorages(entity, context, manager, failures, error);
	}

	protected static bool ResolveGRSSlot(IEntity character, string key, out BaseInventoryStorageComponent storage, out int slotId)
	{
		storage = null;
		slotId = -1;
		array<string> weaponKeys = {"Primary", "Secondary", "Sidearm", "Grenade", "Throwable"};
		int weaponIndex = weaponKeys.Find(key);
		if (weaponIndex >= 0)
		{
			storage = BaseInventoryStorageComponent.Cast(character.FindComponent(EquipedWeaponStorageComponent));
			slotId = weaponIndex;
			return storage && storage.GetSlot(slotId);
		}

		if (key.StartsWith("Character."))
		{
			string sourceName = key.Substring(10, key.Length() - 10);
			set<BaseInventoryStorageComponent> candidates = new set<BaseInventoryStorageComponent>();
			FindStorageComponents(character, candidates);
			foreach (BaseInventoryStorageComponent candidate : candidates)
			{
				if (!candidate.Type().IsInherited(SCR_EquipmentStorageComponent))
					continue;
				for (int i = 0; i < candidate.GetSlotsCount(); i++)
				{
					InventoryStorageSlot slot = candidate.GetSlot(i);
					if (slot && (slot.GetSourceName() == sourceName || slot.GetID().ToString() == sourceName))
					{
						storage = candidate;
						slotId = slot.GetID();
						return true;
					}
				}
			}
			return false;
		}

		typename area = key.ToType();
		SCR_CharacterInventoryStorageComponent clothing = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!area || !clothing)
			return false;
		LoadoutSlotInfo clothingSlot = clothing.GetSlotFromArea(area);
		if (!clothingSlot)
			return false;
		storage = clothing;
		slotId = clothingSlot.GetID();
		return true;
	}

	protected static bool ApplyGRS(IEntity character, JsonLoadContext context, InventoryStorageManagerComponent manager, out string error)
	{
		int count;
		if (!context.StartMap("slotData", count) || count == 0)
		{
			error = "invalid-grs-slot-data";
			return false;
		}

		array<string> keys = {};
		array<BaseInventoryStorageComponent> storages = {};
		array<int> slots = {};
		// Resolve every root slot before modifying equipment; GRS uses area names, not fixed indices.
		for (int i = 0; i < count; i++)
		{
			string key;
			BaseInventoryStorageComponent storage;
			int slotId;
			if (!context.ReadMapKey(i, key) || !ResolveGRSSlot(character, key, storage, slotId))
			{
				error = "grs-slot-unavailable: " + key;
				return false;
			}
			if (!context.StartObject(key))
			{
				error = "invalid-grs-item-data: " + key;
				return false;
			}
			ResourceName prefab;
			if (!context.ReadValue("prefab", prefab))
			{
				error = "invalid-grs-prefab: " + key;
				return false;
			}
			Resource resource = Resource.Load(prefab);
			if (!resource || !resource.IsValid())
			{
				error = "prefab-unavailable: " + prefab;
				return false;
			}
			if (!context.EndObject())
			{
				error = "invalid-grs-item-data: " + key;
				return false;
			}
			keys.Insert(key);
			storages.Insert(storage);
			slots.Insert(slotId);
		}

		int failures;
		set<BaseInventoryStorageComponent> roots = new set<BaseInventoryStorageComponent>();
		FindStorageComponents(character, roots);
		foreach (BaseInventoryStorageComponent root : roots)
		{
			if (!root.Type().IsInherited(SCR_CharacterInventoryStorageComponent) && !root.Type().IsInherited(EquipedWeaponStorageComponent) && !root.Type().IsInherited(SCR_EquipmentStorageComponent))
				continue;
			array<InventoryItemComponent> items = {};
			root.GetOwnedItems(items, false);
			foreach (InventoryItemComponent item : items)
			{
				if (!manager.TryDeleteItem(item.GetOwner()))
					RecordFailure("grs-strip-failed: " + root.ClassName(), failures, error);
			}
		}

		for (int index = 0; index < keys.Count(); index++)
		{
			if (!context.StartObject(keys[index]) || !ApplyItem(storages[index], slots[index], context, manager, failures, error) || !context.EndObject())
			{
				error = "invalid-grs-item-data: " + keys[index];
				return false;
			}
		}
		if (!context.EndMap())
		{
			error = "invalid-grs-slot-data";
			return false;
		}
		if (failures > 0)
		{
			error = string.Format("partial-load: %1 failures; %2", failures, error);
			return false;
		}
		return true;
	}
}
