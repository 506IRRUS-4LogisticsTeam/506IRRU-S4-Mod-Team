class IRRU_CPRUserAction : ScriptedUserAction
{
	protected static const ref array<float> CPR_PERFORMER_ANGLES = {90, -90};
	protected static const ref array<vector> CPR_PERFORMER_OFFSETS = {{-0.3, 0, -0.8}, {0.3, 0, -0.8}};
	protected static const vector CPR_PERFORMER_BB_OFFSET = {0, 0.45, 0};
	protected static const vector CPR_PERFORMER_BB_HALF_EXTENDS = {0.15, 0.15, 0.15};

	protected static const float CPR_MAX_DURATION = 30.0;
	protected static const float CPR_COOLDOWN = 12.0;
	protected static const float CPR_MIN_HEALING = 20.0;
	protected static const float CPR_MAX_HEALING = 40.0;
	protected static const float CPR_RESILIENCE_REGEN_INTERVAL = 1.0;
	protected static const float CPR_RESILIENCE_PERCENT_PER_SECOND = 4.0;
	protected static const float CPR_LIFESTATE_CHECK_INTERVAL = 0.5;

	[Attribute(defvalue: "3", desc: "Maximum distance to perform CPR in meters", params: "1 5 0.5", category: "CPR Settings")]
	protected float m_fMaxDistance;

	[RplProp()]
	protected int m_iPerformingPlayerId = -1;
	[RplProp()]
	protected bool m_bCPRActive = false;
	[RplProp()]
	protected float m_fCPRStartTime = 0;

	protected IRRU_CPRHelperCompartment m_pActiveHelper;

	//! The patient is the action's owner and never changes
	protected SCR_ChimeraCharacter m_Patient;
	protected SCR_CharacterControllerComponent m_PatientController;
	protected CompartmentAccessComponent m_PatientCompartment;
	protected IRRU_NoInstantDeathComponent m_PatientNID;
	protected SCR_CharacterDamageManagerComponent m_PatientDamageManager;
	protected bool m_bPatientRefsResolved;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);

		// Only the owner is safe to take here - see IRRU_ResolvePatientRefs()
		m_Patient = SCR_ChimeraCharacter.Cast(pOwnerEntity);
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve the patient's components once, then never again (one bool test
	//! per call afterwards).
	//! This CANNOT be done in Init(): actions are constructed in the same frame
	//! the character entity is created, before the character has populated its
	//! own component accessors, so GetDamageManager() returns null there. Caching
	//! that null left healing and resilience regeneration dead for the casualty's
	//! whole life. FindComponent works as soon as the component exists, and the
	//! damage manager decides when we are done, since it is the one that matters.
	protected void IRRU_ResolvePatientRefs()
	{
		if (m_bPatientRefsResolved || !m_Patient)
			return;

		m_PatientDamageManager = SCR_CharacterDamageManagerComponent.Cast(m_Patient.FindComponent(SCR_CharacterDamageManagerComponent));
		m_PatientController = SCR_CharacterControllerComponent.Cast(m_Patient.FindComponent(SCR_CharacterControllerComponent));
		m_PatientCompartment = CompartmentAccessComponent.Cast(m_Patient.FindComponent(CompartmentAccessComponent));
		m_PatientNID = IRRU_NoInstantDeathComponent.Cast(m_Patient.FindComponent(IRRU_NoInstantDeathComponent));

		m_bPatientRefsResolved = m_PatientDamageManager != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Evaluated every frame for nearby bodies: cheap rejections first, the world trace last
	override bool CanBeShownScript(IEntity user)
	{
		IRRU_ResolvePatientRefs();

		if (!user || !m_PatientNID || !m_PatientNID.IsUnconscious())
			return false;

		if (m_PatientController && m_PatientController.GetLifeState() == ECharacterLifeState.DEAD)
			return false;

		if (vector.Distance(user.GetOrigin(), m_Patient.GetOrigin()) > m_fMaxDistance)
			return false;

		if (m_PatientCompartment && m_PatientCompartment.IsInCompartment())
			return false;

		if (m_PatientController && m_PatientController.ACE_Medical_GetUnconsciousPose() != ACE_Medical_EUnconsciousPose.BACK)
			return false;

		return CheckAnimationPositionClear(user);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!user)
			return false;

		float userCooldown = IRRU_GetUserCooldownRemaining(user);
		if (userCooldown >= 0)
		{
			SetCannotPerformReason(string.Format("Resting - %1s remaining", userCooldown));
			return false;
		}

		if (m_PatientNID && m_PatientNID.IsReceivingCPR())
		{
			if (m_bCPRActive && m_iPerformingPlayerId == GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(user))
				return true;

			SetCannotPerformReason("Another medic is already performing CPR");
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return true;

		float userCooldown = IRRU_GetUserCooldownRemaining(pc.GetControlledEntity());
		if (userCooldown >= 0)
		{
			outName = string.Format("CPR Cooldown (%1s)", userCooldown);
			return true;
		}

		outName = "Start CPR";
		if (m_bCPRActive && m_fCPRStartTime > 0)
		{
			float remainingTime = CPR_MAX_DURATION - (GetGame().GetWorld().GetWorldTime() - m_fCPRStartTime) / 1000.0;
			if (remainingTime > 0)
				outName = string.Format("Stop CPR (%1s)", Math.Ceil(remainingTime));
			else
				outName = "Stop CPR";
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		if (m_bCPRActive && m_iPerformingPlayerId == playerId)
		{
			StopCPR(pUserEntity);
			return;
		}

		IRRU_ResolvePatientRefs();

		SCR_ChimeraCharacter userChar = SCR_ChimeraCharacter.Cast(pUserEntity);
		if (!m_Patient || !m_PatientNID || !userChar)
			return;

		vector transform[4];
		GetEntryTransform(transform, pUserEntity);

		m_pActiveHelper = IRRU_CPRHelperCompartment.Cast(IRRU_AnimationTools.AnimateWithHelperCompartment(IRRU_EAnimationHelperID.CPR, userChar, transform));
		if (!m_pActiveHelper)
		{
			Print("[IRRU_CPR] Helper compartment could not be created - CPR not started", LogLevel.WARNING);
			return;
		}

		m_pActiveHelper.SetPatient(m_Patient);
		m_pActiveHelper.GetOnTerminated().Insert(OnAnimationTerminated);

		m_iPerformingPlayerId = playerId;
		m_bCPRActive = true;
		m_fCPRStartTime = GetGame().GetWorld().GetWorldTime();

		GetGame().GetCallqueue().CallLater(AutoStopCPRDueToFatigue, CPR_MAX_DURATION * 1000, false);
		GetGame().GetCallqueue().CallLater(CheckPatientLifeState, CPR_LIFESTATE_CHECK_INTERVAL * 1000, false);

		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(ApplyResilienceRegeneration, CPR_RESILIENCE_REGEN_INTERVAL * 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	//! The helper ended on its own (performer left or died); always tear down so no timer outlives it
	protected void OnAnimationTerminated()
	{
		IEntity performer;
		if (m_pActiveHelper)
			performer = m_pActiveHelper.GetPerformer();

		StopCPR(performer);
	}

	//------------------------------------------------------------------------------------------------
	protected void AutoStopCPRDueToFatigue()
	{
		if (!m_bCPRActive || !m_pActiveHelper)
			return;

		ApplyHealingToPatient();
		StopCPR(m_pActiveHelper.GetPerformer());
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckPatientLifeState()
	{
		if (!m_bCPRActive || !m_pActiveHelper)
			return;

		if (m_PatientController && m_PatientController.GetLifeState() == ECharacterLifeState.ALIVE)
		{
			StopCPR(m_pActiveHelper.GetPerformer());
			return;
		}

		GetGame().GetCallqueue().CallLater(CheckPatientLifeState, CPR_LIFESTATE_CHECK_INTERVAL * 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyHealingToPatient()
	{
		if (!Replication.IsServer())
			return;

		// Guard, not a workaround: HealHitZones is called on this reference. If it
		// is ever null again, CPR silently stops healing - so say so loudly.
		if (!m_PatientDamageManager)
		{
			Print("[IRRU_CPR] Patient damage manager missing - no CPR healing applied", LogLevel.WARNING);
			return;
		}

		m_PatientDamageManager.HealHitZones(Math.RandomFloatInclusive(CPR_MIN_HEALING, CPR_MAX_HEALING), false, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyResilienceRegeneration()
	{
		if (!m_pActiveHelper || !m_PatientDamageManager || m_PatientDamageManager.IRRU_GetHealthPercentage() < 33.0)
			return;

		SCR_CharacterResilienceHitZone resilienceHZ = m_PatientDamageManager.GetResilienceHitZone();
		if (!resilienceHZ)
			return;

		resilienceHZ.HandleDamage(-resilienceHZ.GetMaxHealth() * (CPR_RESILIENCE_PERCENT_PER_SECOND / 100.0), EDamageType.HEALING, null);
	}

	//------------------------------------------------------------------------------------------------
	protected void StopCPR(IEntity performer)
	{
		if (m_iPerformingPlayerId != -1 && Replication.IsServer())
		{
			IRRU_NoInstantDeathComponent performerNID = IRRU_GetNID(performer);
			if (performerNID)
				performerNID.SetCPRCooldown(CPR_COOLDOWN);
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

		if (m_PatientNID)
			m_PatientNID.SetReceivingCPR(false);

		m_bCPRActive = false;
		m_fCPRStartTime = 0;
		m_iPerformingPlayerId = -1;
	}

	//------------------------------------------------------------------------------------------------
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

	//------------------------------------------------------------------------------------------------
	protected bool CheckAnimationPositionClear(IEntity user)
	{
		vector transform[4];
		GetEntryTransform(transform, user);

		TraceOBB trace = new TraceOBB();
		trace.Exclude = user;
		for (int i = 0; i < 3; i++)
			trace.Mat[i] = transform[i];

		trace.Maxs = CPR_PERFORMER_BB_HALF_EXTENDS;
		trace.Mins = -CPR_PERFORMER_BB_HALF_EXTENDS;
		trace.Start = transform[3] + CPR_PERFORMER_BB_OFFSET;

		return GetGame().GetWorld().TracePosition(trace, TraceObstructionCallback) >= 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Kneeling spot beside the patient closest to the user
	protected void GetEntryTransform(out vector transform[4], IEntity user)
	{
		vector userPos = user.GetOrigin();
		vector bestTransform[4];
		float bestDistance = float.MAX;

		for (int i = 0; i < CPR_PERFORMER_ANGLES.Count(); i++)
		{
			m_Patient.GetWorldTransform(transform);
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

	//------------------------------------------------------------------------------------------------
	protected IRRU_NoInstantDeathComponent IRRU_GetNID(IEntity entity)
	{
		if (!entity)
			return null;

		return IRRU_NoInstantDeathComponent.Cast(entity.FindComponent(IRRU_NoInstantDeathComponent));
	}

	//------------------------------------------------------------------------------------------------
	//! \return whole seconds left on the user's CPR cooldown, -1 when none
	protected float IRRU_GetUserCooldownRemaining(IEntity user)
	{
		IRRU_NoInstantDeathComponent userNID = IRRU_GetNID(user);
		if (userNID && userNID.IsOnCPRCooldown())
			return Math.Ceil(userNID.GetCPRCooldownRemaining());

		return -1;
	}
}
