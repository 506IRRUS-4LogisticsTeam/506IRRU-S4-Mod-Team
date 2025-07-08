class IRRU_MortarComputerUserAction : ScriptedUserAction
{
    protected SCR_MapEntity m_MapEntity;
    protected IRRU_MortarArtilleryComputerComponent m_MortarComputer;
    
    //------------------------------------------------------------------------------------------------
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
        super.Init(pOwnerEntity, pManagerComponent);
        
        m_MortarComputer = IRRU_MortarArtilleryComputerComponent.Cast(pOwnerEntity.FindComponent(IRRU_MortarArtilleryComputerComponent));
        if (!m_MortarComputer)
            Print("'IRRU_MortarComputerUserAction': '" + pOwnerEntity.GetName() + "' is missing IRRU_MortarArtilleryComputerComponent!", LogLevel.WARNING);
        
        m_MapEntity = SCR_MapEntity.GetMapInstance();
    }
    
    //------------------------------------------------------------------------------------------------
    override bool CanBeShownScript(IEntity user)
    {
        // Try updating the map reference if not yet valid
        if (!m_MapEntity)
            m_MapEntity = SCR_MapEntity.GetMapInstance();
        
        // Must have valid components to show
        return m_MapEntity && m_MortarComputer;
    }
    
    //------------------------------------------------------------------------------------------------
    override bool CanBePerformedScript(IEntity user)
    {
        // Defensive check
        return m_MapEntity && m_MortarComputer;
    }
    
    //------------------------------------------------------------------------------------------------
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        super.PerformAction(pOwnerEntity, pUserEntity);
        
        // Ensure only local player triggers map opening
        if (SCR_PossessingManagerComponent.GetPlayerIdFromControlledEntity(pUserEntity) != SCR_PlayerController.GetLocalPlayerId())
            return;
        
        // Re-check map in case it's still null
        if (!m_MapEntity)
            m_MapEntity = SCR_MapEntity.GetMapInstance();
        
        // If map still not ready, retry after short delay
        if (!m_MapEntity)
        {
            Print("MapEntity still not initialized, retrying PerformAction in 100ms...", LogLevel.WARNING);
            GetGame().GetCallqueue().CallLater(PerformAction, 100, false, pOwnerEntity, pUserEntity);
            return;
        }
        
        m_MortarComputer.OpenComputer(m_MapEntity, pUserEntity);
    }
}