// ============================================================================
//  NoInstantDeath_Settings.c   (hot-patch v2 — no ternary operator)
// ============================================================================

[BaseContainerProps()]
class NoInstantDeath_Settings : ACE_ModSettings
{
	// ─── Tunables ─────────────────────────────────────────────────────────
	[Attribute(defvalue: "360",
	           desc:      "Time (in seconds) before the unconscious player dies.",
	           category:  "No Instant Death")]
	float m_fBleedoutTime;

	[Attribute(defvalue: "1",
	           desc:      "Enable verbose debug output to the RPT log.",
	           category:  "No Instant Death",
	           uiwidget:  UIWidgets.CheckBox)]
	bool m_bDebugEnabled;

	// ─── Lightweight singleton so other scripts can query us ─────────────
	static autoptr NoInstantDeath_Settings s_Instance;

	void NoInstantDeath_Settings()        // constructor (runs once)
	{
		s_Instance = this;
	}

	static bool IsDebugEnabled()
	{
		// EnforceScript has no ternary operator, so use an explicit check
		if (s_Instance)
			return s_Instance.m_bDebugEnabled;

		// Default to verbose logging until the settings asset exists
		return true;
	}
}

// Quick compile-time ping
static void TestPrint()
{
	Print("[NoInstantDeath_Settings] ✅ Hot-patch v2 compiled and loaded");
}
