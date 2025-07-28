class StaticWeaponBagComponentClass: GameComponentClass
{
}

class StaticWeaponBagComponent: GameComponent
{
	[Attribute("0", desc: "If marked true this bag will be used as a bipod bag in the assembly of a static weapon.")]
	protected bool m_bIsBipod;
	
	[Attribute("", desc: "The static weapon that will spawn when assembled.", params: "et")]
	protected ResourceName m_rWeaponPrefab;
	
	protected int currentMagazineAmmoCount = -1;
	
	protected ref array<string> inventoryItems = {};
	
	//Used to pull the weapon prefab from the bag
	ResourceName GetWeaponPrefab()
	{
		return m_rWeaponPrefab;
	}
	
	//Is the bag a bipod bag
	bool IsBipod()
	{
		return m_bIsBipod;
	}
	
	void SetAmmoCount(int ammoCount)
	{
		currentMagazineAmmoCount = ammoCount;
	}
	
	int GetAmmoCount()
	{
		return currentMagazineAmmoCount;
	}
	
	array<string> GetInventoryItems()
	{
		return inventoryItems;
	}
	
	void ClearInventoryItems()
	{
		inventoryItems = {};
	}
	
	void SetInventoryItems(array<string> items)
	{
		inventoryItems = items;
	}
}