[BaseContainerProps()]
modded class SCR_GroupPreset
{
    [Attribute(defvalue: "0", desc: "If enabled, this group acts as a ForceGM slot and grants Game Master permissions", category: "GM")]
    bool m_bForceGM;

    bool HasForceGMSlot()
    {
        return m_bForceGM;
    }

    bool IsForceGM()
    {
        // Backward-compatible alias for existing scripts.
        return HasForceGMSlot();
    }
}

modded class SCR_Faction
{
    bool IRRU_GetPredefinedGroups(out array<ref SCR_GroupPreset> outGroups)
    {
        outGroups = {};

        if (!m_aPredefinedGroups)
            return false;

        foreach (SCR_GroupPreset preset : m_aPredefinedGroups)
        {
            if (!preset)
                continue;

            outGroups.Insert(preset);
        }

        return !outGroups.IsEmpty();
    }
}