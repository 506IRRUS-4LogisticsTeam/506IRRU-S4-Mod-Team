//------------------------------------------------------------------------------------------------
//! WARNING - INTENTIONALLY OVERRIDES ACE MEDICAL. LOAD ORDER SENSITIVE.
//!
//! ACE Medical (ACE_Medical_Bleeding) also mods this class and writes BOTH m_fDOTScale and
//! m_fRegenScale inside its OnPostInit, sourced from ACE_Medical_Core_Settings. ACE calls super,
//! and so do we, so ACE's OnPostInit still runs - we do not block it. We instead overwrite the
//! bleeding scale afterwards on a delayed callback, so the last writer (this mod) wins.
//!
//! Why: ACE reads its value once at init from ACE mod settings, which on our servers did not
//! reliably reflect the mission-header config (players bled out at 0% blood despite
//! m_bBleedOutForPlayersEnabled being 0). This mod owns the bleeding rate instead, sourced from
//! IRRU_NoInstantDeathSettings so it behaves identically in Workbench and on a dedicated server.
//!
//! Compatibility consequences:
//! - ACE's m_fBleedingRateScale (server config / mission header) is INERT for bleeding.
//!   Change the rate in IRRU_NoInstantDeathSettings.conf, not in the server config.
//! - m_fRegenScale is deliberately NOT touched here, so ACE still owns blood regen whenever its
//!   settings are loaded; it falls back to the mission/prefab value when they are not.
//! - Mission and prefab m_fDOTScale values are overwritten at runtime, so per-world bleed tuning
//!   no longer has any effect.
//! - Any mod that writes m_fDOTScale later than IRRU_APPLY_DELAY_MS will beat this and win.
//! - Any mod that overrides OnPostInit WITHOUT calling super stops our apply from ever being
//!   scheduled, silently reverting bleeding to whatever ACE or the mission set.
//------------------------------------------------------------------------------------------------
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
		SetBleedingScale(IRRU_NoInstantDeathSettings.GetBleedingRateScale());

		Print(string.Format("[NoInstantDeath] Health settings resolved - BleedingScale: %1, RegenScale: %2, MaxTotalBleedRate: %3 ml/s",
			GetBleedingScale(), GetRegenScale(), IRRU_NoInstantDeathSettings.GetMaxTotalBleedingRate()));
	}
}
