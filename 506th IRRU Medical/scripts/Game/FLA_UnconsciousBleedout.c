modded class SCR_CharacterDamageManagerComponent : SCR_DamageManagerComponent
{
    protected float m_FLA_Medical_BleedOutTime = 60.0;  // Bleed-out time (configurable)
	protected float m_fTimeSinceUnconscious = 0.0;  // Timer to track bleed-out
	protected bool m_bInvulnerable = false;  // Store invulnerability state    
    protected bool m_bIsUnconscious = false;  // To track if the character is unconscious
    protected bool m_bIsBeingCarried = false; // To track if the character is being carried
   

    // Health components
    protected HitZone m_pACE_Medical_HealthHitZone;
    protected float m_fACE_Medical_CriticalHealth;
    protected ref array<HitZone> m_aACE_Medical_PhysicalHitZones = {};

    //-----------------------------------------------------------------------------------------------------------
    //! Initialize member variables
    override void OnInit(IEntity owner)
    {
        super.OnInit(owner);
        
        // Initialize the health hit zone and physical zones
        m_pACE_Medical_HealthHitZone = GetHitZoneByName("Health");
        if (!m_pACE_Medical_HealthHitZone)
            return;

        m_fACE_Medical_CriticalHealth = m_pACE_Medical_HealthHitZone.GetDamageStateThreshold(ECharacterHealthState.CRITICAL);
        GetPhysicalHitZones(m_aACE_Medical_PhysicalHitZones);  // Initialize physical hit zones for damage calculations
    }

    //-----------------------------------------------------------------------------------------------------------
    //! Renamed method to avoid conflicts
    void TriggerUnconsciousness()
    {
        m_bIsUnconscious = true;
        m_fTimeSinceUnconscious = 0.0; // Reset the bleed-out timer

        // Additional logic to handle unconsciousness, if any
    }

    //-----------------------------------------------------------------------------------------------------------
    //! Called to update the state of unconsciousness and handle bleed-out
    void UpdateUnconsciousState(float deltaTime)
    {
        if (m_bIsUnconscious)
        {
            m_fTimeSinceUnconscious += deltaTime;
            
            if (m_fTimeSinceUnconscious >= m_FLA_Medical_BleedOutTime)
            {
                // Trigger death or any bleed-out-related logic
                OnBleedOut();
            }
        }
    }

    //-----------------------------------------------------------------------------------------------------------
    //! Trigger death or other consequences when the bleed-out timer expires
    void OnBleedOut()
    {
        // Get the owner of this component (the character entity)
        IEntity owner = GetOwner();

        // Check if the owner is valid and has the health hit zone component
        if (owner)
        {
            // Get the health hit zone from the owner entity
            SCR_CharacterHealthHitZone healthHitZone = SCR_CharacterHealthHitZone.Cast(owner.FindComponent(SCR_CharacterHealthHitZone));

            // If the health hit zone is found, set health to 0 to simulate death
            if (healthHitZone)
            {
		

				
                // Set health to zero (character dies when health reaches 0)
                healthHitZone.SetHealth(0);  // This should trigger death or a state change to dead
                Print("BLUFOR Member has bled out and died");
                
                
            }
            else
            {
                Print("Error: Health hit zone not found on the character.");
            }
        }
        else
        {
            Print("Error: Owner entity is not valid.");
        }
    }
    //-----------------------------------------------------------------------------------------------------------
  // Function to set invulnerability based on the player's state (unconscious AND being carried)
	void SetInvulnerable(bool isInvulnerable)
	{
    // Get the current entity (player/character)
    	IEntity owner = GetOwner(); // Replace with your method for obtaining the entity

    	if (owner) 
   		{
        // Access the damage manager or health component
        SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));

        if (damageManager)
        {
            // Check if the player is unconscious (use the correct component to access the state)
         

            // Only make the character invulnerable if they are both unconscious and being carried
            if (m_bIsUnconscious && m_bIsBeingCarried)
            {
                damageManager.SetInvulnerable(true); // Disable damage when both unconscious and being carried
                Print("Character is unconscious and being carried. Invulnerability enabled.");
            }
            else
            {
                damageManager.SetInvulnerable(false); // Allow damage when the character is not unconscious or not being carried
                Print("Character is not unconscious and/or not being carried. Invulnerability disabled.");
            }
        }
    }
}

// Function to handle the being carried state and set invulnerability accordingly
	void SetBeingCarried(bool isBeingCarried)
	{
    m_bIsBeingCarried = isBeingCarried;
    
    // Instead of using GetComponent, we will use FindComponent to get the character's controller
    SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(GetOwner().FindComponent(SCR_CharacterControllerComponent));

    if (controller)
    {
        controller.ACE_Carrying_SetIsCarried(isBeingCarried);  // Set the character's carrying status
    }

    // Update invulnerability based on both unconscious and being carried states
    SetInvulnerable(true); // Check invulnerability status based on both unconscious and carried states
	}

    //-----------------------------------------------------------------------------------------------------------
    //! Check if the character is unconscious
    bool IsUnconscious()
    {
        return m_bIsUnconscious;
    }

    //-----------------------------------------------------------------------------------------------------------
    //! Update method for each frame to check for unconscious state and update bleed-out timer
    void Update(float deltaTime)
    {
        // If the base class does not have Update, you can use this method to manually call `UpdateUnconsciousState` here
        UpdateUnconsciousState(deltaTime);
    }
}
