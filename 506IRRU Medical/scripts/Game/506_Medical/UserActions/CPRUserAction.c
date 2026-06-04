class IRRU_CPRUserAction : ScriptedUserAction
{
	protected static const ref array<float> CPR_PERFORMER_ANGLES = {90, -90};
	protected static const ref array<vector> CPR_PERFORMER_OFFSETS = {{-0.3, 0, -0.8}, {0.3, 0, -0.8}};
	protected static const vector CPR_PERFORMER_BB_OFFSET = {0, 0.45, 0};
	protected static const vector CPR_PERFORMER_BB_HALF_EXTENDS = {0.15, 0.15, 0.15};

	protected static const float CPR_MAX_DURATION = 30.0;
	protected static const float CPR_BASE_COOLDOWN = 12.0;
	protected static const float CPR_COOLDOWN_RATIO = 0.4;
	protected static const float CPR_MIN_COOLDOWN = 12.0;
	protected static const float CPR_MIN_HEALING = 5.0;
	protected static const float CPR_MAX_HEALING = 17.0;
	protected static const float CPR_FALLBACK_DURATION = 30.0;
	protected static const float CPR_RESILIENCE_REGEN_INTERVAL = 1.0;
	protected static const float CPR_RESILIENCE_PERCENT_PER_SECOND = 3.5;
	protected static const float CPR_LIFESTATE_CHECK_INTERVAL = 0.5;

	[Attribute(defvalue: "3", desc: "Maximum distance to perform CPR in meters", params: "1 5 0.5", category: "CPR Settings")]
	protected float m_fMaxDistance;

	[RplProp()]
	protected int m_iPerformingPlayerId = -1;
	protected IRRU_CPRHelperCompartment m_pActiveHelper;
	[RplProp()]
	protected bool m_bCPRActive = false;
	[RplProp()]
	protected float m_fCPRStartTime = 0;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print("[NoInstantDeath] CPR action initialized");
	}

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

		CharacterControllerComponent ctrl = character.GetCharacterController();
		if (ctrl && ctrl.GetLifeState() == ECharacterLifeState.DEAD)
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

		float distance = vector.Distance(user.GetOrigin(), owner.GetOrigin());
		if (distance > m_fMaxDistance)
			return false;

		return true;
	}

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

		SCR_ChimeraCharacter userChar = SCR_ChimeraCharacter.Cast(user);
		if (userChar)
		{
			IRRU_NoInstantDeathComponent userNid = IRRU_NoInstantDeathComponent.Cast(userChar.FindComponent(IRRU_NoInstantDeathComponent));
			if (userNid && userNid.IsOnCPRCooldown())
			{
				SetCannotPerformReason(string.Format("Resting - %1s remaining", Math.Ceil(userNid.GetCPRCooldownRemaining())));
				return false;
			}
		}

		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(owner.FindComponent(IRRU_NoInstantDeathComponent));

		if (nid && !nid.IsUnconscious())
		{
			SetCannotPerformReason("Patient is conscious");
			return false;
		}

		if (nid && nid.IsReceivingCPR())
		{
			if (m_bCPRActive && m_iPerformingPlayerId == playerId)
				return true;

			SetCannotPerformReason("Another medic is already performing CPR");
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

	override bool GetActionNameScript(out string outName)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return true;

		IEntity user = pc.GetControlledEntity();
		if (user)
		{
			SCR_ChimeraCharacter userChar = SCR_ChimeraCharacter.Cast(user);
			if (userChar)
			{
				IRRU_NoInstantDeathComponent userNid = IRRU_NoInstantDeathComponent.Cast(userChar.FindComponent(IRRU_NoInstantDeathComponent));
				if (userNid && userNid.IsOnCPRCooldown())
				{
					outName = string.Format("CPR Cooldown (%1s)", Math.Ceil(userNid.GetCPRCooldownRemaining()));
					return true;
				}
			}
		}

		if (m_bCPRActive && m_fCPRStartTime > 0)
		{
			float elapsedTime = (GetGame().GetWorld().GetWorldTime() - m_fCPRStartTime) / 1000.0;
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
		}
	}

	protected void ApplyHealingToPatient(IEntity patient)
	{
		if (!patient || !Replication.IsServer())
			return;

		SCR_CharacterDamageManagerComponent dmgManager = SCR_CharacterDamageManagerComponent.Cast(patient.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!dmgManager)
			return;

		float healAmount = Math.RandomFloatInclusive(CPR_MIN_HEALING, CPR_MAX_HEALING);
		dmgManager.HealHitZones(healAmount, false, 1.0);
	}

	protected void CheckPatientLifeState()
	{
		if (!m_bCPRActive || !m_pActiveHelper)
			return;

		IEntity patient = m_pActiveHelper.GetPatient();
		if (!patient)
			return;

		SCR_ChimeraCharacter patientChar = SCR_ChimeraCharacter.Cast(patient);
		if (!patientChar)
			return;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(patientChar.GetCharacterController());
		if (!controller)
			return;

		if (controller.GetLifeState() == ECharacterLifeState.ALIVE)
		{
			IEntity performer = m_pActiveHelper.GetPerformer();
			if (performer)
				StopCPR(patient, performer);
			return;
		}

		GetGame().GetCallqueue().CallLater(CheckPatientLifeState, CPR_LIFESTATE_CHECK_INTERVAL * 1000, false);
	}

	protected void ApplyResilienceRegeneration()
	{
		if (!m_pActiveHelper || !Replication.IsServer())
			return;

		IEntity patient = m_pActiveHelper.GetPatient();
		if (!patient)
			return;

		SCR_CharacterDamageManagerComponent dmgManager = SCR_CharacterDamageManagerComponent.Cast(patient.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!dmgManager)
			return;

		if (dmgManager.IRRU_GetHealthPercentage() < 33.0)
			return;

		SCR_CharacterResilienceHitZone resilienceHZ = dmgManager.GetResilienceHitZone();
		if (!resilienceHZ)
			return;

		float resilienceHealAmount = resilienceHZ.GetMaxHealth() * (CPR_RESILIENCE_PERCENT_PER_SECOND / 100.0);
		resilienceHZ.HandleDamage(-resilienceHealAmount, EDamageType.HEALING, null);
	}

	protected void StopCPR(IEntity pOwnerEntity, IEntity pUserEntity, bool wasForcedByFatigue = false)
	{
		if (m_iPerformingPlayerId != -1 && Replication.IsServer())
		{
			float cprDuration;
			if (m_fCPRStartTime > 0)
				cprDuration = (GetGame().GetWorld().GetWorldTime() - m_fCPRStartTime) / 1000.0;
			else
				cprDuration = CPR_FALLBACK_DURATION;

			float cooldownTime;
			if (wasForcedByFatigue)
				cooldownTime = CPR_BASE_COOLDOWN;
			else
				cooldownTime = Math.Clamp(cprDuration * CPR_COOLDOWN_RATIO, CPR_MIN_COOLDOWN, CPR_BASE_COOLDOWN);

			SCR_ChimeraCharacter userChar = SCR_ChimeraCharacter.Cast(pUserEntity);
			if (userChar)
			{
				IRRU_NoInstantDeathComponent userNid = IRRU_NoInstantDeathComponent.Cast(userChar.FindComponent(IRRU_NoInstantDeathComponent));
				if (userNid)
					userNid.SetCPRCooldown(cooldownTime);
			}
		}

		GetGame().GetCallqueue().Remove(AutoStopCPRDueToFatigue);
		GetGame().GetCallqueue().Remove(ApplyResilienceRegeneration);
		GetGame().GetCallqueue().Remove(CheckPatientLifeState);

		if (m_pActiveHelper)
		{
			m_pActiveHelper.GetOnTerminated().Remove(OnAnimationTerminated);
			m_pActiveHelper.Terminate();
			m_pActiveHelper = null;
		}

		IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(pOwnerEntity.FindComponent(IRRU_NoInstantDeathComponent));
		if (nid)
			nid.SetReceivingCPR(false);

		m_bCPRActive = false;
		m_fCPRStartTime = 0;
		m_iPerformingPlayerId = -1;
	}

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
			return;

		SCR_ChimeraCharacter userChar = SCR_ChimeraCharacter.Cast(pUserEntity);
		if (!userChar)
			return;

		vector transform[4];
		GetEntryTransform(transform, pOwnerEntity, pUserEntity);

		m_pActiveHelper = IRRU_CPRHelperCompartment.Cast(
			IRRU_AnimationTools.AnimateWithHelperCompartment(IRRU_EAnimationHelperID.CPR, userChar, transform));

		if (m_pActiveHelper)
		{
			m_pActiveHelper.SetPatient(SCR_ChimeraCharacter.Cast(pOwnerEntity));
			m_pActiveHelper.GetOnTerminated().Insert(OnAnimationTerminated);

			nid.SetReceivingCPR(true);
			m_bCPRActive = true;
			m_fCPRStartTime = GetGame().GetWorld().GetWorldTime();

			GetGame().GetCallqueue().CallLater(AutoStopCPRDueToFatigue, CPR_MAX_DURATION * 1000);
			GetGame().GetCallqueue().CallLater(CheckPatientLifeState, CPR_LIFESTATE_CHECK_INTERVAL * 1000, false);

			if (Replication.IsServer())
				GetGame().GetCallqueue().CallLater(ApplyResilienceRegeneration, CPR_RESILIENCE_REGEN_INTERVAL * 1000, true);
		}
	}

	override bool HasLocalEffectOnlyScript() { return false; }
	override bool CanBroadcastScript() { return false; }

	override bool OnSaveActionData(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iPerformingPlayerId);
		writer.WriteBool(m_bCPRActive);
		writer.WriteFloat(m_fCPRStartTime);
		return true;
	}

	override bool OnLoadActionData(ScriptBitReader reader)
	{
		reader.ReadInt(m_iPerformingPlayerId);
		reader.ReadBool(m_bCPRActive);
		reader.ReadFloat(m_fCPRStartTime);
		return true;
	}

	protected bool IsLocalPlayer(IEntity entity)
	{
		if (!entity)
			return false;
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return false;
		return pc.GetControlledEntity() == entity;
	}

	protected bool CheckAnimationPositionClear(IEntity owner, IEntity user)
	{
		TraceOBB trace = new TraceOBB();
		trace.Exclude = user;
		vector transform[4];
		GetEntryTransform(transform, owner, user);

		for (int i = 0; i < 3; i++)
			trace.Mat[i] = transform[i];

		trace.Maxs = CPR_PERFORMER_BB_HALF_EXTENDS;
		trace.Mins = -CPR_PERFORMER_BB_HALF_EXTENDS;
		trace.Start = transform[3] + CPR_PERFORMER_BB_OFFSET;

		return GetGame().GetWorld().TracePosition(trace, TraceObstructionCallback) >= 0;
	}

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

	protected bool TraceObstructionCallback(IEntity entity)
	{
		return !InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
	}
}
