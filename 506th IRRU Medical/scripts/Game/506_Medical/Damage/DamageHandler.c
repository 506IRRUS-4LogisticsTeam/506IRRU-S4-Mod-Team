// ============================================================================
//  DamageHandler.c  – waits for player init
// ============================================================================

[ComponentEditorProps(category: "Health",
        description: "Intercepts player damage to prevent instant death")]
class DamageInterceptorComponentClass : ScriptComponentClass {}

class DamageInterceptorComponent : ScriptComponent
{
	protected SCR_CharacterDamageManagerComponent m_DmgMgr;
	protected NoInstantDeathComponent             m_DeathLogic;
	protected bool                                m_bListenerBound = false;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_DmgMgr     = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_DeathLogic = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (!m_DmgMgr || !m_DeathLogic)
			return;

		// Bind immediately; we’ll filter in the callback
		m_DmgMgr.OnCustomDamageTaken.Insert(OnEntityDamaged);
		m_bListenerBound = true;

		if (m_DeathLogic.NID_IsInitialized())
			Print("[NoInstantDeath] Interceptor active for player.");
		else
			Print("[NoInstantDeath] Interceptor dormant (AI / un-init).");
	}

	void OnEntityDamaged(IEntity owner, float damage,
	                     notnull Instigator instigator, vector dir, HitZone hitZone)
	{
		// Skip everything until the player-controller has initialised NID
		if (!m_DeathLogic || !m_DeathLogic.NID_IsInitialized())
			return;                     // AI or not yet a player

		// First time we realise it’s a player → announce activation
		static bool s_bAnnounced = false;
		if (!s_bAnnounced)
		{
			Print("[NoInstantDeath] Interceptor active for player.");
			s_bAnnounced = true;
		}

		if (m_DmgMgr.GetHealth() - damage <= 0.1 && !m_DeathLogic.IsUnconscious())
		{
			Print("[NoInstantDeath] Intercepted lethal damage, forcing unconscious.");
			m_DeathLogic.MakeUnconscious(owner);
		}
	}
}
