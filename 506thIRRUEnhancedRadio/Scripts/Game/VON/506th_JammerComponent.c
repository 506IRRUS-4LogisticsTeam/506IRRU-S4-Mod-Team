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

	#ifdef WORKBENCH
	protected ref array<ref Shape> m_aDebugShapes;
	#endif

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Server copies config to replicated vars... hopefully....
		if (Replication.IsServer() || !Replication.IsRunning())
		{
			m_fRange = m_fRangeConfig;
			m_fConeAngle = m_fConeAngleConfig;
			Replication.BumpMe();
		}

		IRRU_JammerManager.GetInstance().RegisterJammer(this);

		#ifdef WORKBENCH
		SetEventMask(owner, EntityEvent.FRAME);
		#endif
	}

	#ifdef WORKBENCH
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		DebugDrawJammer(owner);
	}

	protected void DebugDrawJammer(IEntity owner)
	{
		if (!owner)
			return;

		if (!m_aDebugShapes)
			m_aDebugShapes = new array<ref Shape>();
		m_aDebugShapes.Clear();

		vector mat[4];
		owner.GetWorldTransform(mat);
		vector origin = mat[3];
		vector forward = mat[2];
		vector right = mat[0];
		vector up = mat[1];

		float range = m_fRangeConfig;
		float coneAngle = m_fConeAngleConfig;

		int sphereColor = 0x40808080;
		int lineColor = Color.GRAY;
		if (m_bActive)
		{
			sphereColor = 0x40FF0000;
			lineColor = Color.YELLOW;
		}

		Shape sphereShape = Shape.CreateSphere(sphereColor, ShapeFlags.ONCE | ShapeFlags.TRANSP, origin, range);
		if (sphereShape)
			m_aDebugShapes.Insert(sphereShape);

		vector forwardEnd = origin + forward * range;
		Shape arrowShape = Shape.CreateArrow(origin, forwardEnd, 5.0, lineColor, ShapeFlags.NOZBUFFER | ShapeFlags.ONCE);
		if (arrowShape)
			m_aDebugShapes.Insert(arrowShape);

		if (coneAngle < 180)
		{
			float halfAngleRad = (coneAngle * 0.5) * Math.DEG2RAD;
			float cosAngle = Math.Cos(halfAngleRad);
			float sinAngle = Math.Sin(halfAngleRad);

			int numConeLines = 16;
			float coneStep = (Math.PI * 2.0) / numConeLines;

			for (int i = 0; i < numConeLines; i++)
			{
				float angle = i * coneStep;
				vector radialDir = right * Math.Cos(angle) + up * Math.Sin(angle);
				vector coneDir = (forward * cosAngle + radialDir * sinAngle).Normalized() * range;

				Shape coneLine = Shape.CreateArrow(origin, origin + coneDir, 0.1, lineColor, ShapeFlags.NOZBUFFER | ShapeFlags.ONCE);
				if (coneLine)
					m_aDebugShapes.Insert(coneLine);
			}
		}
	}
	#endif

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
