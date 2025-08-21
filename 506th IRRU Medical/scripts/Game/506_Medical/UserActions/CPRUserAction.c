//! CPR user action for stabilizing unconscious patients
class IRRU_CPRUserAction : ScriptedUserAction
{
	protected static const ref array<float> CPR_PERFORMER_ANGLES = {90, -90};
	protected static const ref array<vector> CPR_PERFORMER_OFFSETS = {{-0.3, 0, -0.8}, {0.3, 0, -0.8}};
	protected static const vector CPR_PERFORMER_BB_OFFSET = {0, 0.45, 0};
	protected static const vector CPR_PERFORMER_BB_HALF_EXTENDS = {0.15, 0.15, 0.15};
	
	protected static const float CPR_MAX_DURATION = 30.0;
	protected static const float CPR_BASE_COOLDOWN = 12.0;
	protected static const float CPR_COOLDOWN_RATIO = 0.4;
	
	[Attribute(defvalue: "3", desc: "Maximum distance to perform CPR in meters", params: "1 5 0.5", category: "CPR Settings")]
	protected float m_fMaxDistance;
	
	protected int m_iPerformingPlayerId = -1;
	protected IRRU_CPRHelperCompartment m_pActiveHelper;
	protected bool m_bCPRActive = false;
	
	protected float m_fCPRStartTime = 0;
	protected ref map<int, float> m_mPlayerCooldowns = new map<int, float>();
	
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
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(owner);
		if (!character)
			return false;
		
		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));
		if (!nid || !nid.IsUnconscious())
			return false;
		
		CompartmentAccessComponent compartment = CompartmentAccessComponent.Cast(character.FindComponent(CompartmentAccessComponent));
		if (compartment && compartment.IsInCompartment())
			return false;
		
		if (!CheckAnimationPositionClear(owner, user))
			return false;
		
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (controller && controller.ACE_Medical_GetUnconsciousPose() != ACE_Medical_EUnconsciousPose.BACK)
			return false;
		
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
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;
			
		int playerId = playerManager.GetPlayerIdFromControlledEntity(user);
		
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
				m_mPlayerCooldowns.Remove(playerId);
			}
		}
		
		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));
		if (nid && nid.IsReceivingCPR())
		{
			if (m_bCPRActive && m_iPerformingPlayerId == playerId)
			{
				return true;
			}
			else
			{
				SetCannotPerformReason("Another medic is already performing CPR");
				return false;
			}
		}
		
		if (nid && !nid.IsUnconscious())
		{
			SetCannotPerformReason("Patient is conscious");
			return false;
		}
		
		if (IRRU_AnimationTools.GetHelperCompartment(user))
			return true;
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(owner);
		if (character)
		{
			CompartmentAccessComponent compartment = CompartmentAccessComponent.Cast(character.FindComponent(CompartmentAccessComponent));
			if (compartment && compartment.IsInCompartment())
			{
				SetCannotPerformReason("Cannot perform CPR in vehicle");
				return false;
			}
			
			SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
			if (controller && controller.ACE_Medical_GetUnconsciousPose() != ACE_Medical_EUnconsciousPose.BACK)
			{
				SetCannotPerformReason("Patient must be on their back for CPR");
				return false;
			}
			
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
		IEntity user = GetGame().GetPlayerController().GetControlledEntity();
		if (user)
		{
			PlayerManager playerManager = GetGame().GetPlayerManager();
			if (playerManager)
			{
				int playerId = playerManager.GetPlayerIdFromControlledEntity(user);
				
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
	protected void OnAnimationTerminated()
	{
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
	protected void AutoStopCPRDueToFatigue()
	{
		if (!m_bCPRActive || !m_pActiveHelper)
			return;
			
		IEntity patient = m_pActiveHelper.GetPatient();
		IEntity performer = m_pActiveHelper.GetPerformer();
		
		if (patient && performer)
		{
			ApplyHealingToPatient(patient);
			
			StopCPR(patient, performer, true);
			
			if (IsLocalPlayer(performer))
			{
				SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
				if (hintManager)
					hintManager.ShowCustom("CPR completed - Fatigue limit reached", "Medical", 3.0);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ApplyHealingToPatient(IEntity patient)
	{
		if (!patient || !Replication.IsServer())
			return;
		
		SCR_CharacterDamageManagerComponent dmgManager = SCR_CharacterDamageManagerComponent.Cast(
			patient.FindComponent(SCR_CharacterDamageManagerComponent));
		
		if (!dmgManager)
			return;
		
		float healPercent = Math.RandomFloatInclusive(5.0, 15.0);
		
		bool healedSomething = dmgManager.HealHitZones(healPercent, false, true);
		
		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
		{
			if (healedSomething)
				Print(string.Format("[NoInstantDeath][CPR] Full CPR completed - Applied %1%% healing to patient", healPercent));
			else
				Print("[NoInstantDeath][CPR] Full CPR completed - Patient already at full health");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void StopCPR(IEntity pOwnerEntity, IEntity pUserEntity, bool wasForcedByFatigue = false)
	{
		if (m_iPerformingPlayerId != -1 && m_fCPRStartTime > 0)
		{
			float currentTime = GetGame().GetWorld().GetWorldTime();
			float cprDuration = (currentTime - m_fCPRStartTime) / 1000.0;
			
			float cooldownTime;
			if (wasForcedByFatigue)
			{
				cooldownTime = CPR_BASE_COOLDOWN;
			}
			else
			{
				cooldownTime = cprDuration * CPR_COOLDOWN_RATIO;
				cooldownTime = Math.Clamp(cooldownTime, 2.0, CPR_BASE_COOLDOWN);
			}
			
			float cooldownEndTime = currentTime + (cooldownTime * 1000.0);
			m_mPlayerCooldowns.Set(m_iPerformingPlayerId, cooldownEndTime);
			
			if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
				Print(string.Format("[NoInstantDeath][CPR] Applied %1s cooldown after %2s of CPR", cooldownTime, cprDuration));
		}
		
		GetGame().GetCallqueue().Remove(AutoStopCPRDueToFatigue);
		
		if (m_pActiveHelper)
		{
			m_pActiveHelper.GetOnTerminated().Remove(OnAnimationTerminated);
			m_pActiveHelper.Terminate(EGetOutType.ANIMATED);
			m_pActiveHelper = null;
		}
		
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
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			int playerId = playerManager.GetPlayerIdFromControlledEntity(pUserEntity);
			
			if (m_bCPRActive && m_iPerformingPlayerId == playerId)
			{
				StopCPR(pOwnerEntity, pUserEntity);
				return;
			}
			
			m_iPerformingPlayerId = playerId;
		}
		
		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(pOwnerEntity.FindComponent(IRRU_NoInstantDeathComponent));
		if (!nid)
		{
			return;
		}
		
		vector transform[4];
		GetEntryTransform(transform, pOwnerEntity, pUserEntity);
		
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
			
			m_pActiveHelper.GetOnTerminated().Insert(OnAnimationTerminated);
			
			nid.SetReceivingCPR(true);
			m_bCPRActive = true;
			
			m_fCPRStartTime = GetGame().GetWorld().GetWorldTime();
			
			GetGame().GetCallqueue().CallLater(AutoStopCPRDueToFatigue, CPR_MAX_DURATION * 1000);
			
			if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
				Print(string.Format("[NoInstantDeath][CPR] Started CPR session, will auto-stop in %1 seconds", CPR_MAX_DURATION));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool OnSaveActionData(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iPerformingPlayerId);
		writer.WriteBool(m_bCPRActive);
		writer.WriteFloat(m_fCPRStartTime);
		
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
	protected void GetEntryTransform(out vector transform[4], IEntity owner, IEntity user)
	{
		vector userPos = user.GetOrigin();
		vector bestTransform[4];
		float bestDistance = float.MAX;
		
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
	protected bool TraceObstructionCallback(IEntity entity)
	{
		return !InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
	}
	
}