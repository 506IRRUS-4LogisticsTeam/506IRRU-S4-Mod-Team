[BaseContainerProps()]
class NoInstantDeath_Settings : ACE_ModSettings
{
	[Attribute(defvalue: "360", desc: "Time (in seconds) before the unconscious player dies.", category: "No Instant Death")]
	float m_fBleedoutTime;
}

static void TestPrint()
{
	Print("[NoInstantDeath_Settings] ✅ Class compiled and loaded");
}