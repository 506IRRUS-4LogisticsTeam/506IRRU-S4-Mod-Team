modded class SCR_CharacterDamageManagerComponent : SCR_CharacterDamageManagerComponent
{
	ref ScriptInvoker OnCustomDamageTaken = new ScriptInvoker();

	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);
		
		PrintFormat("[Debug] DamageContext: %1", damageContext);
		
		OnCustomDamageTaken.Invoke(
			GetOwner(),
			damageContext.damageValue,
			damageContext.instigator,
			"0 0 0", // Placeholder for direction if none available
			damageContext.struckHitZone
		);
	}
}
