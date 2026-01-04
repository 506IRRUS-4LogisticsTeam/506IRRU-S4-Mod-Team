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
	protected ref Shape m_DebugSphere;
	protected ref Shape m_DebugForward;
	protected ref Shape m_DebugConeLines;
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
		DebugDrawJammer();
	}

	//------------------------------------------------------------------------------------------------
	protected void DebugDrawJammer()
	{
		// Clear previous shapes
		m_DebugSphere = null;
		m_DebugForward = null;
		m_DebugConeLines = null;

		vector origin = GetPosition();
		vector forward = GetForwardVector();
		float range = m_fRangeConfig;
		float coneAngle = m_fConeAngleConfig;

		// Choose color based on active state
		int sphereColor = Color.GRAY;
		int lineColor = Color.GRAY;
		if (m_bActive)
		{
			sphereColor = Color.RED;
			lineColor = Color.YELLOW;
		}

		// Draw range sphere (wireframe style using NOZBUFFER so it's visible through terrain)
		m_DebugSphere = Shape.CreateSphere(sphereColor, ShapeFlags.NOZBUFFER | ShapeFlags.ONCE | ShapeFlags.WIREFRAME, origin, range);

		// Draw forward direction arrow
		vector forwardEnd = origin + forward * range;
		m_DebugForward = Shape.CreateArrow(origin, forwardEnd, 5.0, lineColor, ShapeFlags.NOZBUFFER | ShapeFlags.ONCE);

		// If directional (cone angle < 180), draw cone edges
		if (coneAngle < 180)
		{
			float halfAngleRad = (coneAngle * 0.5) * Math.DEG2RAD;

			// Get right and up vectors relative to forward
			vector right = GetOwner().GetTransformAxis(0);
			vector up = GetOwner().GetTransformAxis(1);

			// Calculate cone edge directions
			float cosAngle = Math.Cos(halfAngleRad);
			float sinAngle = Math.Sin(halfAngleRad);

			// Create 4 edge lines for the cone (right, left, up, down)
			vector coneRight = (forward * cosAngle + right * sinAngle).Normalized() * range;
			vector coneLeft = (forward * cosAngle - right * sinAngle).Normalized() * range;
			vector coneUp = (forward * cosAngle + up * sinAngle).Normalized() * range;
			vector coneDown = (forward * cosAngle - up * sinAngle).Normalized() * range;

			// Draw cone lines
			vector conePoints[8];
			conePoints[0] = origin;
			conePoints[1] = origin + coneRight;
			conePoints[2] = origin;
			conePoints[3] = origin + coneLeft;
			conePoints[4] = origin;
			conePoints[5] = origin + coneUp;
			conePoints[6] = origin;
			conePoints[7] = origin + coneDown;

			m_DebugConeLines = Shape.CreateLines(lineColor, ShapeFlags.NOZBUFFER | ShapeFlags.ONCE, conePoints, 8);
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
