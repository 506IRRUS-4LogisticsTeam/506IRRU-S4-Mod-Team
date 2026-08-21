class IRRU_JammerToggleUserAction : ScriptedUserAction
{
	protected IRRU_JammerComponent m_JammerComponent;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);

		m_JammerComponent = IRRU_JammerComponent.Cast(
			pOwnerEntity.FindComponent(IRRU_JammerComponent)
		);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		return m_JammerComponent != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return m_JammerComponent != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (m_JammerComponent.IsJammerActive())
			outName = "Disable Jammer";
		else
			outName = "Enable Jammer";

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);
		m_JammerComponent.SetJammerActive(!m_JammerComponent.IsJammerActive());
	}
}
