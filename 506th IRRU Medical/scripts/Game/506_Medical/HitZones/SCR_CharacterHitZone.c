//! Modded character health hit zone to guarantee Second Chance while unconscious

modded class SCR_CharacterHealthHitZone : SCR_HitZone
{
	//------------------------------------------------------------------------------------------------
	//! Override damage calculation to ensure unconscious characters always get Second Chance
	override float ComputeEffectiveDamage(notnull BaseDamageContext damageContext, bool isDOT)
	{
		SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(GetHitZoneContainer());
		if (damageManager)
		{
			IEntity owner = damageManager.GetOwner();
			if (owner)
			{
				IRRU_NoInstantDeathComponent nid = IRRU_NoInstantDeathComponent.Cast(
					owner.FindComponent(IRRU_NoInstantDeathComponent));

				if (nid && nid.IsUnconscious())
				{
					damageManager.ACE_Medical_EnableSecondChance(true);
				}
			}
		}

		return super.ComputeEffectiveDamage(damageContext, isDOT);
	}
}
