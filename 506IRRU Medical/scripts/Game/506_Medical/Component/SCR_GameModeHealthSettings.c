modded class SCR_GameModeHealthSettings : ScriptComponent
{
	protected const float IRRU_APPLY_DELAY_MS = 5000;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;

		GetGame().GetCallqueue().CallLater(IRRU_ApplyBleedingSettings, IRRU_APPLY_DELAY_MS, false);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(IRRU_ApplyBleedingSettings);

		super.OnDelete(owner);
	}

	protected void IRRU_ApplyBleedingSettings()
	{
		ACE_Medical_Core_Settings settings = ACE_SettingsHelperT<ACE_Medical_Core_Settings>.GetModSettings();
		if (!settings)
		{
			Print("[NoInstantDeath] ACE mod settings unavailable, bleeding rate left at engine default", LogLevel.WARNING);
			return;
		}

		SetBleedingScale(settings.m_fBleedingRateScale);
		SetRegenScale(settings.m_fBloodRegenScale);

		Print(string.Format("[NoInstantDeath] Bleeding settings applied - BleedingRateScale: %1, BloodRegenScale: %2", GetBleedingScale(), GetRegenScale()));
	}
}
