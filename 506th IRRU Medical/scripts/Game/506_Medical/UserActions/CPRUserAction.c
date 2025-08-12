// ============================================================================
//  CPRUserAction.c
//  506th IRRU Medical Mod v2.0.6
//  CPR stabilization mechanic - pauses bleedout timer while performing CPR
// ============================================================================

class CPRUserAction : ScriptedUserAction
{
	// Configurable attributes
	[Attribute(defvalue: "18", desc: "Duration of CPR cycle in seconds", params: "5 60 1", category: "CPR Settings")]
	protected float m_fCPRCycleDuration;
	
	[Attribute(defvalue: "15", desc: "Duration of fatigue after completing full CPR cycle in seconds", params: "5 60 1", category: "CPR Settings")]
	protected float m_fFatigueDuration;
	
	[Attribute(defvalue: "3", desc: "Rate at which fatigue decays (3 = three times as fast as it builds)", params: "0.5 5 0.1", category: "CPR Settings")]
	protected float m_fFatigueDecayRate;
	
	[Attribute(defvalue: "3", desc: "Minimum CPR duration to cause fatigue in seconds", params: "1 30 1", category: "CPR Settings")]
	protected float m_fMinimumCPRForFatigue;
	
	[Attribute(defvalue: "5", desc: "Extra fatigue penalty for spam (multiplier for short bursts)", params: "1 10 0.5", category: "CPR Settings")]
	protected float m_fSpamPenaltyMultiplier;
	
	[Attribute(defvalue: "3", desc: "Maximum distance to perform CPR in meters", params: "1 5 0.5", category: "CPR Settings")]
	protected float m_fMaxDistance;
	
	// Static fatigue tracking per player
	protected static ref map<int, float> s_mPlayerFatigue = new map<int, float>();
	protected static ref map<int, float> s_mFatigueTimestamps = new map<int, float>();
	
	// Instance tracking
	protected float m_fActionStartTime;
	protected int m_iPerformingPlayerId = -1;
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print(string.Format("[CPR] Action initialized with duration: %1s", m_fCPRCycleDuration));
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
		NoInstantDeathComponent nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
		if (!nid || !nid.IsUnconscious())
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
		
		// Get player ID
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;
			
		int playerId = playerManager.GetPlayerIdFromControlledEntity(user);
		
		// Check if this player is fatigued
		if (IsPlayerFatigued(playerId))
		{
			float remaining = GetFatigueTimeRemaining(playerId);
			if (remaining > 30)
				SetCannotPerformReason(string.Format("Exhausted from CPR! Rest for %1s", Math.Round(remaining)));
			else if (remaining > 10)
				SetCannotPerformReason(string.Format("Catching breath... %1s", Math.Round(remaining)));
			else
				SetCannotPerformReason(string.Format("Almost ready... %1s", Math.Round(remaining)));
			
			return false;
		}
		
		// Check if someone else is already doing CPR (but not us!)
		NoInstantDeathComponent nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
		if (nid && nid.IsReceivingCPR())
		{
			// Check if WE are the ones performing CPR
			if (m_iPerformingPlayerId == playerId)
			{
				// We're the ones doing CPR, so allow it to continue
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
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		IEntity user = GetGame().GetPlayerController().GetControlledEntity();
		if (user)
		{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			if (playerManager)
			{
				int playerId = playerManager.GetPlayerIdFromControlledEntity(user);
				if (IsPlayerFatigued(playerId))
				{
					float remaining = GetFatigueTimeRemaining(playerId);
					outName = string.Format("Perform CPR (Fatigued: %1s)", Math.Round(remaining));
					return true;
				}
			}
		}
		
		outName = "Perform CPR";
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnActionStart(IEntity pUserEntity)
	{
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print("[CPR] Action started by: " + pUserEntity.GetName());
		
		m_fActionStartTime = GetGame().GetWorld().GetWorldTime() * 0.001; // Convert to seconds
		
		// Store who's performing CPR
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
			m_iPerformingPlayerId = playerManager.GetPlayerIdFromControlledEntity(pUserEntity);
		
		// Set CPR flag on patient - this pauses their bleedout timer
		IEntity owner = GetOwner();
		if (owner)
		{
			NoInstantDeathComponent nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
			if (nid)
			{
				nid.SetReceivingCPR(true);
				
				if (NoInstantDeath_Settings.IsDebugEnabled())
					Print("[CPR] Set receiving CPR flag on patient");
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		float currentTime = GetGame().GetWorld().GetWorldTime() * 0.001;
		float timePerformed = currentTime - m_fActionStartTime;
		
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print(string.Format("[CPR] Action canceled after %1 seconds", timePerformed));
		
		// Clear CPR flag on patient - timer resumes
		NoInstantDeathComponent nid = NoInstantDeathComponent.Cast(pOwnerEntity.FindComponent(NoInstantDeathComponent));
		if (nid)
		{
			nid.SetReceivingCPR(false);
			
			if (NoInstantDeath_Settings.IsDebugEnabled())
				Print("[CPR] Cleared receiving CPR flag on patient");
		}
		
		// Apply fatigue based on time performed
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(pUserEntity);
			
			// Calculate base fatigue
			float completionPercent = Math.Min(timePerformed / m_fCPRCycleDuration, 1.0);
			float fatigueDuration = m_fFatigueDuration * completionPercent;
			
			// Apply spam penalty for very short bursts (under minimum time)
			if (timePerformed < m_fMinimumCPRForFatigue && timePerformed > 0.5)
			{
				// Heavy penalty for spam - multiply fatigue
				fatigueDuration = m_fMinimumCPRForFatigue * m_fSpamPenaltyMultiplier;
				
				// Show warning about spamming (account for decay rate)
				float effectiveFatigueTime = fatigueDuration / m_fFatigueDecayRate;
				SCR_HintManagerComponent.ShowCustomHint(
					string.Format("Ineffective CPR! Rest for %1 seconds", Math.Round(effectiveFatigueTime)),
					"Medical",
					3.0
				);
			}
			else if (timePerformed >= m_fMinimumCPRForFatigue)
			{
				// Normal proportional fatigue for legitimate attempts
				// Show feedback to player (account for decay rate)
				float effectiveFatigueTime = fatigueDuration / m_fFatigueDecayRate;
				SCR_HintManagerComponent.ShowCustomHint(
					string.Format("CPR interrupted. Rest for %1 seconds", Math.Round(effectiveFatigueTime)),
					"Medical",
					3.0
				);
			}
			// else: Very short accidental presses (< 0.5s) don't cause fatigue
			
			if (fatigueDuration > 0)
				ApplyFatigueToPlayer(playerId, fatigueDuration);
		}
		
		m_iPerformingPlayerId = -1;
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print("[CPR] Full cycle completed");
		
		// Clear CPR flag temporarily (will restart if player keeps holding)
		NoInstantDeathComponent nid = NoInstantDeathComponent.Cast(pOwnerEntity.FindComponent(NoInstantDeathComponent));
		if (nid)
		{
			nid.SetReceivingCPR(false);
			
			// Optional: Add bonus time for completing full cycle
			// nid.AddBonusTime(30.0);
			
			if (NoInstantDeath_Settings.IsDebugEnabled())
				Print("[CPR] Full CPR cycle completed - patient stabilized temporarily");
		}
		
		// Apply reduced fatigue for completing full cycle (reward for doing it properly)
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(pUserEntity);
			// Give a small bonus for completing the full cycle - only 80% of max fatigue
			float reducedFatigue = m_fFatigueDuration * 0.8;
			ApplyFatigueToPlayer(playerId, reducedFatigue);
			
			// Show feedback (account for decay rate for actual rest time)
			float effectiveRestTime = reducedFatigue / m_fFatigueDecayRate;
			SCR_HintManagerComponent.ShowCustomHint(
				string.Format("CPR cycle complete. Rest for %1 seconds", Math.Round(effectiveRestTime)),
				"Medical",
				3.0
			);
		}
		
		m_iPerformingPlayerId = -1;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false; // CPR needs to be synced across network
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnSaveActionData(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iPerformingPlayerId);
		writer.WriteFloat(m_fActionStartTime);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnLoadActionData(ScriptBitReader reader)
	{
		reader.ReadInt(m_iPerformingPlayerId);
		reader.ReadFloat(m_fActionStartTime);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	// FATIGUE SYSTEM HELPERS
	//------------------------------------------------------------------------------------------------
	
	protected bool IsPlayerFatigued(int playerId)
	{
		if (!s_mPlayerFatigue || !s_mPlayerFatigue.Contains(playerId))
			return false;
			
		float fatigueTime = s_mPlayerFatigue.Get(playerId);
		float timestamp = s_mFatigueTimestamps.Get(playerId);
		float currentTime = GetGame().GetWorld().GetWorldTime() * 0.001;
		float elapsed = currentTime - timestamp;
		
		// Fatigue decays over time (faster than it accumulates)
		float remainingFatigue = fatigueTime - (elapsed * m_fFatigueDecayRate);
		
		if (remainingFatigue <= 0)
		{
			s_mPlayerFatigue.Remove(playerId);
			s_mFatigueTimestamps.Remove(playerId);
			return false;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected float GetFatigueTimeRemaining(int playerId)
	{
		if (!s_mPlayerFatigue || !s_mPlayerFatigue.Contains(playerId))
			return 0;
			
		float fatigueTime = s_mPlayerFatigue.Get(playerId);
		float timestamp = s_mFatigueTimestamps.Get(playerId);
		float currentTime = GetGame().GetWorld().GetWorldTime() * 0.001;
		float elapsed = currentTime - timestamp;
		
		float remainingFatigue = fatigueTime - (elapsed * m_fFatigueDecayRate);
		return Math.Max(0, remainingFatigue);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ApplyFatigueToPlayer(int playerId, float duration)
	{
		if (!s_mPlayerFatigue)
			s_mPlayerFatigue = new map<int, float>();
		if (!s_mFatigueTimestamps)
			s_mFatigueTimestamps = new map<int, float>();
		
		float currentTime = GetGame().GetWorld().GetWorldTime() * 0.001;
		s_mPlayerFatigue.Set(playerId, duration);
		s_mFatigueTimestamps.Set(playerId, currentTime);
		
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print(string.Format("[CPR] Applied %1s fatigue to player %2", duration, playerId));
	}
}