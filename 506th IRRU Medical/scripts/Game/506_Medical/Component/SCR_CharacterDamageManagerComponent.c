// ============================================================================
//  SCR_CharacterDamageManagerComponent.c     (modded, NID fixes 2025-06-15)
//
//  • Fix A – blocks DESTROYED while the custom bleed-out timer is active
//  • Fix B – intercepts the first lethal hit before health reaches 0
//  • Fix C – adds a 1 HP safety buffer when we knock the player out
// ----------------------------------------------------------------------------

modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	// Public invoker so other scripts can subscribe
	ref ScriptInvoker OnCustomDamageTaken = new ScriptInvoker();

	// ─── helper: readable entity / player name ────────────────────────────
	protected string GetPlayerOrEntityNameStr(IEntity entity)
	{
		if (!entity)
			return "UnknownEntity(null)";

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
					if (!n.IsEmpty())
						return n;
				}
			}
		}
		return entity.ToString();
	}

	// ───────────────────────────────────────────────────────────────────────
	//  FIX B – intercept lethal damage before super.OnDamage()
	// ───────────────────────────────────────────────────────────────────────
	override void OnDamage(notnull BaseDamageContext damageContext)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		// 1) First lethal hit → convert to knock-out
		if (nid && !nid.IsUnconscious())
		{
			float projectedHealth = GetHealth() - damageContext.damageValue;
			if (projectedHealth <= 0.1)
			{
				nid.MakeUnconscious(owner);

				// FIX C – ensure at least 1 HP so DESTROYED isn’t queued
				HitZone core = GetDefaultHitZone();
				if (core && core.GetHealth() < 1.0)
					core.SetHealth(1.0);

				return;                           // skip super → health stays > 0
			}
		}

		// 2) While unconscious, ignore any non-healing damage
		if (nid && nid.IsUnconscious() && damageContext.damageType != EDamageType.HEALING)
			return;

		// 3) Normal processing
		super.OnDamage(damageContext);
		vector zeroVec = vector.Zero;
		OnCustomDamageTaken.Invoke(owner, damageContext.damageValue, damageContext.instigator, zeroVec, damageContext.struckHitZone);
	}

	// ───────────────────────────────────────────────────────────────────────
	//  FIX A – block DESTROYED while custom bleed-out owns the life-cycle
	// ───────────────────────────────────────────────────────────────────────
	override void OnDamageStateChanged(EDamageState state)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (nid && nid.IsUnconscious() && !nid.IsInitiatingKill() && state == EDamageState.DESTROYED)
			return;                       // swallow fatal state during bleed-out

		super.OnDamageStateChanged(state);
	}

	// ───────────────────────────────────────────────────────────────────────
	//  Respect the bleed-out timer in Kill()
	// ───────────────────────────────────────────────────────────────────────
	override void Kill(notnull Instigator instigator)
	{
		IEntity owner = GetOwner();
		NoInstantDeathComponent nid = null;
		if (owner)
			nid = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (nid)
		{
			// 1) Our timer reached 360 s → allow true death
			if (nid.IsInitiatingKill())
			{
				nid.ResetInitiatingKillFlag();
				super.Kill(instigator);
				return;
			}

			// 2) Ignore external Kill() calls while unconscious
			if (nid.IsUnconscious())
				return;

			// 3) Unexpected lethal call → convert to knock-out
			nid.MakeUnconscious(owner);
			HitZone core = GetDefaultHitZone();
			if (core && core.GetHealth() < 1.0)
				core.SetHealth(1.0);
			return;
		}

		// Entity has no NID component → vanilla behaviour
		super.Kill(instigator);
	}
}
