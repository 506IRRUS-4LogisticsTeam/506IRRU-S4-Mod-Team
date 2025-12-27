class IRRU_JammerComponentClass : ScriptComponentClass
{
}

class IRRU_JammerComponent : ScriptComponent
{
	[Attribute("500", UIWidgets.Slider, "Jam range in meters", "100 2000 50")]
	protected float m_fRangeConfig;

	[Attribute("180", UIWidgets.Slider, "Cone angle in degrees (180 = omnidirectional)", "10 180 5")]
	protected float m_fConeAngleConfig;

	[Attribute("1", UIWidgets.CheckBox, "Is jammer active")]
	protected bool m_bActive;

	[RplProp()]
	protected float m_fRange;

	[RplProp()]
	protected float m_fConeAngle;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Server copies config to replicated vars
		if (Replication.IsServer() || !Replication.IsRunning())
		{
			m_fRange = m_fRangeConfig;
			m_fConeAngle = m_fConeAngleConfig;
			Replication.BumpMe();
		}

		IRRU_JammerManager.GetInstance().RegisterJammer(this);
	}

	override void OnDelete(IEntity owner)
	{
		IRRU_JammerManager.GetInstance().UnregisterJammer(this);
		super.OnDelete(owner);
	}

	bool IsJammerActive()
	{
		return m_bActive;
	}

	void SetJammerActive(bool active)
	{
		m_bActive = active;
	}

	float GetRange()
	{
		return m_fRange;
	}

	float GetConeAngle()
	{
		return m_fConeAngle;
	}

	vector GetPosition()
	{
		return GetOwner().GetOrigin();
	}

	vector GetForwardVector()
	{
		return GetOwner().GetTransformAxis(2);
	}
}
