//! User action for accessing mortar computer interface
class IRRU_MortarComputerUserAction : ScriptedUserAction
{
    protected SCR_MapEntity m_MapEntity;
    protected IRRU_MortarArtilleryComputerComponent m_MortarComputer;
    
    protected const int RETRY_DELAY_MS = 100;
    
    //------------------------------------------------------------------------------------------------
    //! Initialize user action
    //! \param pOwnerEntity Entity that owns this action
    //! \param pManagerComponent Component manager
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
        super.Init(pOwnerEntity, pManagerComponent);
        
        m_MortarComputer = IRRU_MortarArtilleryComputerComponent.Cast(
            pOwnerEntity.FindComponent(IRRU_MortarArtilleryComputerComponent)
        );
        
        m_MapEntity = SCR_MapEntity.GetMapInstance();
    }
    
    //------------------------------------------------------------------------------------------------
    //! Check if action can be shown to user
    //! \param user Entity attempting to use the action
    //! \return True if action should be visible
    override bool CanBeShownScript(IEntity user)
    {
        if (!m_MapEntity)
            m_MapEntity = SCR_MapEntity.GetMapInstance();
        
        return m_MapEntity && m_MortarComputer;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Check if action can be performed
    //! \param user Entity attempting to perform the action
    //! \return True if action can be executed
    override bool CanBePerformedScript(IEntity user)
    {
        return m_MapEntity && m_MortarComputer;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Execute the mortar computer action
    //! \param pOwnerEntity Entity that owns this action
    //! \param pUserEntity Entity using the action
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        super.PerformAction(pOwnerEntity, pUserEntity);
        
        if (!IsLocalPlayer(pUserEntity))
            return;
        
        if (!EnsureMapEntity())
        {
            RetryAction(pOwnerEntity, pUserEntity);
            return;
        }
        
        if (!m_MortarComputer)
            return;
        
        m_MortarComputer.OpenComputer(m_MapEntity, pUserEntity);
    }
    
    //------------------------------------------------------------------------------------------------
    //! Check if user is the local player
    //! \param pUserEntity Entity to check
    //! \return True if entity is controlled by local player
    protected bool IsLocalPlayer(IEntity pUserEntity)
    {
        int playerId = SCR_PossessingManagerComponent.GetPlayerIdFromControlledEntity(pUserEntity);
        int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
        
        return playerId == localPlayerId;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Ensure map entity is available
    //! \return True if map entity is ready
    protected bool EnsureMapEntity()
    {
        if (!m_MapEntity)
            m_MapEntity = SCR_MapEntity.GetMapInstance();
        
        return m_MapEntity != null;
    }
    
    //------------------------------------------------------------------------------------------------
    //! Retry action after delay
    //! \param pOwnerEntity Entity that owns this action
    //! \param pUserEntity Entity using the action
    protected void RetryAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        GetGame().GetCallqueue().CallLater(PerformAction, RETRY_DELAY_MS, false, pOwnerEntity, pUserEntity);
    }
}