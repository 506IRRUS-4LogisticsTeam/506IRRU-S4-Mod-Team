// Modifies the base Character Damage Manager
modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	// Existing invoker for custom damage events
	ref ScriptInvoker OnCustomDamageTaken = new ScriptInvoker();

	// Helper to get player name or entity string (can be duplicated or moved to a shared utility script)
	protected string GetPlayerOrEntityNameStr(IEntity entity)
	{
		if (!entity) return "UnknownEntity(null)";

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			if (character)
			{
				int playerId = playerManager.GetPlayerIdFromControlledEntity(character);
				if (playerId > 0) 
				{
					string playerName = playerManager.GetPlayerName(playerId);
					if (!playerName.IsEmpty())
					{
						return playerName;
					}
				}
			}
		}
		return entity.ToString(); // Fallback
	}

	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		IEntity owner = GetOwner();
		if (owner)
		{
			NoInstantDeathComponent noInstantDeathComp = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
			if (noInstantDeathComp && noInstantDeathComp.IsUnconscious())
			{
				if (damageContext.damageType != EDamageType.HEALING)
				{
					// PrintFormat("[SCR_CharacterDamageManagerComponent][NoInstantDeath] %1: Blocking non-healing damage (Type: %2) while unconscious.", GetPlayerOrEntityNameStr(owner), typename.EnumToString(EDamageType, damageContext.damageType), LogLevel.DEBUG); // Production: Can be verbose
					return; 
				}
			}
		}

		super.OnDamage(damageContext);
		vector hitDirPlaceholder = vector.Zero;
		OnCustomDamageTaken.Invoke( owner, damageContext.damageValue, damageContext.instigator, hitDirPlaceholder, damageContext.struckHitZone);
	}

	override void Kill(notnull Instigator instigator)
	{
		IEntity owner = GetOwner();
		// PrintFormat("[SCR_CharacterDamageManagerComponent] %1: Kill called. Instigator: %2", GetPlayerOrEntityNameStr(owner), instigator); // Production: Verbose

		if (!owner) 
		{
			// PrintFormat("[SCR_CharacterDamageManagerComponent] Kill called on null owner. Instigator: %1. Calling super.Kill().", instigator); // Production: Verbose
			super.Kill(instigator); 
			return; 
		}

		NoInstantDeathComponent noInstantDeathComp = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (noInstantDeathComp) 
		{
			if (noInstantDeathComp.IsInitiatingKill())
			{
				// Logged by NoInstantDeathComponent: "Bleed-out timer expired. Character died."
				// PrintFormat("[SCR_CharacterDamageManagerComponent] %1: Kill confirmed by NoInstantDeathComponent flag. Instigator: %2", GetPlayerOrEntityNameStr(owner), instigator); // Production: Redundant
				noInstantDeathComp.ResetInitiatingKillFlag(); 
				super.Kill(instigator); 
				return;
			}

			if (noInstantDeathComp.IsUnconscious())
			{
				// PrintFormat("[SCR_CharacterDamageManagerComponent] %1: Kill called externally while already unconscious (NoInstantDeath). Ignoring. Instigator: %2", GetPlayerOrEntityNameStr(owner), instigator); // Production: Verbose
				return;
			}
			else
			{
				// Logged by NoInstantDeathComponent: "Entering unconscious state (bleed-out)."
				// PrintFormat("[SCR_CharacterDamageManagerComponent] %1: Intercepting initial Kill(). Calling MakeUnconscious(). Instigator: %2", GetPlayerOrEntityNameStr(owner), instigator); // Production: Redundant
				noInstantDeathComp.MakeUnconscious(owner);
				return; 
			}
		}
		else 
		{
			PrintFormat("[SCR_CharacterDamageManagerComponent] %1: NoInstantDeathComponent not found. Standard Kill. Instigator: %2", GetPlayerOrEntityNameStr(owner), instigator);
			super.Kill(instigator);
		}
	}

	override protected void OnDamageStateChanged(EDamageState state)
	{
		IEntity owner = GetOwner();
		if (!owner) 
		{
			// PrintFormat("[SCR_CharacterDamageManagerComponent] OnDamageStateChanged called on null owner. NewState: %1", typename.EnumToString(EDamageState, state)); // Production: Verbose
			super.OnDamageStateChanged(state);
			return;
		}

		// PrintFormat("[SCR_CharacterDamageManagerComponent] %1: OnDamageStateChanged. NewState: %2", GetPlayerOrEntityNameStr(owner), typename.EnumToString(EDamageState, state)); // Production: Highly verbose, logged by NoInstantDeathComponent anyway.

		if (state == EDamageState.DESTROYED)
		{
			NoInstantDeathComponent noInstantDeathComp = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
			if (noInstantDeathComp && !noInstantDeathComp.IsUnconscious() && !noInstantDeathComp.IsInitiatingKill())
			{
				// PrintFormat("[SCR_CharacterDamageManagerComponent] %1: Intercepting external/initial DESTROYED state. Calling MakeUnconscious().", GetPlayerOrEntityNameStr(owner)); // Production: Verbose, MakeUnconscious will log.
				noInstantDeathComp.MakeUnconscious(owner);
				return;
			}
		}
		super.OnDamageStateChanged(state);
	}
}