[ComponentEditorProps(category: "Health", description: "Overrides death to force unconsciousness")]
class NoInstantDeathComponentClass : ScriptComponentClass {}

class NoInstantDeathComponent : ScriptComponent
{
	[RplProp(onRplName: "OnRep_IsUnconscious")]
	protected bool m_bIsUnconscious = false;

	protected float m_fBleedOutTime = 60.0;
	protected float m_fUnconsciousTimer = 0.0;
	protected RplComponent m_Rpl;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Rpl = RplComponent.Cast(owner.FindComponent(RplComponent));

		// ✅ Load ACE-style mod settings
		NoInstantDeath_Settings settings = ACE_SettingsHelperT<NoInstantDeath_Settings>.GetModSettings();
		if (settings)
		{
			m_fBleedOutTime = settings.m_fBleedoutTime;
			PrintFormat("[NoInstantDeath] Loaded bleedout time: %1s", m_fBleedOutTime);
		}
		else
		{
			Print("[NoInstantDeath] No settings found — using default bleedout time.");
		}
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bIsUnconscious || !Replication.IsServer())
			return;

		m_fUnconsciousTimer += timeSlice;

		if (m_fUnconsciousTimer >= m_fBleedOutTime)
		{
			Print("[NoInstantDeath] Bleed-out timer expired. Killing character.");
			KillCharacter(owner);
		}
	}

	void MakeUnconscious(IEntity owner)
	{
		if (m_bIsUnconscious)
			return;

		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!dmg)
			return;
            
		// Check if ACE Medical's Second Chance is already triggered
		if (dmg.ACE_Medical_IsInitialized() && dmg.ACE_Medical_WasSecondChanceTrigged())
		{
			Print("[NoInstantDeath] ACE Medical Second Chance already triggered, skipping duplicate unconsciousness.");
			return;
		}

		Print("[NoInstantDeath] Entering unconscious state.");
		m_bIsUnconscious = true;
		m_fUnconsciousTimer = 0.0;

		dmg.ForceUnconsciousness(0.05); // Small health buffer to simulate knockdown

		if (Replication.IsServer())
			Replication.BumpMe();
	}

	void KillCharacter(IEntity owner)
	{
		Print("[NoInstantDeath] Forcing character death.");
		m_bIsUnconscious = false;

		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(owner.FindComponent(SCR_CharacterDamageManagerComponent));
		if (dmg)
			dmg.Kill(dmg.GetInstigator());

		if (Replication.IsServer())
			Replication.BumpMe();
	}

	void OnRep_IsUnconscious()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		PrintFormat("[NoInstantDeath] Replicated unconscious state: %1", m_bIsUnconscious);
	}

	bool IsUnconscious()
	{
		return m_bIsUnconscious;
	}
}
