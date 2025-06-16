// ============================================================================
//  NID_DamageManager.c  (No-Instant-Death patch, ACE-compatible)  •  v6
//  – Restores OnCustomDamageTaken invoker
//  – Uses portable hit-zone top-up (core, Head, Torso)
// ============================================================================

// Note! Enforce script has no ternary operator, so we use an explicit check.

modded class SCR_CharacterDamageManagerComponent
    : SCR_CharacterDamageManagerComponent
{
	// Public invoker (DamageHandler subscribes here)
	ref ScriptInvoker OnCustomDamageTaken = new ScriptInvoker();

	// ─── debug helper ─────────────────────────────────────────────────────
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

	// ───────────────────────────────────────────────────────────────────────
	override void OnDamage(notnull BaseDamageContext damageContext)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		// EARLY-OUT: AI or un-initialised pawn
		if (!nid || !nid.NID_IsInitialized())
		{
			super.OnDamage(damageContext);
			vector zeroVecEarly = vector.Zero;
			OnCustomDamageTaken.Invoke(owner, damageContext.damageValue,
			                           damageContext.instigator, zeroVecEarly,
			                           damageContext.struckHitZone);
			return;
		}

		// First lethal hit → knock-out
		if (!nid.IsUnconscious())
		{
			float projected = GetHealth() - damageContext.damageValue;
			if (projected <= 0.1)
			{
				nid.MakeUnconscious(owner);

				// ── Portable 5-HP buffer on core, Head, Torso ──
				HitZone core = GetDefaultHitZone();
				if (core && core.GetHealth() < 5.0) core.SetHealth(5.0);

				HitZone head = GetHitZoneByName("Head");
				if (head && head.GetHealth() < 5.0) head.SetHealth(5.0);

				HitZone torso = GetHitZoneByName("Torso");
				if (torso && torso.GetHealth() < 5.0) torso.SetHealth(5.0);
				// if any other vital zones need to be added, do so here

				NID_DebugPrint(string.Format(
					"%1 – lethal hit intercepted (knock-out)",
					GetPlayerOrEntityNameStr(owner)));
				return;
			}
		}

		// Ignore damage while unconscious (unless healing)
		if (nid.IsUnconscious() && damageContext.damageType != EDamageType.HEALING)
			return;

		// Normal processing
		super.OnDamage(damageContext);
		vector zeroVec = vector.Zero;
		OnCustomDamageTaken.Invoke(owner, damageContext.damageValue,
		                           damageContext.instigator, zeroVec,
		                           damageContext.struckHitZone);
	}

	// ───────────────────────────────────────────────────────────────────────
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

		// Block DESTROYED during bleed-out
		if (nid.IsUnconscious() && !nid.IsInitiatingKill()
		    && state == EDamageState.DESTROYED)
		{
			NID_DebugPrint(string.Format(
				"%1 – DESTROYED state intercepted",
				GetPlayerOrEntityNameStr(owner)));
			return;
		}

		super.OnDamageStateChanged(state);
	}

	// ───────────────────────────────────────────────────────────────────────
	override void Kill(notnull Instigator instigator)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		// AI or un-initialised → vanilla
		if (!nid || !nid.NID_IsInitialized())
		{
			super.Kill(instigator);
			return;
		}

		// Timer-expired kill allowed
		if (nid.IsInitiatingKill())
		{
			nid.ResetInitiatingKillFlag();
			super.Kill(instigator);
			return;
		}

		// Ignore external Kill() while unconscious
		if (nid.IsUnconscious())
		{
			NID_DebugPrint(string.Format(
				"%1 – Kill() ignored while unconscious",
				GetPlayerOrEntityNameStr(owner)));
			return;
		}

		// Unexpected lethal call → knock-out
		nid.MakeUnconscious(owner);
		HitZone core = GetDefaultHitZone();
		if (core && core.GetHealth() < 1.0)
			core.SetHealth(1.0);

		NID_DebugPrint(string.Format(
			"%1 – Kill() intercepted, converted to knock-out",
			GetPlayerOrEntityNameStr(owner)));
	}
}
