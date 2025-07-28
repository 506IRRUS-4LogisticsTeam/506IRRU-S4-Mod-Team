[ComponentEditorProps(category: "Game Mode Component", description: "")]
class StaticWeaponGamemodeComponentClass : SCR_BaseGameModeComponentClass
{
}

class StaticWeaponGamemodeComponent : SCR_BaseGameModeComponent
{
	static StaticWeaponGamemodeComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return StaticWeaponGamemodeComponent.Cast(gameMode.FindComponent(StaticWeaponGamemodeComponent));
		else
			return null;
	}
	
	void PlaceWeapon(RplId weaponID, vector previewVector[4])
	{
		RpcDo_PlaceWeapon(weaponID, previewVector);
		Rpc(RpcDo_PlaceWeapon, weaponID, previewVector);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlaceWeapon(RplId weaponID, vector previewVector[4])
	{
		Print("Placing weapons on clients");
		if(!GetGame())
			return;
		
		if(!GetGame().GetWorld())
			return;
		
		if(!Replication.FindItem(weaponID))
			return;

		GenericEntity weaponEntity = GenericEntity.Cast(RplComponent.Cast(Replication.FindItem(weaponID)).GetEntity()); 
		
		if(!weaponEntity)
			return;
		
		float scale = weaponEntity.GetScale();
		weaponEntity.SetWorldTransform(previewVector);
		weaponEntity.SetScale(scale);
		weaponEntity.Update();
		weaponEntity.OnTransformReset();
	}
	
	void SetIsMoving(bool input, RplId weaponID)
	{
		RpcDo_SetIsMoving(input, weaponID);
		Rpc(RpcDo_SetIsMoving, input, weaponID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SetIsMoving(bool input, RplId weaponID)
	{
		if(!GetGame())
			return;
		
		if(!GetGame().GetWorld())
			return;
		
		if(!Replication.FindItem(weaponID))
			return;
		
		StaticWeaponComponent.Cast(RplComponent.Cast(Replication.FindItem(weaponID)).GetEntity().FindComponent(StaticWeaponComponent)).SetIsMoving(input); 
	}
}