class StaticWeaponComponentClass: GameComponentClass
{
}

class StaticWeaponComponent: GameComponent
{
	[Attribute("", desc: "Weapon bag to spawn when disassembled", params: "et")]
	protected ResourceName m_rWeaponBagPrefab;
	
	[Attribute("", desc: "Bipod bag to spawn when disassembled.", params: "et")]
	protected ResourceName m_rBipodBagPrefab;
	
	protected bool isMoving = false;

	
	//Used to pull the weapon prefab from the bag
	ResourceName GetWeaponPrefab()
	{
		return m_rWeaponBagPrefab;
	}
	
	//Is the bag a bipod bag
	ResourceName GetBipodPrefab()
	{
		return m_rBipodBagPrefab;
	}
	
	void SetIsMoving(bool input)
	{
		isMoving = input;
	}
	
	bool GetIsMoving()
	{
		return isMoving;
	}
}