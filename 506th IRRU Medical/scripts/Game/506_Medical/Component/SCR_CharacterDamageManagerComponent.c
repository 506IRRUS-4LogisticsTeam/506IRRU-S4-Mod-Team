// Modifies the base Character Damage Manager
modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	// Existing invoker for custom damage events
	ref ScriptInvoker OnCustomDamageTaken = new ScriptInvoker();

	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);
		// Use vector.Zero as a placeholder for hit direction if not directly available
		vector hitDirPlaceholder = vector.Zero;
		OnCustomDamageTaken.Invoke( GetOwner(), damageContext.damageValue, damageContext.instigator, hitDirPlaceholder, damageContext.struckHitZone);
	}

	// Override the default Kill behavior
	override void Kill(notnull Instigator instigator) 
	{
		IEntity owner = GetOwner();
		if (!owner) { super.Kill(instigator); return; } // Basic check

		// Check if our NoInstantDeathComponent is present
		NoInstantDeathComponent noInstantDeathComp = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));

		if (noInstantDeathComp) // Component found
		{
			// Did the component's timer trigger this Kill? (Check the flag)
			if (noInstantDeathComp.IsInitiatingKill())
			{
				// Yes, proceed with the actual death
				Print("[SCR_CharacterDamageManagerComponent] Kill confirmed by NoInstantDeathComponent flag. Resetting flag and calling super.Kill().");
				noInstantDeathComp.ResetInitiatingKillFlag(); 
				super.Kill(instigator); 
				return;
			}

			// If timer didn't trigger it, check if already unconscious
			if (noInstantDeathComp.IsUnconscious())
			{
				// Already down, ignore this external/duplicate Kill call
				Print("[SCR_CharacterDamageManagerComponent] Kill called externally while already unconscious. Ignoring.");
				return; 
			}
			else
			{
				// Initial lethal hit, intercept and make unconscious instead
				Print("[SCR_CharacterDamageManagerComponent] Intercepting initial Kill(). Calling MakeUnconscious().");
				noInstantDeathComp.MakeUnconscious(owner);
				return; // Prevent super.Kill()
			}
		}
		else // Component not found
		{
			// No component, normal death behavior
			Print("[SCR_CharacterDamageManagerComponent] NoInstantDeathComponent not found. Proceeding with original Kill().");
			super.Kill(instigator);
		}
	}

	// Override state changes as a backup interceptor
	override protected void OnDamageStateChanged(EDamageState state)
	{
		// Intercept direct state change to DESTROYED if it wasn't initiated by our component's timer
		if (state == EDamageState.DESTROYED)
		{
			IEntity owner = GetOwner();
			if (owner)
			{
				NoInstantDeathComponent noInstantDeathComp = NoInstantDeathComponent.Cast(owner.FindComponent(NoInstantDeathComponent));
				// Check: Component exists? Not already unconscious? Not the one killing?
				if (noInstantDeathComp && !noInstantDeathComp.IsUnconscious() && !noInstantDeathComp.IsInitiatingKill())
				{
					Print("[SCR_CharacterDamageManagerComponent] Intercepting DESTROYED state change. Calling MakeUnconscious().");
					noInstantDeathComp.MakeUnconscious(owner);
					// Allow state change to proceed for animations etc., Kill() override prevents final death action.
				}
			}
		}
		// Allow other state changes to proceed normally
		super.OnDamageStateChanged(state);
	}
}
