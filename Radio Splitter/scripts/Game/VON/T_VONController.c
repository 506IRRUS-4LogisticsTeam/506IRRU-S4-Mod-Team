modded class SCR_VONController
{
	protected ref map<SCR_VONEntry, float> m_EarAssignment; // Stores radio → ear assignments (-1: Left, 0: Center, 1: Right)
	protected ref array<SCR_VONEntry> m_ActiveEntries; // Active channels currently transmitting
	protected ref map<SCR_VONEntry, vector> m_OriginalPositions; // Stores original positions for reset

    //------------------------------------------------------------------------------------------------
    override protected void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        m_EarAssignment = new map<SCR_VONEntry, float>();
        m_ActiveEntries = new array<SCR_VONEntry>();
        m_OriginalPositions = new map<SCR_VONEntry, vector>(); // Initialize original positions storage

        // 🎮 Register Keybinds for Assigning Radios to Ears
        InputManager inputManager = GetGame().GetInputManager();
        if (inputManager)
        {
            inputManager.AddActionListener("RadioToLeftEar", EActionTrigger.DOWN, MoveActiveRadioLeft);
            inputManager.AddActionListener("RadioToCentre", EActionTrigger.DOWN, MoveActiveRadioCenter);
            inputManager.AddActionListener("RadioToRightEar", EActionTrigger.DOWN, MoveActiveRadioRight);
        }
    }

	//------------------------------------------------------------------------------------------------
	//! Assign a VON radio channel to a specific ear (-1 = Left, 0 = Center, 1 = Right)
	void AssignChannelToEar(SCR_VONEntry entry, float earSide)
	{
		if (!entry || entry == m_DirectSpeechEntry) // Ensure Direct Speech is NOT modified
			return;

		// Clamp between Left (-1), Center (0), Right (1)
		earSide = Math.Clamp(earSide, -1.0, 1.0);
		m_EarAssignment.Set(entry, earSide);
	}

	//------------------------------------------------------------------------------------------------
	//! Handle new radio transmission activation (ignores Direct Speech)
	void OnRadioTransmissionStart(SCR_VONEntry entry)
	{
		if (!entry || entry == m_DirectSpeechEntry || m_ActiveEntries.Contains(entry))
			return;

		m_ActiveEntries.Insert(entry);
		ApplyPositionalAudio(entry, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Handle radio transmission end (ignores Direct Speech)
	void OnRadioTransmissionEnd(SCR_VONEntry entry)
	{
		if (!entry || entry == m_DirectSpeechEntry || !m_ActiveEntries.Contains(entry))
			return;

		m_ActiveEntries.RemoveItem(entry);
		ApplyPositionalAudio(entry, false); // Reset position
	}

	//------------------------------------------------------------------------------------------------
	//! Apply 3D positional audio by shifting the radio entity slightly left or right
	void ApplyPositionalAudio(SCR_VONEntry entry, bool isActive)
	{
		// Ensure the entry is a radio entry
		SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
		if (!radioEntry) return;

		// Get the transceiver (radio) entity
		BaseTransceiver transceiver = radioEntry.GetTransceiver();
		if (!transceiver) return;

		// Get the radio entity
		IEntity radioEntity = transceiver.GetRadio().GetOwner();
		if (!radioEntity) return;

		// Get the player's position
		IEntity playerEntity = GetGame().GetPlayerController().GetControlledEntity();
		if (!playerEntity) return;

		// Get the radio's current position
		vector pos = radioEntity.GetOrigin();

		// Save original position if this is the first time adjusting it
		if (!m_OriginalPositions.Contains(entry))
			m_OriginalPositions.Set(entry, pos);

		// Determine ear assignment (default: Center)
		float earSide = 0.0;
		if (m_EarAssignment.Contains(entry))
			earSide = m_EarAssignment.Get(entry);

		// Apply a fixed positional shift of -0.3 for Left and 0.3 for Right
		float offsetAmount = 0.3;
		vector playerRight = playerEntity.GetTransformAxis(0); // Get right direction

		// Apply offset if active, reset if inactive
		if (isActive)
		{
			pos = pos + (playerRight * earSide * offsetAmount);
		}
		else
		{
			pos = m_OriginalPositions.Get(entry); // Reset to original position
			m_OriginalPositions.Remove(entry); // Remove from storage after reset
		}

		// Apply the new position
		radioEntity.SetOrigin(pos);

		// 🔹 FIXED: Correct way to get the radio name using SCR_VONEntryRadio
		UIInfo uiInfo = radioEntry.GetUIInfo();
		string radioName = "Unknown Radio"; // Default

		if (uiInfo)
		{
			radioName = uiInfo.GetName();
		}

		// Log debug info
		Print("Applying positional audio for " + radioName + " at offset " + (earSide * offsetAmount));
	}
	
	//------------------------------------------------------------------------------------------------
    //! Moves the currently selected radio's sound to the left ear (-0.3)
    void MoveActiveRadioLeft(float value, EActionTrigger reason)
    {
        SCR_VONEntry activeRadio = GetActiveEntry();
        if (!activeRadio) return;
        AssignChannelToEar(activeRadio, -0.3);
        Print("📻 Moved radio to LEFT ear (-0.3). ");
    }

    //! Moves the currently selected radio's sound to the center (0.0)
    void MoveActiveRadioCenter(float value, EActionTrigger reason)
    {
        SCR_VONEntry activeRadio = GetActiveEntry();
        if (!activeRadio) return;
        AssignChannelToEar(activeRadio, 0.0);
        Print("📻 Moved radio to CENTER (0.0). ");
    }

    //! Moves the currently selected radio's sound to the right ear (0.03)
    void MoveActiveRadioRight(float value, EActionTrigger reason)
    {
        SCR_VONEntry activeRadio = GetActiveEntry();
        if (!activeRadio) return;
        AssignChannelToEar(activeRadio, 0.03);
        Print("📻 Moved radio to RIGHT ear (0.03). ");
    }

	//------------------------------------------------------------------------------------------------
	//! Clear active radio transmissions on player death/disconnect
	void ResetStereoVON()
	{
		foreach (SCR_VONEntry entry : m_ActiveEntries)
		{
			ApplyPositionalAudio(entry, false); // Reset positions
		}
		m_ActiveEntries.Clear();
		m_OriginalPositions.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Override existing transmission methods (Only for Radio Transmissions)
	override protected void ActivateVON(EVONTransmitType transmitType)
	{
		super.ActivateVON(transmitType);

		// Only modify radio transmissions, ignore Direct Speech
		if (transmitType == EVONTransmitType.CHANNEL || transmitType == EVONTransmitType.LONG_RANGE)
		{
			SCR_VONEntry entry = GetEntryByTransmitType(transmitType);
			if (entry)
				OnRadioTransmissionStart(entry);
		}
	}

	override protected void DeactivateVON(EVONTransmitType transmitType = EVONTransmitType.NONE)
	{
		super.DeactivateVON(transmitType);

		// Only modify radio transmissions, ignore Direct Speech
		if (transmitType == EVONTransmitType.CHANNEL || transmitType == EVONTransmitType.LONG_RANGE)
		{
			SCR_VONEntry entry = GetEntryByTransmitType(transmitType);
			if (entry)
				OnRadioTransmissionEnd(entry);
		}
	}
}
