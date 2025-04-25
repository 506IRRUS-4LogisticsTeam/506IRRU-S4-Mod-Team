modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	ref ScriptInvoker OnCustomDamageTaken = new ScriptInvoker();

	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);
		vector hitDirPlaceholder = vector.Zero;
		OnCustomDamageTaken.Invoke( GetOwner(), damageContext.damageValue, damageContext.instigator, hitDirPlaceholder, damageContext.struckHitZone);
	}

	override void Kill(notnull Instigator instigator)
	{
		IEntity owner = GetOwner();
		if (!owner) {
			super.Kill(instigator);
			return;
		}

		NoInstantDeathComponent noInstantDeathComp = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		// If the component exists...
		if (noInstantDeathComp)
		{
			// Check if the component's timer triggered this Kill call
			if (noInstantDeathComp.IsInitiatingKill())
			{
				Print("[SCR_CharacterDamageManagerComponent] Kill confirmed by NoInstantDeathComponent flag. Resetting flag and calling super.Kill().");
				noInstantDeathComp.ResetInitiatingKillFlag(); // Reset the flag
				super.Kill(instigator); // Proceed with original death
				return;
			}

			// If it wasn't the component timer, check if player is already unconscious
			if (noInstantDeathComp.IsUnconscious())
			{
				// Player is already down, but the call didn't come from the component timer. Ignore it.
				Print("[SCR_CharacterDamageManagerComponent] Kill called externally while already unconscious. Ignoring.");
				return; 
			}
			else
			{
				// Player is not unconscious yet, this is the initial lethal hit. Intercept it.
				Print("[SCR_CharacterDamageManagerComponent] Intercepting initial Kill(). Calling MakeUnconscious().");
				noInstantDeathComp.MakeUnconscious(owner);
				return; // Prevent original death
			}
		}
		else // Component doesn't exist
		{
			Print("[SCR_CharacterDamageManagerComponent] NoInstantDeathComponent not found. Proceeding with original Kill().");
			super.Kill(instigator);
		}
	}

	override protected void OnDamageStateChanged(EDamageState state)
	{
		// This override remains important as a backup interceptor
		if (state == EDamageState.DESTROYED)
		{
			IEntity owner = GetOwner();
			if (owner)
			{
				NoInstantDeathComponent noInstantDeathComp = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
				// Check component exists AND is NOT already unconscious AND is NOT the one initiating the kill
				if (noInstantDeathComp && !noInstantDeathComp.IsUnconscious() && !noInstantDeathComp.IsInitiatingKill())
				{
					Print("[SCR_CharacterDamageManagerComponent] Intercepting DESTROYED state change. Calling MakeUnconscious().");
					noInstantDeathComp.MakeUnconscious(owner);
					// Don't return, let state change proceed for anims, Kill() override handles the final block.
				}
			}
		}
		super.OnDamageStateChanged(state);
	}
}
