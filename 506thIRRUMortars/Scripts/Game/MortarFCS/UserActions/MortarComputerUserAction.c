class IRRU_MortarComputerUserAction : ScriptedUserAction
{
    protected SCR_MapEntity m_MapEntity;
    protected IRRU_MortarArtilleryComputerComponent m_MortarComputer;
    
    //------------------------------------------------------------------------------------------------
    override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
    {
        Print("[MORTAR] UserAction Init - Starting");
        super.Init(pOwnerEntity, pManagerComponent);
        
        Print(string.Format("[MORTAR] UserAction Init - Owner entity: %1", pOwnerEntity));
        Print(string.Format("[MORTAR] UserAction Init - Owner entity name: %1", pOwnerEntity.GetName()));
        
        m_MortarComputer = IRRU_MortarArtilleryComputerComponent.Cast(pOwnerEntity.FindComponent(IRRU_MortarArtilleryComputerComponent));
        if (!m_MortarComputer)
        {
            Print("[MORTAR] UserAction Init - ERROR: IRRU_MortarArtilleryComputerComponent NOT FOUND!");
            Print(string.Format("[MORTAR] UserAction Init - Entity '%1' is missing IRRU_MortarArtilleryComputerComponent!", pOwnerEntity.GetName()));
        }
        else
        {
            Print("[MORTAR] UserAction Init - SUCCESS: Found IRRU_MortarArtilleryComputerComponent");
        }
        
        m_MapEntity = SCR_MapEntity.GetMapInstance();
        if (!m_MapEntity)
        {
            Print("[MORTAR] UserAction Init - WARNING: MapEntity is null at init");
        }
        else
        {
            Print("[MORTAR] UserAction Init - SUCCESS: MapEntity found at init");
        }
        
        Print("[MORTAR] UserAction Init - Completed");
    }
    
    //------------------------------------------------------------------------------------------------
    override bool CanBeShownScript(IEntity user)
    {
       // Print("[MORTAR] CanBeShownScript - Called");
        //Print(string.Format("[MORTAR] CanBeShownScript - User entity: %1", user));
        
        // Try updating the map reference if not yet valid
        if (!m_MapEntity)
        {
            //Print("[MORTAR] CanBeShownScript - MapEntity is null, trying to get instance");
            m_MapEntity = SCR_MapEntity.GetMapInstance();
            
            if (!m_MapEntity)
            {
               //Print("[MORTAR] CanBeShownScript - MapEntity still null after GetInstance");
            }
            else
            {
                //Print("[MORTAR] CanBeShownScript - MapEntity successfully obtained");
            }
        }
        else
        {
            //Print("[MORTAR] CanBeShownScript - MapEntity already exists");
        }
        
        bool hasMap = false;
        bool hasComponent = false;
        
        if (m_MapEntity)
            hasMap = true;
            
        if (m_MortarComputer)
            hasComponent = true;
        
        //Print(string.Format("[MORTAR] CanBeShownScript - HasMap: %1", hasMap));
        //Print(string.Format("[MORTAR] CanBeShownScript - HasComponent: %1", hasComponent));
        
        bool result = hasMap && hasComponent;
        //Print(string.Format("[MORTAR] CanBeShownScript - Returning: %1", result));
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    override bool CanBePerformedScript(IEntity user)
    {
        //Print("[MORTAR] CanBePerformedScript - Called");
        
        bool hasMap = false;
        bool hasComponent = false;
        
        if (m_MapEntity)
            hasMap = true;
            
        if (m_MortarComputer)
            hasComponent = true;
            
        //Print(string.Format("[MORTAR] CanBePerformedScript - HasMap: %1, HasComponent: %2", hasMap, hasComponent));
        
        bool result = hasMap && hasComponent;
        //Print(string.Format("[MORTAR] CanBePerformedScript - Returning: %1", result));
        
        return result;
    }
    
    //------------------------------------------------------------------------------------------------
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        Print("[MORTAR] PerformAction - Starting");
        Print(string.Format("[MORTAR] PerformAction - Owner: %1, User: %2", pOwnerEntity, pUserEntity));
        
        super.PerformAction(pOwnerEntity, pUserEntity);
        
        // Ensure only local player triggers map opening
        int playerId = SCR_PossessingManagerComponent.GetPlayerIdFromControlledEntity(pUserEntity);
        int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
        
        Print(string.Format("[MORTAR] PerformAction - Player ID: %1, Local ID: %2", playerId, localPlayerId));
        
        if (playerId != localPlayerId)
        {
            Print("[MORTAR] PerformAction - Not local player, aborting");
            return;
        }
        
        // Re-check map in case it's still null
        if (!m_MapEntity)
        {
            Print("[MORTAR] PerformAction - MapEntity is null, trying to get instance");
            m_MapEntity = SCR_MapEntity.GetMapInstance();
        }
        
        // If map still not ready, retry after short delay
        if (!m_MapEntity)
        {
            Print("[MORTAR] PerformAction - ERROR: MapEntity still not initialized, retrying in 100ms");
            GetGame().GetCallqueue().CallLater(PerformAction, 100, false, pOwnerEntity, pUserEntity);
            return;
        }
        
        if (!m_MortarComputer)
        {
            Print("[MORTAR] PerformAction - ERROR: MortarComputer component is null!");
            return;
        }
        
        Print("[MORTAR] PerformAction - Calling OpenComputer");
        m_MortarComputer.OpenComputer(m_MapEntity, pUserEntity);
        Print("[MORTAR] PerformAction - Completed");
    }
}