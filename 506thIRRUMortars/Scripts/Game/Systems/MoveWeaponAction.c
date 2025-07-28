class MoveWeaponAction: SCR_InventoryAction
{
	protected IEntity m_ePreviewEntity;
	protected ResourceName m_rWeaponPrefab;
	protected CameraBase m_pCharacterCamera;
	protected IEntity m_ePlayer;
	protected vector m_vOriginalPosition;
	protected IEntity m_eOriginalWeapon;
	
	override protected void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		m_ePlayer = pUserEntity;
		m_rWeaponPrefab = pOwnerEntity.GetPrefabData().GetPrefabName();
		m_vOriginalPosition = pOwnerEntity.GetOrigin();
		m_eOriginalWeapon = pOwnerEntity;

		GetGame().GetInputManager().ActivateContext("StaticWeaponContext", 99999);
		GetGame().GetInputManager().AddActionListener("PlaceStaticWeapon", EActionTrigger.PRESSED, PlaceWeaponAction);
		GetGame().GetInputManager().AddActionListener("CancelStaticWeapon", EActionTrigger.PRESSED, CancelWeaponAction);
		
		ClientStaticWeaponComponent.GetInstance().SetIsMoving(true, RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent)).Id());

		IntializePreview();
		
	}
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_eOriginalWeapon = pOwnerEntity;
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		if(SCR_BaseCompartmentManagerComponent.Cast(m_eOriginalWeapon.FindComponent(SCR_BaseCompartmentManagerComponent)).GetOccupantCount() > 0)
			return false;
		
		if(StaticWeaponComponent.Cast(m_eOriginalWeapon.FindComponent(StaticWeaponComponent)).GetIsMoving())
			return false;
		
		if(!SCR_CharacterControllerComponent.Cast(user.FindComponent(SCR_CharacterControllerComponent)).IsCharacterStationary())
			return false;
		
		return true;
	}
	
	//Spawns the preview entity and initializes everything
	protected void IntializePreview()
	{
		m_pCharacterCamera = GetGame().GetCameraManager().CurrentCamera();
		m_ePreviewEntity = SCR_PrefabPreviewEntity.SpawnPreviewFromPrefab(Resource.Load(m_rWeaponPrefab), "SCR_PrefabPreviewEntity", null, null, "{58F07022C12D0CF5}Assets/Editor/PlacingPreview/Preview.emat");
		GetGame().GetCallqueue().CallLater(Update, 100, true);
	}
	
	//Inspired from ACE Trenches
	//Updates the weapons position when being placed
	protected void UpdateWeaponPosition(out vector transform[4])
	{
		m_pCharacterCamera.GetWorldTransform(transform);
		vector vectorDir = transform[2];
		vectorDir[1] = 0;
		vectorDir.Normalize();
		Math3D.AnglesToMatrix(Vector(vectorDir.ToYaw(), 0, 0), transform);
		transform[3] = m_ePlayer.GetOrigin() + 1.5 * vectorDir;
		vector surface;
		SCR_TerrainHelper.SnapToGeometry(surface, transform[3], {}, GetGame().GetWorld());
		transform[3] = surface;
		SCR_TerrainHelper.OrientToTerrain(transform);
	}
	
	//Used to update the position of the weapon
	protected void Update()
	{
		if (!m_ePreviewEntity)
			return;
		
		if(SCR_CharacterControllerComponent.Cast(m_ePlayer.FindComponent(SCR_CharacterControllerComponent)).GetLifeState() == ECharacterLifeState.INCAPACITATED ||SCR_CharacterControllerComponent.Cast(m_ePlayer.FindComponent(SCR_CharacterControllerComponent)).GetLifeState() == ECharacterLifeState.DEAD)
		{
			CancelWeapon();
			return;
		}
		
		if(vector.Distance(m_eOriginalWeapon.GetOrigin(), m_ePlayer.GetOrigin()) > 10)
		{
			CancelWeapon();
			return;
		}
		
		vector transform[4];
		UpdateWeaponPosition(transform);
		m_ePreviewEntity.SetWorldTransform(transform);
		SCR_CharacterControllerComponent.Cast(m_ePlayer.FindComponent(SCR_CharacterControllerComponent)).GetStaminaComponent().AddStamina(-999);
		m_ePreviewEntity.Update();
	}
	
	//Calls PlaceWeapon to the Clients Player Controller Component
	protected void PlaceWeaponAction(float value, EActionTrigger reason)
	{
		PlaceWeapon();
	}
	
	protected void PlaceWeapon()
	{
		GetGame().GetCallqueue().Remove(Update);
		vector transform[4];
		UpdateWeaponPosition(transform);
		
		ClientStaticWeaponComponent.GetInstance().PlaceWeapon(RplComponent.Cast(m_eOriginalWeapon.FindComponent(RplComponent)).Id(), transform);
		GetGame().GetCallqueue().CallLater(DeleteWeapon, 250, false, m_ePreviewEntity);
		GetGame().GetInputManager().RemoveActionListener("CancelStaticWeapon", EActionTrigger.PRESSED, PlaceWeapon);
		GetGame().GetInputManager().RemoveActionListener("PlaceStaticWeapon", EActionTrigger.PRESSED, PlaceWeapon);
		GetGame().GetInputManager().ActivateContext("StaticWeaponContext");
		ClientStaticWeaponComponent.GetInstance().SetIsMoving(false, RplComponent.Cast(m_eOriginalWeapon.FindComponent(RplComponent)).Id());
	}
	
	protected void CancelWeaponAction(float value, EActionTrigger reason)
	{
		CancelWeapon();
	}
	
	protected void CancelWeapon()
	{
		GetGame().GetCallqueue().Remove(Update);
		GetGame().GetInputManager().RemoveActionListener("CancelStaticWeapon", EActionTrigger.PRESSED, PlaceWeapon);
		GetGame().GetInputManager().RemoveActionListener("PlaceStaticWeapon", EActionTrigger.PRESSED, PlaceWeapon);
		GetGame().GetInputManager().ActivateContext("StaticWeaponContext");
		ClientStaticWeaponComponent.GetInstance().SetIsMoving(false, RplComponent.Cast(m_eOriginalWeapon.FindComponent(RplComponent)).Id());
		delete m_ePreviewEntity;
	}	
	
	//Used to delay preview entities deletion
	protected void DeleteWeapon(IEntity entity)
	{
		delete entity;
	}
}