//------------------------------------------------------------------------------------------------
//! DIAGNOSTICS ONLY - THIS COMPONENT NO LONGER CONTROLS BLEEDING.
//!
//! History: this override used to force m_fDOTScale from IRRU settings on a delayed callback,
//! assuming the engine applied it to ACE's blood drain. Server log evidence (Nizla dedicated,
//! 2026-07-19) proved the knob is INERT under ACE Medical: the component verifiably held 0.25
//! while actual drain matched the raw wound sum 1:1, because ACE_Medical_BloodLossDamageEffect
//! consumes SCR_CharacterBloodHitZone.GetTotalBleedingAmount() directly and nothing downstream
//! reads the game mode scale. The real scale and cap now live in the modded
//! SCR_CharacterBloodHitZone.GetTotalBleedingAmount() in this mod.
//!
//! This override only prints the resolved values at startup so the RPT shows, in one line, what
//! the IRRU config actually enforces versus what the inert game mode knobs happen to hold (ACE
//! still writes them from its own settings in OnPostInit; mission templates also set them). Do
//! not "fix" bleeding by editing m_fDOTScale anywhere - prefab, layer, or server config - it
//! does nothing under ACE. Edit IRRU_NoInstantDeathSettings.conf instead.
//------------------------------------------------------------------------------------------------
modded class SCR_GameModeHealthSettings : ScriptComponent
{
	protected const float IRRU_LOG_DELAY_MS = 5000;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || !Replication.IsServer())
			return;

		GetGame().GetCallqueue().CallLater(IRRU_LogHealthSettings, IRRU_LOG_DELAY_MS, false);
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(IRRU_LogHealthSettings);

		super.OnDelete(owner);
	}

	protected void IRRU_LogHealthSettings()
	{
		Print(string.Format("[NoInstantDeath] Health settings resolved - IRRU BleedingScale: %1 (enforced at blood hitzone), IRRU MaxTotalBleedRate: %2 ml/s effective, GameMode DOTScale: %3 (inert under ACE), RegenScale: %4",
			IRRU_NoInstantDeathSettings.GetBleedingRateScale(), IRRU_NoInstantDeathSettings.GetMaxTotalBleedingRate(), GetBleedingScale(), GetRegenScale()));
	}
}
