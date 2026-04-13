class GM_AutoAssignComponentClass : ScriptComponentClass {}

class GM_AutoAssignComponent : ScriptComponent
{
    [Attribute(defvalue: "3000", desc: "How often to evaluate GM assignment (milliseconds)", category: "GM")]
    protected int m_iCheckIntervalMs;

    [Attribute(defvalue: "1", desc: "Revoke GM when a player no longer occupies a ForceGM slot", category: "GM")]
    protected bool m_bRevokeWhenNotForceSlot;

    [Attribute(defvalue: "0", desc: "Enable debug logging", category: "GM")]
    protected bool m_bDebugLog;

    // Track players that this component has promoted, so we do not revoke manual/admin GM.
    protected ref array<int> m_AssignedByComponent = {};

    // Runtime-only tracking used for connection and slot-transition debug.
    protected ref array<int> m_SeenConnectedPlayers = {};
    protected ref array<int> m_LastForceSlotPlayers = {};
    protected ref array<int> m_NotifiedGMPlayers = {};
    protected ref array<int> m_PendingRevokePlayers = {};

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (!Replication.IsServer())
            return;

        int intervalMs = Math.Max(m_iCheckIntervalMs, 500);
        DebugLog(string.Format("Starting GM auto-assign loop. intervalMs=%1 revokeOnExit=%2", intervalMs, m_bRevokeWhenNotForceSlot));
        GetGame().GetCallqueue().CallLater(CheckPlayersLoop, intervalMs, true);
    }

    void CheckPlayersLoop()
    {
        SCR_EditorManagerCore editorCore = GetEditorCore();
        if (!editorCore)
            return;

        array<int> players = {};
        GetGame().GetPlayerManager().GetPlayers(players);

        LogPlayerConnectionEvents(players);

        array<int> currentForceSlotPlayers = {};

        foreach (int playerId : players)
        {
            bool inForceSlot = IsPlayerInForceGMSlot(playerId);

            if (inForceSlot)
                currentForceSlotPlayers.Insert(playerId);

            LogForceSlotTransition(playerId, inForceSlot);

            if (inForceSlot)
            {
                EnsureGM(playerId, editorCore);
            }
            else if (m_bRevokeWhenNotForceSlot)
            {
                RevokeGMIfTracked(playerId, editorCore);
            }
        }

        m_LastForceSlotPlayers = currentForceSlotPlayers;
        PruneTrackedPlayers(players, editorCore);
    }

    protected void LogPlayerConnectionEvents(array<int> players)
    {
        foreach (int playerId : players)
        {
            if (m_SeenConnectedPlayers.Contains(playerId))
                continue;

            m_SeenConnectedPlayers.Insert(playerId);
            // Group/entity assignment happens after connection; force slot check runs on next loop tick.
            DebugLog(string.Format("Player connected: playerId=%1 (group assignment pending)", playerId));
        }

        for (int i = m_SeenConnectedPlayers.Count() - 1; i >= 0; i--)
        {
            int knownPlayerId = m_SeenConnectedPlayers[i];
            if (players.Contains(knownPlayerId))
                continue;

            DebugLog(string.Format("Player disconnected: playerId=%1", knownPlayerId));
            m_SeenConnectedPlayers.Remove(i);
        }
    }

    protected void LogForceSlotTransition(int playerId, bool inForceSlot)
    {
        bool wasInForceSlot = m_LastForceSlotPlayers.Contains(playerId);
        if (wasInForceSlot == inForceSlot)
            return;

        if (inForceSlot)
            DebugLog(string.Format("Player entered ForceGM slot: playerId=%1 group=%2", playerId, GetPlayerGroupDebugName(playerId)));
        else
            DebugLog(string.Format("Player left ForceGM slot: playerId=%1 group=%2", playerId, GetPlayerGroupDebugName(playerId)));
    }

    protected bool IsPlayerInForceGMSlot(int playerId)
    {
        SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
        if (!groupsManager)
        {
            DebugLog("IsPlayerInForceGMSlot: SCR_GroupsManagerComponent not available.");
            return false;
        }

        SCR_AIGroup playerGroup = groupsManager.GetPlayerGroup(playerId);
        if (!playerGroup)
        {
            DebugLog(string.Format("IsPlayerInForceGMSlot: playerId=%1 has no group yet.", playerId));
            return false;
        }

        bool isForceGMEnabled = IsGroupForceGMEnabled(playerGroup, groupsManager);
        return isForceGMEnabled;
    }

    protected bool IsGroupForceGMEnabled(notnull SCR_AIGroup playerGroup, notnull SCR_GroupsManagerComponent groupsManager)
    {
        Faction faction = playerGroup.GetFaction();
        if (!faction)
        {
            DebugLog("IsGroupForceGMEnabled: group has no faction.");
            return false;
        }

        SCR_Faction scrFaction = SCR_Faction.Cast(faction);
        if (!scrFaction)
        {
            DebugLog("IsGroupForceGMEnabled: faction is not SCR_Faction.");
            return false;
        }

        array<ref SCR_GroupPreset> predefinedGroups = {};
        if (!scrFaction.IRRU_GetPredefinedGroups(predefinedGroups) || predefinedGroups.IsEmpty())
        {
            DebugLog("IsGroupForceGMEnabled: no predefined group presets available for faction.");
            return false;
        }

        array<SCR_AIGroup> playableGroups = groupsManager.GetPlayableGroupsByFaction(faction);
        if (!playableGroups || playableGroups.IsEmpty())
        {
            DebugLog("IsGroupForceGMEnabled: no playable groups found for faction.");
            return false;
        }

        int playableIndex = playableGroups.Find(playerGroup);
        if (playableIndex < 0)
        {
            DebugLog("IsGroupForceGMEnabled: player group not found in faction playable groups list.");
            return false;
        }

        if (playableIndex >= predefinedGroups.Count())
        {
            DebugLog(string.Format("IsGroupForceGMEnabled: index mismatch playableIndex=%1 predefinedCount=%2.", playableIndex, predefinedGroups.Count()));
            return false;
        }

        SCR_GroupPreset preset = predefinedGroups[playableIndex];
        if (!preset)
        {
            DebugLog(string.Format("IsGroupForceGMEnabled: null preset at index=%1.", playableIndex));
            return false;
        }

        return preset.HasForceGMSlot();
    }

    protected string GetPlayerGroupDebugName(int playerId)
    {
        SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
        if (!groupsManager)
            return "groupsManager=null";

        SCR_AIGroup group = groupsManager.GetPlayerGroup(playerId);
        if (!group)
            return "group=null";

        Faction faction = group.GetFaction();
        string factionKey = "null";
        SCR_Faction scrFac = SCR_Faction.Cast(faction);
        if (scrFac)
            factionKey = scrFac.GetFactionKey();
        return string.Format("faction='%1' custom='%2' role='%3'", factionKey, group.GetCustomName(), group.GetGroupRoleName());
    }

    protected SCR_EditorManagerCore GetEditorCore()
    {
        SCR_EditorManagerCore editorCore = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
        if (!editorCore)
        {
            DebugLog("Cannot evaluate GM assignment: SCR_EditorManagerCore is not available.");
            return null;
        }

        return editorCore;
    }

    protected bool IsPlayerGameMaster(int playerId, SCR_EditorManagerCore editorCore)
    {
        if (!editorCore)
            return false;

        return editorCore.GetEditorManager(playerId) != null;
    }

    protected void EnsureGM(int playerId, SCR_EditorManagerCore editorCore)
    {
        if (!editorCore)
        {
            DebugLog(string.Format("Cannot grant GM to playerId=%1: editorCore is null.", playerId));
            return;
        }

        bool hadPendingRevoke = IsRevokePending(playerId);
        if (hadPendingRevoke)
        {
            ClearPendingRevoke(playerId);
            DebugLog(string.Format("PlayerId=%1 re-entered ForceGM slot while revoke was pending; ensuring GM remains granted.", playerId));
        }

        // In this branch, GetEditorManager(playerId) can report true before rights are fully active.
        // Only trust that check for players we already promoted and verified.
        if (!hadPendingRevoke && IsTracked(playerId) && IsPlayerGameMaster(playerId, editorCore))
            return;

        if (IsTracked(playerId) && !IsPlayerGameMaster(playerId, editorCore))
            DebugLog(string.Format("PlayerId=%1 was tracked as GM but no longer appears to have GM; retrying grant.", playerId));

        SCR_VotingManagerComponent votingManager = SCR_VotingManagerComponent.GetInstance();
        if (!votingManager)
        {
            DebugLog(string.Format("Cannot grant GM to playerId=%1: SCR_VotingManagerComponent is not available.", playerId));
            return;
        }

        DebugLog(string.Format("Granting GM to playerId=%1 via forced EDITOR_IN vote.", playerId));

        // value must be playerId (the player being voted for), not 0.
        votingManager.StartVoting(EVotingType.EDITOR_IN, playerId, playerId);
        votingManager.EndVoting(EVotingType.EDITOR_IN, playerId, EVotingOutcome.FORCE_WIN);
        DebugLog(string.Format("EDITOR_IN vote force-won for playerId=%1.", playerId));

        TrackPlayer(playerId);

        bool hasGMNow = IsPlayerGameMaster(playerId, editorCore);
        DebugLog(string.Format("Immediate GM check for playerId=%1: hasGM=%2.", playerId, hasGMNow));
        GetGame().GetCallqueue().CallLater(VerifyGrantResult, 750, false, playerId);
    }

    protected void VerifyGrantResult(int playerId)
    {
        SCR_EditorManagerCore editorCore = GetEditorCore();
        if (!editorCore)
        {
            DebugLog(string.Format("Delayed GM verification failed for playerId=%1: editorCore is null.", playerId));
            return;
        }

        bool hasGM = IsPlayerGameMaster(playerId, editorCore);
        if (hasGM)
        {
            NotifyGrantSuccess(playerId);
            DebugLog(string.Format("Delayed GM verification succeeded for playerId=%1.", playerId));
        }
        else
            DebugLog(string.Format("Delayed GM verification failed for playerId=%1: player still does not have GM.", playerId));
    }

    protected void RevokeGMIfTracked(int playerId, SCR_EditorManagerCore editorCore)
    {
        if (!editorCore)
            return;

        if (!IsTracked(playerId))
            return;

        if (!IsPlayerGameMaster(playerId, editorCore))
        {
            UntrackPlayer(playerId);
            ClearPendingRevoke(playerId);
            return;
        }

        if (IsRevokePending(playerId))
            return;

        SCR_VotingManagerComponent votingManager = SCR_VotingManagerComponent.GetInstance();
        if (!votingManager)
        {
            DebugLog(string.Format("Cannot revoke GM from playerId=%1: SCR_VotingManagerComponent is not available.", playerId));
            return;
        }

        MarkPendingRevoke(playerId);
        DebugLog(string.Format("Revoking GM from playerId=%1 via forced EDITOR_OUT vote.", playerId));
        votingManager.StartVoting(EVotingType.EDITOR_OUT, playerId, playerId);
        votingManager.EndVoting(EVotingType.EDITOR_OUT, playerId, EVotingOutcome.FORCE_WIN);

        bool hasGMNow = IsPlayerGameMaster(playerId, editorCore);
        if (!hasGMNow)
        {
            UntrackPlayer(playerId);
            ClearPendingRevoke(playerId);
            NotifyRevokeSuccess(playerId);
            DebugLog(string.Format("Immediate GM revoke check succeeded for playerId=%1.", playerId));
            return;
        }

        DebugLog(string.Format("Immediate GM revoke check inconclusive for playerId=%1; scheduling delayed verification.", playerId));
        GetGame().GetCallqueue().CallLater(VerifyRevokeResult, 750, false, playerId);
    }

    protected void VerifyRevokeResult(int playerId)
    {
        SCR_EditorManagerCore editorCore = GetEditorCore();
        if (!editorCore)
        {
            DebugLog(string.Format("Delayed GM revoke verification failed for playerId=%1: editorCore is null.", playerId));
            return;
        }

        if (!IsRevokePending(playerId))
            return;

        if (IsPlayerInForceGMSlot(playerId))
        {
            ClearPendingRevoke(playerId);
            DebugLog(string.Format("PlayerId=%1 is back in a ForceGM slot during delayed revoke verification; re-ensuring GM.", playerId));
            EnsureGM(playerId, editorCore);
            return;
        }

        bool hasGM = IsPlayerGameMaster(playerId, editorCore);
        if (!hasGM)
        {
            UntrackPlayer(playerId);
            ClearPendingRevoke(playerId);
            NotifyRevokeSuccess(playerId);
            DebugLog(string.Format("Delayed GM revoke verification succeeded for playerId=%1.", playerId));
        }
        else
        {
            DebugLog(string.Format("Delayed GM revoke verification failed for playerId=%1: player still has GM; suppressing repeated revoke retries until state changes.", playerId));
        }
    }

    protected void PruneTrackedPlayers(array<int> activePlayers, SCR_EditorManagerCore editorCore)
    {
        for (int i = m_AssignedByComponent.Count() - 1; i >= 0; i--)
        {
            int trackedPlayerId = m_AssignedByComponent[i];

            if (activePlayers.Contains(trackedPlayerId))
                continue;

            if (editorCore && IsPlayerGameMaster(trackedPlayerId, editorCore))
                DebugLog(string.Format("Tracked GM disconnected playerId=%1 (revoke skipped in branch-safe mode).", trackedPlayerId));

            m_AssignedByComponent.Remove(i);
            ClearPendingRevoke(trackedPlayerId);
            ClearNotificationState(trackedPlayerId);
        }
    }

    protected void NotifyGrantSuccess(int playerId)
    {
        if (m_NotifiedGMPlayers.Contains(playerId))
            return;

        m_NotifiedGMPlayers.Insert(playerId);
        SCR_NotificationsComponent.SendToPlayer(playerId, ENotification.EDITOR_PLAYER_BECAME_GM, vector.Zero, playerId);
        DebugLog(string.Format("Sent GM granted HUD popup to playerId=%1.", playerId));
    }

    protected void NotifyRevokeSuccess(int playerId)
    {
        int index = m_NotifiedGMPlayers.Find(playerId);
        if (index == -1)
            return;

        m_NotifiedGMPlayers.Remove(index);
        SCR_NotificationsComponent.SendToPlayer(playerId, ENotification.EDITOR_PLAYER_NO_LONGER_GM, vector.Zero, playerId);
        DebugLog(string.Format("Sent GM revoked HUD popup to playerId=%1.", playerId));
    }

    protected void ClearNotificationState(int playerId)
    {
        int index = m_NotifiedGMPlayers.Find(playerId);
        if (index != -1)
            m_NotifiedGMPlayers.Remove(index);
    }

    protected bool IsRevokePending(int playerId)
    {
        return m_PendingRevokePlayers.Contains(playerId);
    }

    protected void MarkPendingRevoke(int playerId)
    {
        if (IsRevokePending(playerId))
            return;

        m_PendingRevokePlayers.Insert(playerId);
    }

    protected void ClearPendingRevoke(int playerId)
    {
        int index = m_PendingRevokePlayers.Find(playerId);
        if (index != -1)
            m_PendingRevokePlayers.Remove(index);
    }

    protected bool IsTracked(int playerId)
    {
        return m_AssignedByComponent.Contains(playerId);
    }

    protected void TrackPlayer(int playerId)
    {
        if (IsTracked(playerId))
            return;

        m_AssignedByComponent.Insert(playerId);
    }

    protected void UntrackPlayer(int playerId)
    {
        int index = m_AssignedByComponent.Find(playerId);
        if (index != -1)
            m_AssignedByComponent.Remove(index);
    }

    protected void DebugLog(string message)
    {
        if (!m_bDebugLog)
            return;

        Print(string.Format("[IRRU_AutoAssignGM] %1", message));
    }
}