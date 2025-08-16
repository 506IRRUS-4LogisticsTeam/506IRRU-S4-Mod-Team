//! Damage interception component for no-instant-death system

[ComponentEditorProps(category: "Health", description: "Intercepts player damage to prevent instant death")]
class IRRU_DamageInterceptorComponentClass : ScriptComponentClass
{
}

//! Intercepts damage to prevent instant death in medical system
class IRRU_DamageInterceptorComponent : ScriptComponent
{
	protected SCR_CharacterDamageManagerComponent m_DamageManager;
	protected IRRU_NoInstantDeathComponent m_DeathLogic;
	protected bool m_bListenerBound = false;
	protected bool m_bAnnouncedReady = false;

	//------------------------------------------------------------------------------------------------
	protected void DebugPrint(string msg)
	{
		if (IRRU_NoInstantDeathSettings.IsDebugEnabled())
			Print("[NoInstantDeath][INT] " + msg);
	}

	//------------------------------------------------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_DamageManager = SCR_CharacterDamageManagerComponent.Cast(
			owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_DeathLogic = IRRU_NoInstantDeathComponent.Cast(
			owner.FindComponent(IRRU_NoInstantDeathComponent));

		if (!m_DamageManager || !m_DeathLogic)
			return;

		m_DamageManager.OnCustomDamageTaken.Insert(OnEntityDamaged);
		m_bListenerBound = true;

		if (m_DeathLogic.IsInitialized())
			DebugPrint("Interceptor active for player.");
		else
			DebugPrint("Interceptor dormant (AI or not initialized).");
	}

	void OnEntityDamaged(IEntity owner, float damage, notnull Instigator instigator, vector dir, HitZone hitZone)
	{
		if (!m_DeathLogic || !m_DeathLogic.IsInitialized())
			return;

		if (!m_bAnnouncedReady)
		{
			DebugPrint("Interceptor active for player.");
			m_bAnnouncedReady = true;
		}

		if ((m_DamageManager.GetHealth() - damage) <= 0.1 && !m_DeathLogic.IsUnconscious())
		{
			DebugPrint("Intercepted lethal damage, forcing unconscious.");
			m_DeathLogic.MakeUnconscious(owner);
		}
	}
}
