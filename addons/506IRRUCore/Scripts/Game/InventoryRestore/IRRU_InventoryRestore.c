[EntityEditorProps(category: "GameScripted/Character", description: "Checks and adds map and radio to player inventory on spawn")]
class IRRU_InventoryRestore : ScriptComponent
{
    static string GetPlayerDebugName(IEntity player)
    {
        if (!player)
            return "unknown";

        PlayerManager pm = GetGame().GetPlayerManager();
        if (pm)
        {
            int playerId = pm.GetPlayerIdFromControlledEntity(player);
            if (playerId > 0)
            {
                string networkName = pm.GetPlayerName(playerId);
                if (!networkName.IsEmpty())
                    return networkName;
            }
        }

        string playerName = player.GetName();
        if (playerName.IsEmpty())
            return player.ToString();

        return playerName;
    }

    void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] Component initialized");
    }
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        SetEventMask(owner, EntityEvent.INIT);
    }

    override void EOnInit(IEntity owner)
    {
        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] Component initialized on entity - inventory check handled by SCR_PlayerController");
    }

    static void CheckAndAddItems(IEntity player)
    {
        string playerName = GetPlayerDebugName(player);

        if (!Replication.IsServer())
        {
            if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                Print("[InventoryCheck] Not running on server");
            return;
        }

        if (!player) {
            if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                Print("[InventoryCheck] Player entity is null");
            return;
        }

        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] Starting inventory check for player: " + playerName);

        SCR_InventoryStorageManagerComponent storageManager = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!storageManager) {
            if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                Print("[InventoryCheck] SCR_InventoryStorageManagerComponent not found");
            return;
        }

        // Get all storages and scan full inventory
        array<BaseInventoryStorageComponent> storages = {};
        storageManager.GetStorages(storages);

        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] [" + playerName + "] Found " + storages.Count() + " storages");

        array<IEntity> items = {};
        foreach (BaseInventoryStorageComponent storage : storages) {
            if (!storage)
                continue;

            array<IEntity> storageItems = {};
            storage.GetAll(storageItems);
            foreach (IEntity item : storageItems) {
                if (item)
                    items.Insert(item);
            }
        }

        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] [" + playerName + "] Found " + items.Count() + " total items in player inventory");

        bool hasMap = false;
        bool hasRadio = false;

        ResourceName mapResourceName = "{922F95F91943F69A}Prefabs/Items/Equipment/Maps/Map_Paper_01/PaperMap_01_folded_US.et";

        //! Handed out when the player carries none of the radios below
        ResourceName issuedRadioName = "{3AAE94CE6ADC37E6}Prefabs/Accessories/IRRU_Radio_ANPRC_V3.et";

        //! IRRU radios, which carry RelayTransceivers and therefore transmit without a
        //! relay in range. The superseded GRS prefabs are deliberately NOT listed: they
        //! still use RadioTransceivers and cannot transmit at all unless a relay is in
        //! range, so a player arriving with one (e.g. from a stale saved loadout) counts
        //! as having no radio and is issued a working one.
        array<ResourceName> radioNames = {
            "{DFB0F41DA8D04752}Prefabs/Accessories/IRRU_Radio_ANPRC_JTAC_Right.et",
            "{F526F4C334526B74}Prefabs/Accessories/IRRU_Radio_ANPRC_LEFT_JTAC.et",
            "{ED6C531BB75E8DB5}Prefabs/Accessories/IRRU_Radio_MPU5.et",
            issuedRadioName
        };

        foreach (IEntity item : items) {
            ResourceName prefabName = item.GetPrefabData().GetPrefabName();
            if (prefabName == mapResourceName) {
                hasMap = true;
                if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                    Print("[InventoryCheck] Player already has map item prefab: " + prefabName);
            }
            foreach (ResourceName radioName : radioNames) {
                if (prefabName == radioName) {
                    hasRadio = true;
                    if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                        Print("[InventoryCheck] Player already has radio item prefab: " + prefabName);
                }
            }
        }

        if (IRRU_InventoryRestoreSettings.IsDebugEnabled()) {
            if (!hasMap)
                Print("[InventoryCheck] [" + playerName + "] Player does not have map: " + mapResourceName);
            if (!hasRadio)
                Print("[InventoryCheck] [" + playerName + "] Player does not have a radio (any of configured prefabs)");
        }

        if (!hasMap)
            TryAddMissingItem(storageManager, player, mapResourceName, "map", "Adding map to gear");

        if (!hasRadio)
            TryAddMissingItem(storageManager, player, issuedRadioName, "radio", "Adding ANPRCV3 Radio to gear");

        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] Inventory check completed for player: " + playerName);
    }

    static bool IsItemInStorages(SCR_InventoryStorageManagerComponent storageManager, IEntity searchItem)
    {
        if (!storageManager || !searchItem)
            return false;

        array<BaseInventoryStorageComponent> storages = {};
        storageManager.GetStorages(storages);

        foreach (BaseInventoryStorageComponent storage : storages)
        {
            if (!storage)
                continue;

            array<IEntity> items = {};
            storage.GetAll(items);
            foreach (IEntity item : items)
            {
                if (item == searchItem)
                    return true;
            }
        }

        return false;
    }

    static void TryAddMissingItem(SCR_InventoryStorageManagerComponent storageManager, IEntity player, ResourceName prefabName, string itemLabel, string addLog)
    {
        string playerName = GetPlayerDebugName(player);

        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] [" + playerName + "] " + addLog);

        Resource itemRes = Resource.Load(prefabName);
        EntitySpawnParams spawnParams = new EntitySpawnParams();
        IEntity item = GetGame().SpawnEntityPrefab(itemRes, GetGame().GetWorld(), spawnParams);

        if (item)
        {
            bool success = TryAddItemToInventory(storageManager, player, item, itemLabel);
            if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                Print("[InventoryCheck] [" + playerName + "] " + itemLabel + " added success state: " + success);
        }
        else
        {
            if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                Print("[InventoryCheck] Failed to spawn " + itemLabel + " item");
        }
    }

    static bool IsPrefabInStorages(SCR_InventoryStorageManagerComponent storageManager, ResourceName prefabName)
    {
        if (!storageManager || prefabName.IsEmpty())
            return false;

        array<BaseInventoryStorageComponent> storages = {};
        storageManager.GetStorages(storages);

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

                ResourceName itemPrefab = item.GetPrefabData().GetPrefabName();
                if (itemPrefab == prefabName)
                    return true;
            }
        }

        return false;
    }

    static void VerifyInsertedItemLater(SCR_InventoryStorageManagerComponent storageManager, ResourceName itemPrefab, string itemLabel)
    {
        if (!storageManager || itemPrefab.IsEmpty())
            return;

        bool foundPrefab = IsPrefabInStorages(storageManager, itemPrefab);
        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] Delayed verify " + itemLabel + " prefab present after insert? " + foundPrefab + " (" + itemPrefab + ")");

        if (!foundPrefab)
        {
            if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                Print("[InventoryCheck] Delayed verify still failed for " + itemLabel + " - possible external inventory sync issue");
        }
    }

    static bool TryAddItemToInventory(SCR_InventoryStorageManagerComponent storageManager, IEntity player, IEntity item, string itemLabel)
    {
        string playerName = GetPlayerDebugName(player);

        if (!storageManager || !item || !player)
        {
            if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                Print("[InventoryCheck] TryAddItemToInventory invalid params for " + itemLabel);
            return false;
        }

        bool success = storageManager.TryInsertItem(item, EStoragePurpose.PURPOSE_ANY, null);
        if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] [" + playerName + "] TryInsertItem (manager default) " + itemLabel + ": " + success);

        bool inserted = false;
        if (success)
        {
            ResourceName itemPrefab = item.GetPrefabData().GetPrefabName();
            bool foundExact = IsItemInStorages(storageManager, item);
            bool foundPrefab = IsPrefabInStorages(storageManager, itemPrefab);

            if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
            {
                Print("[InventoryCheck] Item " + itemLabel + " exact reference present after insert: " + foundExact);
                Print("[InventoryCheck] Item " + itemLabel + " prefab present after insert: " + foundPrefab + " (" + itemPrefab + ")");
            }

            if (foundExact || foundPrefab)
            {
                inserted = true;
            }
            else
            {
                if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                    Print("[InventoryCheck] [" + playerName + "] Misleading true from TryInsertItem for " + itemLabel + " - doing retry path");

                // retry insertion, moving item to player origin first maybe required
                item.SetOrigin(player.GetOrigin());
                success = storageManager.TryInsertItem(item, EStoragePurpose.PURPOSE_ANY, null);
                if (IRRU_InventoryRestoreSettings.IsDebugEnabled())
                    Print("[InventoryCheck] [" + playerName + "] TryInsertItem retry " + itemLabel + ": " + success);

                if (success)
                {
                    foundExact = IsItemInStorages(storageManager, item);
                    foundPrefab = IsPrefabInStorages(storageManager, itemPrefab);
                    inserted = (foundExact || foundPrefab);
                }

                if (!inserted)
                {
                    GetGame().GetCallqueue().CallLater(VerifyInsertedItemLater, 0.5, false, storageManager, itemPrefab, itemLabel);
                }
            }
        }

        if (!inserted && IRRU_InventoryRestoreSettings.IsDebugEnabled())
            Print("[InventoryCheck] [" + playerName + "] Failed to add " + itemLabel + " to player inventory");

        return inserted;
    }
}

class IRRU_InventoryRestoreClass : ScriptComponentClass
{
}