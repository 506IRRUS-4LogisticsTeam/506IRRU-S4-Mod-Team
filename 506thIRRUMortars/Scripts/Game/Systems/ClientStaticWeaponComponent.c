[EntityEditorProps(description: "Assembly of static weapons communication to the server.")]
class ClientStaticWeaponComponentClass : ScriptComponentClass
{
}

class ClientStaticWeaponComponent : ScriptComponent
{
	protected IEntity m_eBipodBag;
	protected IEntity m_eWeaponBag;
	protected IEntity m_eCurrentWeapon
	protected ECharacterLifeState m_lCurrentLifeStatus = ECharacterLifeState.ALIVE;
		
	//I FUCKING LOVE GETINSTANCE
	static ClientStaticWeaponComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return ClientStaticWeaponComponent.Cast(GetGame().GetPlayerController().FindComponent(ClientStaticWeaponComponent));
		else
			return null;
	}
	
	ECharacterLifeState GetLifeStatus()
	{
		return m_lCurrentLifeStatus;
	}
	
	void SetIsMoving(bool input, RplId weaponID)
	{
		Rpc(RpcDo_SetIsMoving, input, weaponID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_SetIsMoving(bool input, RplId weaponID)
	{
		StaticWeaponGamemodeComponent.Cast(GetGame().GetGameMode().FindComponent(StaticWeaponGamemodeComponent)).SetIsMoving(input, weaponID);
	}
	
	//From the client -> Server to place the weapon
	void PlaceWeapon(RplId weaponID, vector previewVector[4])
	{
		Rpc(RpcDo_ServerPlaceWeapon, weaponID, previewVector);
	}
	
	//Calls to the server so it can then broadcast the movement of the weapon
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_ServerPlaceWeapon(RplId weaponID, vector previewVector[4])
	{
		StaticWeaponGamemodeComponent.Cast(GetGame().GetGameMode().FindComponent(StaticWeaponGamemodeComponent)).PlaceWeapon(weaponID, previewVector);
	}
	
	//From the client -> Server to spawn the weapon
	void SpawnWeapon(RplId weaponBagID, RplId bipodBagID, vector playerRotation)
	{
		Rpc(RpcDo_SpawnWeapon, weaponBagID, bipodBagID, playerRotation);
	}
	
	//No need to broadcast the spawning of a weapon
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_SpawnWeapon(RplId weaponBagID, RplId bipodBagID, vector playerRotation)
	{
		m_eBipodBag = null;
		m_eWeaponBag = null;
		m_eBipodBag = RplComponent.Cast(Replication.FindItem(bipodBagID)).GetEntity(); 
		m_eWeaponBag = RplComponent.Cast(Replication.FindItem(weaponBagID)).GetEntity(); 
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = m_eWeaponBag.GetOrigin();
		
		IEntity weapon = GetGame().SpawnEntityPrefab(Resource.Load(StaticWeaponBagComponent.Cast(m_eWeaponBag.FindComponent(StaticWeaponBagComponent)).GetWeaponPrefab()),GetGame().GetWorld(),spawnParams);
		BaseMagazineComponent magazine = BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent)).GetCurrentMagazine();
		
		if(m_eWeaponBag.FindComponent(BaseWeaponComponent))
		{
			if(BaseWeaponComponent.Cast(m_eWeaponBag.FindComponent(BaseWeaponComponent)).GetCurrentMagazine())
			{	
				if(m_eWeaponBag.FindComponent(BaseWeaponComponent))
					magazine.SetAmmoCount(BaseWeaponComponent.Cast(m_eWeaponBag.FindComponent(BaseWeaponComponent)).GetCurrentMagazine().GetAmmoCount());
			}
			else
			{
				delete magazine.GetOwner();
			}
		}
		
		if(StaticWeaponBagComponent.Cast(m_eWeaponBag.FindComponent(StaticWeaponBagComponent)).GetAmmoCount() != -1 && !m_eWeaponBag.FindComponent(BaseWeaponComponent))
			magazine.SetAmmoCount(StaticWeaponBagComponent.Cast(m_eWeaponBag.FindComponent(StaticWeaponBagComponent)).GetAmmoCount());
				
		ref array<string> inventoryItems = StaticWeaponBagComponent.Cast(m_eWeaponBag.FindComponent(StaticWeaponBagComponent)).GetInventoryItems();
		if(inventoryItems.Count() > 0)
		{
			SCR_UniversalInventoryStorageComponent inventoryComponent = SCR_UniversalInventoryStorageComponent.Cast(weapon.FindComponent(SCR_UniversalInventoryStorageComponent));
			//Clears the inventory
			ref array<IEntity> weaponMagazines = {};
			inventoryComponent.GetAll(weaponMagazines);
			foreach(IEntity magazineStorage: weaponMagazines)
			{
				delete magazineStorage;
			}
			
			int iteration = 0;
			InventoryStorageManagerComponent inventoryManager = InventoryStorageManagerComponent.Cast(weapon.FindComponent(InventoryStorageManagerComponent));
			for(int i = 0; i < inventoryItems.Count()/2; i++)
			{
				IEntity magazineObject = GetGame().SpawnEntityPrefab(Resource.Load(inventoryItems.Get(0 + iteration)),GetGame().GetWorld(),spawnParams);
				if(inventoryItems.Get(1 + iteration).ToInt() != -1)
					BaseMagazineComponent.Cast(magazineObject.FindComponent(BaseMagazineComponent)).SetAmmoCount(inventoryItems.Get(1 + iteration).ToInt());
				Print(inventoryManager.TryInsertItem(magazineObject));
				iteration = iteration + 2;
			}	
		}
		

		
		weapon.SetAngles(playerRotation);
		delete m_eBipodBag;
		delete m_eWeaponBag;
		
	}
	
	void DisassembleWeapon(RplId playerID, RplId weaponID, ResourceName weaponBag, ResourceName bipodBag, vector cameraTransform[4])
	{
		Rpc(RpcDo_DisassembleWeapon,playerID, weaponID, weaponBag, bipodBag, cameraTransform);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_DisassembleWeapon(RplId playerID, RplId weaponID, ResourceName weaponBag, ResourceName bipodBag, vector cameraTransform[4])
	{
		IEntity weapon = RplComponent.Cast(Replication.FindItem(weaponID)).GetEntity();
		IEntity player = RplComponent.Cast(Replication.FindItem(playerID)).GetEntity();
		
		vector vectorDir = cameraTransform[2];
		vectorDir[1] = 0;
		vectorDir.Normalize();
		Math3D.AnglesToMatrix(Vector(vectorDir.ToYaw(), 0, 0), cameraTransform);
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = player.GetOrigin() + 1 * vectorDir;
		IEntity weaponBagEntity;
		weaponBagEntity = GetGame().SpawnEntityPrefab(Resource.Load(weaponBag),GetGame().GetWorld(),spawnParams);
		
		if(BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent)).GetCurrentMagazine())
			StaticWeaponBagComponent.Cast(weaponBagEntity.FindComponent(StaticWeaponBagComponent)).SetAmmoCount(BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent)).GetCurrentMagazine().GetAmmoCount());
		else
			StaticWeaponBagComponent.Cast(weaponBagEntity.FindComponent(StaticWeaponBagComponent)).SetAmmoCount(0);
		
		if(weaponBagEntity.FindComponent(BaseWeaponComponent))
		{
			if(BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent)).GetCurrentMagazine())
				BaseWeaponComponent.Cast(weaponBagEntity.FindComponent(BaseWeaponComponent)).GetCurrentMagazine().SetAmmoCount(BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent)).GetCurrentMagazine().GetAmmoCount());
			else
				delete BaseWeaponComponent.Cast(weaponBagEntity.FindComponent(BaseWeaponComponent)).GetCurrentMagazine().GetOwner();
		}
		spawnParams.Transform[3] = player.GetOrigin() + 2 * vectorDir;
		IEntity bipodBagEntity = GetGame().SpawnEntityPrefab(Resource.Load(bipodBag),GetGame().GetWorld(),spawnParams);		
		
		SCR_UniversalInventoryStorageComponent inventoryComponent = SCR_UniversalInventoryStorageComponent.Cast(weapon.FindComponent(SCR_UniversalInventoryStorageComponent));
		ref array<IEntity> weaponMagazines = {};
		ref array<string> exportArray = {};
		inventoryComponent.GetAll(weaponMagazines);
		foreach(IEntity magazineStorage: weaponMagazines)
		{
			exportArray.Insert(magazineStorage.GetPrefabData().GetPrefabName());
			
			if(magazineStorage.FindComponent(BaseMagazineComponent))
				exportArray.Insert(BaseMagazineComponent.Cast(magazineStorage.FindComponent(BaseMagazineComponent)).GetAmmoCount().ToString());
			else
				exportArray.Insert("-1");
		}

		StaticWeaponBagComponent.Cast(weaponBagEntity.FindComponent(StaticWeaponBagComponent)).SetInventoryItems(exportArray);
		
		delete weapon;
	}
}