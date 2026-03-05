class IRRU_NeedleDecompressionAction : ScriptedUserAction
{
	protected static const float TREATMENT_DURATION = 8.0;
	protected static const float MAX_DISTANCE = 3.0;

	[RplProp()]
	protected int m_iPerformingPlayerId = -1;

	[RplProp()]
	protected bool m_bTreatmentActive = false;

	[RplProp()]
	protected float m_fTreatmentStartTime = 0;

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!user)
			return false;

		IEntity owner = GetOwner();
		if (!owner)
			return false;

		if (user == owner)
			return false;

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			owner.FindComponent(IRRU_PneumothoraxComponent));
		if (!pneumo || !pneumo.HasPneumothorax())
			return false;

		float distance = vector.Distance(user.GetOrigin(), owner.GetOrigin());
		if (distance > MAX_DISTANCE)
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

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			owner.FindComponent(IRRU_PneumothoraxComponent));

		if (!pneumo || !pneumo.HasPneumothorax())
		{
			SetCannotPerformReason("No pneumothorax detected");
			return false;
		}

		if (m_bTreatmentActive)
		{
			PlayerManager pm = GetGame().GetPlayerManager();
			if (pm)
			{
				int playerId = pm.GetPlayerIdFromControlledEntity(user);
				if (m_iPerformingPlayerId != playerId)
				{
					SetCannotPerformReason("Another medic is already treating");
					return false;
				}
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			outName = "Needle Decompression";
			return true;
		}

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			owner.FindComponent(IRRU_PneumothoraxComponent));

		if (m_bTreatmentActive && m_fTreatmentStartTime > 0)
		{
			float currentTime = GetGame().GetWorld().GetWorldTime();
			float elapsed = (currentTime - m_fTreatmentStartTime) / 1000.0;
			float remaining = TREATMENT_DURATION - elapsed;

			if (remaining > 0)
				outName = string.Format("Treating... (%1s)", Math.Ceil(remaining));
			else
				outName = "Treating...";
		}
		else if (pneumo && pneumo.GetStage() == IRRU_EPneumothoraxStage.TENSION)
		{
			outName = "Needle Decompression (Tension)";
		}
		else
		{
			outName = "Needle Decompression";
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		int playerId = pm.GetPlayerIdFromControlledEntity(pUserEntity);

		if (m_bTreatmentActive && m_iPerformingPlayerId == playerId)
		{
			CompleteTreatment(pOwnerEntity, pUserEntity);
			return;
		}

		m_iPerformingPlayerId = playerId;
		m_bTreatmentActive = true;
		m_fTreatmentStartTime = GetGame().GetWorld().GetWorldTime();

		if (Replication.IsServer())
		{
			Replication.BumpMe();

			GetGame().GetCallqueue().CallLater(AutoCompleteTreatment, TREATMENT_DURATION * 1000, false);
		}

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
		{
			Print(string.Format("[Pneumothorax] Needle decompression started on %1 by player %2",
				pOwnerEntity.ToString(), playerId));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AutoCompleteTreatment()
	{
		if (!m_bTreatmentActive)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		PlayerManager pm = GetGame().GetPlayerManager();
		IEntity performer = null;
		if (pm && m_iPerformingPlayerId > 0)
			performer = pm.GetPlayerControlledEntity(m_iPerformingPlayerId);

		CompleteTreatment(owner, performer);
	}

	//------------------------------------------------------------------------------------------------
	protected void CompleteTreatment(IEntity patient, IEntity performer)
	{
		if (!Replication.IsServer() && Replication.IsRunning())
			return;

		IRRU_PneumothoraxComponent pneumo = IRRU_PneumothoraxComponent.Cast(
			patient.FindComponent(IRRU_PneumothoraxComponent));

		if (pneumo && pneumo.HasPneumothorax())
			pneumo.Treat();

		GetGame().GetCallqueue().Remove(AutoCompleteTreatment);

		m_bTreatmentActive = false;
		m_fTreatmentStartTime = 0;
		m_iPerformingPlayerId = -1;

		Replication.BumpMe();

		if (IRRU_PneumothoraxSettings.IsDebugEnabled())
		{
			Print(string.Format("[Pneumothorax] Needle decompression completed on %1",
				patient.ToString()));
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
}
