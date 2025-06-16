// ============================================================================
//  DamageHandler.c – intercepts player damage (debug‑safe)
// ============================================================================

[ComponentEditorProps(category: "Health",
    description: "Intercepts player damage to prevent instant death")]
class DamageInterceptorComponentClass : ScriptComponentClass {}

class DamageInterceptorComponent : ScriptComponent
{
	protected SCR_CharacterDamageManagerComponent m_DmgMgr;
	protected NoInstantDeathComponent             m_DeathLogic;

	protected bool m_bListenerBound  = false;
	protected bool m_bAnnouncedReady = false;   // replaces static bool

	protected void DebugPrint(string msg)
	{
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print("[NoInstantDeath][INT] " + msg);
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_DmgMgr     = SCR_CharacterDamageManagerComponent.Cast(
			owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_DeathLogic = NoInstantDeathComponent.Cast(
			owner.FindComponent(NoInstantDeathComponent));

		if (!m_DmgMgr || !m_DeathLogic)
			return;

		m_DmgMgr.OnCustomDamageTaken.Insert(OnEntityDamaged);
		m_bListenerBound = true;

		if (m_DeathLogic.NID_IsInitialized())
			DebugPrint("Interceptor active for player.");
		else
			DebugPrint("Interceptor dormant (AI / un‑init).");
	}

	void OnEntityDamaged(IEntity owner, float damage,
	                     notnull Instigator instigator, vector dir, HitZone hitZone)
	{
		if (!m_DeathLogic || !m_DeathLogic.NID_IsInitialized())
			return;   // AI or not yet a player

		if (!m_bAnnouncedReady)
		{
			DebugPrint("Interceptor active for player.");
			m_bAnnouncedReady = true;
		}

		if ((m_DmgMgr.GetHealth() - damage) <= 0.1 && !m_DeathLogic.IsUnconscious())
		{
			DebugPrint("Intercepted lethal damage, forcing unconscious.");
			m_DeathLogic.MakeUnconscious(owner);
		}
	}
}
