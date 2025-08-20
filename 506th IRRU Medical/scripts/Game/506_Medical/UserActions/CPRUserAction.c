//! CPR user action for stabilizing unconscious players

//! Performs CPR to stabilize unconscious patients
class IRRU_CPRUserAction : ScriptedUserAction
{
	// Animation positioning constants
	protected static const ref array<float> CPR_PERFORMER_ANGLES = {90, -90};
	protected static const ref array<vector> CPR_PERFORMER_OFFSETS = {{-0.3, 0, -0.8}, {0.3, 0, -0.8}};
	protected static const vector CPR_PERFORMER_BB_OFFSET = {0, 0.45, 0};
	protected static const vector CPR_PERFORMER_BB_HALF_EXTENDS = {0.15, 0.15, 0.15};
	
	// Fatigue system constants
	protected static const float CPR_MAX_DURATION = 30.0;  // 30 seconds max CPR before fatigue
	protected static const float CPR_BASE_COOLDOWN = 12.0; // 12 seconds base cooldown
	protected static const float CPR_COOLDOWN_RATIO = 0.4; // 40% of time spent = cooldown (12s/30s)
	
	// Configurable attributes
	[Attribute(defvalue: "3", desc: "Maximum distance to perform CPR in meters", params: "1 5 0.5", category: "CPR Settings")]
	protected float m_fMaxDistance;
	
	// Instance tracking
	protected int m_iPerformingPlayerId = -1;
	protected IRRU_CPRHelperCompartment m_pActiveHelper;
	protected bool m_bCPRActive = false;
	
	// Fatigue tracking
	protected float m_fCPRStartTime = 0;
	protected ref map<int, float> m_mPlayerCooldowns = new map<int, float>();  // PlayerId -> cooldown end time
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		
		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print("[NoInstantDeath][CPR] CPR action initialized");
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!user)
			return false;
			
		IEntity owner = GetOwner();
		if (!owner)
			return false;
		
		// Check if target is a character
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(owner);
		if (!character)
			return false;
		
		// Check if character has NoInstantDeath component and is unconscious
		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));
		if (!nid || !nid.IsUnconscious())
			return false;
		
		// Check if patient is in a vehicle (can't perform CPR in vehicles)
		CompartmentAccessComponent compartment = CompartmentAccessComponent.Cast(character.FindComponent(CompartmentAccessComponent));
		if (compartment && compartment.IsInCompartment())
			return false;
		
		// Check if animation positions are blocked
		if (!CheckAnimationPositionClear(owner, user))
			return false;
		
		// Check if patient is on their back (required for CPR)
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (controller && controller.ACE_Medical_GetUnconsciousPose() != ACE_Medical_EUnconsciousPose.BACK)
			return false;
		
		// Check distance (must be close)
		vector userPos = user.GetOrigin();
		vector targetPos = owner.GetOrigin();
		float distance = vector.Distance(userPos, targetPos);
		
		if (distance > m_fMaxDistance)
			return false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!user)
			return false;
			
		IEntity owner = GetOwner();
		if (!owner)
			return false;
		
		// Get player ID for tracking who's performing
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;
			
		int playerId = playerManager.GetPlayerIdFromControlledEntity(user);
		
		// Check if player is on cooldown
		if (m_mPlayerCooldowns.Contains(playerId))
		{
			float currentTime = GetGame().GetWorld().GetWorldTime();
			float cooldownEndTime = m_mPlayerCooldowns.Get(playerId);
			
			if (currentTime < cooldownEndTime)
			{
				float remainingCooldown = (cooldownEndTime - currentTime) / 1000.0;
				SetCannotPerformReason(string.Format("Resting - %1s remaining", Math.Ceil(remainingCooldown)));
				return false;
			}
			else
			{
				// Cooldown expired, remove from map
				m_mPlayerCooldowns.Remove(playerId);
			}
		}
		
		// Check if someone else is already doing CPR
		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));
		if (nid && nid.IsReceivingCPR())
		{
			// Check if WE are the ones performing CPR - if so, this action will STOP CPR
			if (m_bCPRActive && m_iPerformingPlayerId == playerId)
			{
				// We're doing CPR - allow action to stop it
				return true;
			}
			else
			{
				SetCannotPerformReason("Another medic is already performing CPR");
				return false;
			}
		}
		
		// Check if patient is still unconscious
		if (nid && !nid.IsUnconscious())
		{
			SetCannotPerformReason("Patient is conscious");
			return false;
		}
		
		// Check if we're already in animation
		if (IRRU_AnimationTools.GetHelperCompartment(user))
			return true; // Already animating, allow continuation
		
		// Check if patient is in a vehicle (can't perform CPR in vehicles)
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(owner);
		if (character)
		{
			CompartmentAccessComponent compartment = CompartmentAccessComponent.Cast(character.FindComponent(CompartmentAccessComponent));
			if (compartment && compartment.IsInCompartment())
			{
				SetCannotPerformReason("Cannot perform CPR in vehicle");
				return false;
			}
			
			// Check if patient is on their back (required for CPR)
			SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
			if (controller && controller.ACE_Medical_GetUnconsciousPose() != ACE_Medical_EUnconsciousPose.BACK)
			{
				SetCannotPerformReason("Patient must be on their back for CPR");
				return false;
			}
			
			// Check animation position is clear
			if (!CheckAnimationPositionClear(owner, user))
			{
				SetCannotPerformReason("Position obstructed");
				return false;
			}
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		// Get current user for cooldown check
		IEntity user = GetGame().GetPlayerController().GetControlledEntity();
		if (user)
		{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			if (playerManager)
			{
				int playerId = playerManager.GetPlayerIdFromControlledEntity(user);
				
				// Check if player is on cooldown
				if (m_mPlayerCooldowns.Contains(playerId))
				{
					float currentTime = GetGame().GetWorld().GetWorldTime();
					float cooldownEndTime = m_mPlayerCooldowns.Get(playerId);
					
					if (currentTime < cooldownEndTime)
					{
						float remainingCooldown = (cooldownEndTime - currentTime) / 1000.0;
						outName = string.Format("CPR Cooldown (%1s)", Math.Ceil(remainingCooldown));
						return true;
					}
				}
			}
		}
		
		// Check if we're currently performing CPR
		if (m_bCPRActive && m_fCPRStartTime > 0)
		{
			float currentTime = GetGame().GetWorld().GetWorldTime();
			float elapsedTime = (currentTime - m_fCPRStartTime) / 1000.0;
			float remainingTime = CPR_MAX_DURATION - elapsedTime;
			
			if (remainingTime > 0)
				outName = string.Format("Stop CPR (%1s)", Math.Ceil(remainingTime));
			else
				outName = "Stop CPR";
		}
		else
		{
			outName = "Start CPR";
		}
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	// OnActionStart is not needed for instant actions - PerformAction is called directly
	
	//------------------------------------------------------------------------------------------------
	protected void OnAnimationTerminated()
	{
		// Animation ended externally (e.g., performer died, disconnected, etc.)
		if (m_pActiveHelper)
		{
			IEntity patient = m_pActiveHelper.GetPatient();
			IEntity performer = m_pActiveHelper.GetPerformer();
			
			if (patient && performer)
				StopCPR(patient, performer);
			
			m_pActiveHelper = null;
		}
		
		m_bCPRActive = false;
	}
	
	//------------------------------------------------------------------------------------------------
	// Auto-stop CPR when fatigue limit reached
	protected void AutoStopCPRDueToFatigue()
	{
		if (!m_bCPRActive || !m_pActiveHelper)
			return;
			
		IEntity patient = m_pActiveHelper.GetPatient();
		IEntity performer = m_pActiveHelper.GetPerformer();
		
		if (patient && performer)
		{
			StopCPR(patient, performer, true);
			
			// Show feedback to performer
			if (IsLocalPlayer(performer))
			{
				SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
				if (hintManager)
					hintManager.ShowCustom("CPR stopped - Fatigue limit reached", "Medical", 3.0);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Stop CPR manually
	protected void StopCPR(IEntity pOwnerEntity, IEntity pUserEntity, bool wasForcedByFatigue = false)
	{
		// Calculate cooldown based on time spent doing CPR
		if (m_iPerformingPlayerId != -1 && m_fCPRStartTime > 0)
		{
			float currentTime = GetGame().GetWorld().GetWorldTime();
			float cprDuration = (currentTime - m_fCPRStartTime) / 1000.0; // Convert to seconds
			
			// Calculate proportional cooldown
			float cooldownTime;
			if (wasForcedByFatigue)
			{
				// Full cooldown if forced stop due to fatigue
				cooldownTime = CPR_BASE_COOLDOWN;
			}
			else
			{
				// Proportional cooldown: 40% of time spent
				cooldownTime = cprDuration * CPR_COOLDOWN_RATIO;
				// Minimum 2 seconds, maximum base cooldown
				cooldownTime = Math.Clamp(cooldownTime, 2.0, CPR_BASE_COOLDOWN);
			}
			
			// Set cooldown end time for this player
			float cooldownEndTime = currentTime + (cooldownTime * 1000.0);
			m_mPlayerCooldowns.Set(m_iPerformingPlayerId, cooldownEndTime);
			
			if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
				Print(string.Format("[CPR] Applied %1s cooldown after %2s of CPR", cooldownTime, cprDuration));
		}
		
		// Cancel scheduled auto-stop if exists
		GetGame().GetCallqueue().Remove(AutoStopCPRDueToFatigue);
		
		// Terminate animation if active
		if (m_pActiveHelper)
		{
			m_pActiveHelper.GetOnTerminated().Remove(OnAnimationTerminated);
			m_pActiveHelper.Terminate(EGetOutType.ANIMATED);
			m_pActiveHelper = null;
		}
		
		// Clear CPR flag on patient - timer resumes
		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(pOwnerEntity.FindComponent(IRRU_NoInstantDeathComponent));
		if (nid)
		{
			nid.SetReceivingCPR(false);
			
			if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
				Print("[NoInstantDeath][CPR] Cleared receiving CPR flag on patient");
		}
		
		m_bCPRActive = false;
		m_fCPRStartTime = 0;
		m_iPerformingPlayerId = -1;
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// Store who's performing
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(pUserEntity);
			
			// Check if this is the same player stopping their CPR
			if (m_bCPRActive && m_iPerformingPlayerId == playerId)
			{
				StopCPR(pOwnerEntity, pUserEntity);
				return;
			}
			
			m_iPerformingPlayerId = playerId;
		}
		
		// Check if patient has component
		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(pOwnerEntity.FindComponent(IRRU_NoInstantDeathComponent));
		if (!nid)
		{
			return;
		}
		
		vector transform[4];
		GetEntryTransform(transform, pOwnerEntity, pUserEntity);
		
		// Spawn animation helper and start CPR animation
		SCR_ChimeraCharacter userChar = SCR_ChimeraCharacter.Cast(pUserEntity);
		if (!userChar)
		{
			return;
		}
		
		m_pActiveHelper = IRRU_CPRHelperCompartment.Cast(
			IRRU_AnimationTools.AnimateWithHelperCompartment(IRRU_EAnimationHelperID.CPR, userChar, transform)
		);
		
		if (m_pActiveHelper)
		{
			m_pActiveHelper.SetPatient(SCR_ChimeraCharacter.Cast(pOwnerEntity));
			
			// Subscribe to termination event
			m_pActiveHelper.GetOnTerminated().Insert(OnAnimationTerminated);
			
			// NOW set the CPR flag after animation is successfully started
			nid.SetReceivingCPR(true);
			m_bCPRActive = true;
			
			// Track start time for fatigue system
			m_fCPRStartTime = GetGame().GetWorld().GetWorldTime();
			
			// Schedule auto-stop after max duration
			GetGame().GetCallqueue().CallLater(AutoStopCPRDueToFatigue, CPR_MAX_DURATION * 1000); // Convert to milliseconds
			
			if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
				Print(string.Format("[CPR] Started CPR session, will auto-stop in %1 seconds", CPR_MAX_DURATION));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false; // CPR needs to be synced across network
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false; // Clients should not run the action
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnSaveActionData(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iPerformingPlayerId);
		writer.WriteBool(m_bCPRActive);
		writer.WriteFloat(m_fCPRStartTime);
		
		// Save cooldown map
		writer.WriteInt(m_mPlayerCooldowns.Count());
		foreach (int playerId, float cooldownTime : m_mPlayerCooldowns)
		{
			writer.WriteInt(playerId);
			writer.WriteFloat(cooldownTime);
		}
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnLoadActionData(ScriptBitReader reader)
	{
		reader.ReadInt(m_iPerformingPlayerId);
		reader.ReadBool(m_bCPRActive);
		reader.ReadFloat(m_fCPRStartTime);
		
		// Load cooldown map
		int cooldownCount;
		reader.ReadInt(cooldownCount);
		m_mPlayerCooldowns.Clear();
		for (int i = 0; i < cooldownCount; i++)
		{
			int playerId;
			float cooldownTime;
			reader.ReadInt(playerId);
			reader.ReadFloat(cooldownTime);
			m_mPlayerCooldowns.Set(playerId, cooldownTime);
		}
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsLocalPlayer(IEntity entity)
	{
		if (!entity)
			return false;
			
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return false;
			
		return pc.GetControlledEntity() == entity;
	}
	
	//------------------------------------------------------------------------------------------------
	// Check if animation position is clear
	protected bool CheckAnimationPositionClear(IEntity owner, IEntity user)
	{
		TraceOBB trace = new TraceOBB();
		trace.Exclude = user;
		vector transform[4];
		GetEntryTransform(transform, owner, user);
		
		for (int i = 0; i < 3; i++)
		{
			trace.Mat[i] = transform[i];
		}
		
		trace.Maxs = CPR_PERFORMER_BB_HALF_EXTENDS;
		trace.Mins = -CPR_PERFORMER_BB_HALF_EXTENDS;
		trace.Start = transform[3] + CPR_PERFORMER_BB_OFFSET;
		
		return GetGame().GetWorld().TracePosition(trace, TraceObstructionCallback) >= 0;
	}
	
	//------------------------------------------------------------------------------------------------
	// Return the transform for where the user will do CPR from
	protected void GetEntryTransform(out vector transform[4], IEntity owner, IEntity user)
	{
		vector userPos = user.GetOrigin();
		vector bestTransform[4];
		float bestDistance = float.MAX;
		
		// Get best orientation to perform CPR
		for (int i = 0; i < CPR_PERFORMER_ANGLES.Count(); i++)
		{
			owner.GetWorldTransform(transform);
			vector angles = Math3D.MatrixToAngles(transform);
			angles[0] = angles[0] + CPR_PERFORMER_ANGLES[i];
			Math3D.AnglesToMatrix(angles, transform);
			transform[3] = transform[3] + CPR_PERFORMER_OFFSETS[i].Multiply3(transform);
			
			float distance = vector.DistanceSqXZ(userPos, transform[3]);
			if (distance < bestDistance)
			{
				bestTransform = transform;
				bestDistance = distance;
			}
		}
		
		transform = bestTransform;
	}
	
	//------------------------------------------------------------------------------------------------
	// Ignore inventory items for collision check
	protected bool TraceObstructionCallback(IEntity entity)
	{
		return !InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
	}
	
}