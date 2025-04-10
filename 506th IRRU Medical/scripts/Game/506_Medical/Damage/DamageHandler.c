[ComponentEditorProps(category: "Health", description: "Intercepts player damage to prevent instant death")]
class DamageInterceptorComponentClass : ScriptComponentClass {}

class DamageInterceptorComponent : ScriptComponent
{
	protected SCR_CharacterDamageManagerComponent m_DmgMgr;
	protected NoInstantDeathComponent m_DeathLogic;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_DmgMgr = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		m_DeathLogic = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (!m_DmgMgr || !m_DeathLogic)
		{
			Print("[NoInstantDeath] Missing damage or death component.");
			return;
		}

		// Hook into custom signal created in modded manager
		m_DmgMgr.OnCustomDamageTaken.Insert(OnPlayerDamaged);
		Print("[NoInstantDeath] Damage listener attached.");
	}

	void OnPlayerDamaged(IEntity owner, float damage, notnull Instigator instigator, vector dir, HitZone hitZone)
	{
		if (!m_DmgMgr || !m_DeathLogic)
			return;

		float currentHealth = m_DmgMgr.GetHealth();
		if (currentHealth - damage <= 0.1 && !m_DeathLogic.IsUnconscious())
		{
			Print("[NoInstantDeath] Intercepted lethal damage, forcing unconscious.");
			m_DeathLogic.MakeUnconscious(owner);
		}
	}
}
