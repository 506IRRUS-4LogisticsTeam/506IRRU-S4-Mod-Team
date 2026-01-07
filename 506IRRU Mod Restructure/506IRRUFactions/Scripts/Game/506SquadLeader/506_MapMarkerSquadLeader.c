modded class SCR_MapMarkerSquadLeader : SCR_MapMarkerSquadLeader
{
	override void UpdateLocalVisibility()
	{
		m_bDoLocalVisibilityUpdate = false;
	
		PlayerController pController = GetGame().GetPlayerController();
		if (!pController)
			return;
	
		// MOD: squad leader sees their own marker
		if (m_PlayerID == pController.GetPlayerId())
		{
			SetLocalVisible(true);
	
			// Only update player affiliation if widget exists AND group is assigned
			if (m_SquadLeaderWidgetComp && m_Group)
				UpdatePlayerAffiliation();
	
			return;
		}
	
		// Original visibility logic for all other players
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;
	
		SCR_AIGroup localPlayerGroup = groupManager.GetPlayerGroup(pController.GetPlayerId());
		if (!localPlayerGroup)
		{
			SetLocalVisible(false);
			return;
		}
	
		bool isLocalPlayerLeader = localPlayerGroup.IsPlayerLeader(pController.GetPlayerId());
	
		if (isLocalPlayerLeader && CanLeaderSeeOtherLeaders())
		{
			SetLocalVisible(true);
			if (m_SquadLeaderWidgetComp && m_Group)
				UpdatePlayerAffiliation();
			return;
		}
	
		if (!isLocalPlayerLeader &&
			(CanMemberSeeOtherLeaders() || localPlayerGroup.IsPlayerInGroup(m_PlayerID)))
		{
			SetLocalVisible(true);
			if (m_SquadLeaderWidgetComp && m_Group)
				UpdatePlayerAffiliation();
			return;
		}
	
		SetLocalVisible(false);
	}

}
