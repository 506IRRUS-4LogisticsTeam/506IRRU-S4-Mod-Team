// ============================================================================
//  SCR_CharacterDamageManagerComponent.c  (No‑Instant‑Death patch, ACE‑compatible)  •  v7
//  – Prevents any vital hit‑zone from falling below 1 HP
//  – Restores OnCustomDamageTaken invoker
// ============================================================================

modded class SCR_CharacterDamageManagerComponent
    : SCR_CharacterDamageManagerComponent
{
	// Public invoker for other scripts
	ref ScriptInvoker OnCustomDamageTaken = new ScriptInvoker();

	// ─── debug ───────────────────────────────────────────────────────────
	protected void NID_DebugPrint(string msg)
	{
		if (NoInstantDeath_Settings.IsDebugEnabled())
			Print("[NoInstantDeath][DMG] " + msg);
	}

	protected string GetPlayerOrEntityNameStr(IEntity entity)
	{
		if (!entity) return "UnknownEntity(null)";

		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			SCR_ChimeraCharacter chr = SCR_ChimeraCharacter.Cast(entity);
			if (chr)
			{
				int pid = pm.GetPlayerIdFromControlledEntity(chr);
				if (pid > 0)
				{
					string n = pm.GetPlayerName(pid);
					if (!n.IsEmpty()) return n;
				}
			}
		}
		return entity.ToString();
	}

	// ─────────────────────────────────────────────────────────────────────
	override void OnDamage(notnull BaseDamageContext damageContext)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(
				owner.FindComponent(NoInstantDeathComponent));

		// —— 1) AI or un‑initialised pawn → vanilla path ——
		if (!nid || !nid.NID_IsInitialized())
		{
			super.OnDamage(damageContext);
			vector zv = vector.Zero;
			OnCustomDamageTaken.Invoke(owner, damageContext.damageValue,
			                           damageContext.instigator, zv,
			                           damageContext.struckHitZone);
			return;
		}

		// —— 2) First lethal hit while conscious → knock‑out ——
		if (!nid.IsUnconscious())
		{
			float projected = GetHealth() - damageContext.damageValue;
			if (projected <= 0.1)
			{
				nid.MakeUnconscious(owner);

				// Apply 5‑HP buffer but ALSO guarantee ≥1 HP everywhere
				EnforceMinHealth(GetDefaultHitZone(),     5.0);
				EnforceMinHealth(GetHitZoneByName("Head"),5.0);
				EnforceMinHealth(GetHitZoneByName("Torso"),5.0);

				NID_DebugPrint(string.Format(
					"%1 – lethal hit intercepted (knock‑out)",
					GetPlayerOrEntityNameStr(owner)));
				return;
			}
		}

		// —— 3) Damage while unconscious (non‑healing) ——
		if (nid.IsUnconscious() && damageContext.damageType != EDamageType.HEALING)
		{
			// Keep both struck zone and core ≥1 HP
			EnforceMinHealth(damageContext.struckHitZone, 1.0);
			EnforceMinHealth(GetDefaultHitZone(),         1.0);
			return;
		}

		// —— 4) Normal pass‑through ——
		super.OnDamage(damageContext);
		vector zv2 = vector.Zero;
		OnCustomDamageTaken.Invoke(owner, damageContext.damageValue,
		                           damageContext.instigator, zv2,
		                           damageContext.struckHitZone);
	}

	// Utility: clamp zone health
	protected void EnforceMinHealth(HitZone hz, float minHP)
	{
		if (hz && hz.GetHealth() < minHP)
			hz.SetHealth(minHP);
	}

	// ─────────────────────────────────────────────────────────────────────
	override void OnDamageStateChanged(EDamageState state)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (!nid || !nid.NID_IsInitialized())
		{
			super.OnDamageStateChanged(state);
			return;
		}

		if (nid.IsUnconscious() && !nid.IsInitiatingKill()
		    && state == EDamageState.DESTROYED)
		{
			NID_DebugPrint(string.Format(
				"%1 – DESTROYED state intercepted", GetPlayerOrEntityNameStr(owner)));
			return;
		}
		super.OnDamageStateChanged(state);
	}

	// ─────────────────────────────────────────────────────────────────────
	override void Kill(notnull Instigator instigator)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (!nid || !nid.NID_IsInitialized())
		{
			super.Kill(instigator);
			return;
		}

		if (nid.IsInitiatingKill())
		{
			nid.ResetInitiatingKillFlag();
			super.Kill(instigator);
			return;
		}

		if (nid.IsUnconscious())
		{
			NID_DebugPrint(string.Format(
				"%1 – Kill() ignored while unconscious",
				GetPlayerOrEntityNameStr(owner)));
			return;
		}

		nid.MakeUnconscious(owner);
		EnforceMinHealth(GetDefaultHitZone(), 1.0);

		NID_DebugPrint(string.Format(
			"%1 – Kill() intercepted, converted to knock‑out",
			GetPlayerOrEntityNameStr(owner)));
	}
}
