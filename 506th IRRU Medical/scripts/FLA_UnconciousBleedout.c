if (!isServer) exitWith {};

// Prevent instant death by overriding the damage event handler
addMissionEventHandler ["EntityKilled", {
    params ["_unit", "_killer", "_instigator"];
    
    if (_unit isKindOf "Man") then {
        private _damage = damage _unit;
        
        // If the unit is above 0 damage but still alive, force unconscious state
        if (_damage >= 1) then {
            // Check if the second chance is available
            if (_unit ACE_Medical_HasSecondChance()) then {
                // Trigger second chance if possible
                _unit ACE_Medical_SetSecondChanceTrigged(true);
                
                // Keep unit in critical condition but not dead
                _unit setDamage 0.9; // Critical state but not dead
                _unit setVariable ["ACE_Medical_HasCriticalHealth", true, true];
            } else {
                // If second chance is not triggered, kill the unit
                _unit setDamage 1;
            };
        };
    };
}];

// Make unconscious players invulnerable while being carried
addMissionEventHandler ["EachFrame", {
    {
        if ((_x getVariable ["ACE_Medical_HasCriticalHealth", false]) && (isNull objectParent _x)) then {
            _x allowDamage false;
        } else {
            _x allowDamage true;
        };
    } forEach allPlayers;
}];

// Allow death through bleed-out and second chance health regeneration
addMissionEventHandler ["EachFrame", {
    {
        if ((_x getVariable ["ACE_Medical_HasCriticalHealth", false]) && (damage _x >= 0.95)) then {
            // If damage is very high, set to death state
            _x setDamage 1;
        } else if (_x ACE_Medical_WasSecondChanceTrigged()) then {
            // If second chance was triggered, start regeneration based on ACE Medical's settings
            private _regenScale = _x ACE_Medical_GetResilienceRegenScale();
            private _currentHealth = _x getHitZoneDamage("Health");
            
            // Regenerate health if in second chance state
            if (_currentHealth < 1) then {
                private _newHealth = _currentHealth + (0.05 * _regenScale); // Example regen rate
                _x setHitZoneDamage("Health", _newHealth);
            }
        };
    } forEach allPlayers;
}];

// Check if the unit has second chance after health drop
addMissionEventHandler ["EachFrame", {
    {
        // Check for second chance eligibility
        if ((_x ACE_Medical_HasCriticalHealth()) && !_x ACE_Medical_WasSecondChanceTrigged()) then {
            // Trigger second chance if the player is critically hurt and hasn't had it triggered
            _x ACE_Medical_SetSecondChanceTrigged(true);
            _x setDamage 0.9; // Critical health but prevent instant death
            _x setVariable ["ACE_Medical_HasCriticalHealth", true, true];
        };
    } forEach allPlayers;
}];
