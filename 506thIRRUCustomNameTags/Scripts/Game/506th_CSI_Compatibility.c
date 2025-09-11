//------------------------------------------------------------------------------------------------
modded class SCR_NameTagData : Managed
{
	//------------------------------------------------------------------------------------------------
	override void GetName(out string name, out notnull array<string> nameParams)
	{
		if (!m_ClientComponent) 
			return;
		
		if (m_eType == ENameTagEntityType.PLAYER)
		{
			string roleNametagVisible = m_ClientComponent.ReturnLocalCSISettings()[7];
			string rankVisible = m_ClientComponent.ReturnLocalCSISettings()[5];

			PlayerManager playerMgr = GetGame().GetPlayerManager();
			if (playerMgr)
			{
				CustomNamesManager customNamesMgr = CustomNamesManager.GetInstance();
				if (customNamesMgr)
				{
					string customName = customNamesMgr.GetCustomName(m_iPlayerID);
					if (!customName.IsEmpty())
					{	
						m_sName = customName;
					}
					else
					{
						m_sName = playerMgr.GetPlayerName(m_iPlayerID);
					}
				}
				else
				{
					m_sName = playerMgr.GetPlayerName(m_iPlayerID);
				}

				if (rankVisible == "true")
				{
					string rank = m_AuthorityComponent.ReturnLocalPlayerMapValue(-1, m_iPlayerID, "PR"); // PR = PlayerRank
					
					if (!rank.IsEmpty()) 
						m_sName = string.Format("%1 %2", rank, m_sName);
				}
				
				if (roleNametagVisible == "true")
				{
					string icon = m_AuthorityComponent.ReturnLocalPlayerMapValue(m_iGroupID, m_iPlayerID, "DI"); // DI = DisplayIcon
					
					if (icon != "MAN" && !icon.IsEmpty())
						m_sName = string.Format("%1 [%2]", m_sName, icon);
				}
			}
			else
			{ 
				m_sName = "No player manager!";
			}
		}
		else if (m_eType == ENameTagEntityType.AI)
		{
			SCR_CharacterIdentityComponent scrCharIdentity = SCR_CharacterIdentityComponent.Cast(m_Entity.FindComponent(SCR_CharacterIdentityComponent));
			if (scrCharIdentity)
			{
				scrCharIdentity.GetFormattedFullName(m_sName, m_aNameParams);
			}
			else
			{
				CharacterIdentityComponent charIdentity = CharacterIdentityComponent.Cast(m_Entity.FindComponent(CharacterIdentityComponent));
				if (charIdentity && charIdentity.GetIdentity())
					m_sName = charIdentity.GetIdentity().GetName();
				else
					m_sName = "No character identity!";
			}
		}

		name = m_sName;
		nameParams.Copy(m_aNameParams);
	}
}

//------------------------------------------------------------------------------------------------
modded class CSI_GroupDisplay : SCR_InfoDisplay
{
	//------------------------------------------------------------------------------------------------
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);

		if (!m_AuthorityComponent || !m_ClientComponent || !m_GroupsManagerComponent) 
		{
			m_AuthorityComponent = CSI_AuthorityComponent.GetInstance();
			m_ClientComponent = CSI_ClientComponent.GetInstance();
			m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
			return;
		}
		
		string groupDisplayVisible = m_ClientComponent.ReturnLocalCSISettings()[2];
		string rankVisible = m_ClientComponent.ReturnLocalCSISettings()[5];
		string hudAutoHidden = m_ClientComponent.ReturnLocalCSISettings()[14];
		
		array<string> groupArray = m_ClientComponent.GetLocalGroupArray();
		
		SCR_AIGroup playersGroup = m_GroupsManagerComponent.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());

		if ((groupDisplayVisible == "false" || (hudAutoHidden == "true" && !hudToggled)) || !groupArray || groupArray.Count() <= 1 || !playersGroup) 
		{
			ClearGroupDisplay(0, true);
			return;
		}

		foreach (int i, string playerStringToSplit : groupArray)
		{
			array<string> playerSplitArray = {};
			playerStringToSplit.Split(":", playerSplitArray, false);

			int playerID = playerSplitArray[1].ToInt();
			string colorTeamString = m_AuthorityComponent.ReturnLocalPlayerMapValue(playersGroup.GetGroupID(), playerID, "CT"); // CT = ColorTeam
			string iconString = m_AuthorityComponent.ReturnLocalPlayerMapValue(playersGroup.GetGroupID(), playerID, "DI"); // DI = DisplayIcon
			
			string playerName;
			CustomNamesManager customNamesMgr = CustomNamesManager.GetInstance();
			if (customNamesMgr)
			{
				string customName = customNamesMgr.GetCustomName(playerID);
				if (!customName.IsEmpty())
				{	
					playerName = customName;
				}
				else
				{
					playerName = GetGame().GetPlayerManager().GetPlayerName(playerID);
				}
			}
			else
			{
				playerName = GetGame().GetPlayerManager().GetPlayerName(playerID);
			}
			
			if (playerName.IsEmpty() || iconString.IsEmpty()) 
				return;

			if (rankVisible == "true") 
			{
				string rank = m_AuthorityComponent.ReturnLocalPlayerMapValue(-1, playerID, "PR"); // PR = PlayerRank
				if (!rank.IsEmpty()) 
					playerName = string.Format("%1 %2", rank, playerName);
			}

			TextWidget playerDisplay = TextWidget.Cast(m_wRoot.FindAnyWidget(string.Format("Player%1", i)));
			ImageWidget statusDisplay = ImageWidget.Cast(m_wRoot.FindAnyWidget(string.Format("Status%1", i)));
			
			if (!playerDisplay || !statusDisplay) 
				continue;

			playerName = CheckEllipsis(106, playerName);

			playerDisplay.SetText(playerName);
			playerDisplay.SetColorInt(m_ClientComponent.SwitchStringToColorTeam(colorTeamString));
			
			if (iconString == "DRV" || iconString == "PAX") 
			{
				FrameSlot.SetSize(statusDisplay, 17, 17);	
				switch (true)
				{
					case (i >= 0 && i <= 4)   : {FrameSlot.SetPosX(statusDisplay, 88.6);  break;}
					case (i >= 5 && i <= 9)   : {FrameSlot.SetPosX(statusDisplay, 218.8); break;}
					case (i >= 10 && i <= 14) : {FrameSlot.SetPosX(statusDisplay, 349.0); break;}
					case (i >= 15 && i <= 19) : {FrameSlot.SetPosX(statusDisplay, 478.6); break;}
					case (i >= 20 && i <= 24) : {FrameSlot.SetPosX(statusDisplay, 608.6); break;}
				}
			}
			else
			{
				FrameSlot.SetSize(statusDisplay, 23.2, 23.2);
				switch (true)
				{
					case (i >= 0 && i <= 4)   : {FrameSlot.SetPosX(statusDisplay, 85.6125);  break;}
					case (i >= 5 && i <= 9)   : {FrameSlot.SetPosX(statusDisplay, 216.0125); break;}
					case (i >= 10 && i <= 14) : {FrameSlot.SetPosX(statusDisplay, 346.0125); break;}
					case (i >= 15 && i <= 19) : {FrameSlot.SetPosX(statusDisplay, 475.6125); break;}
					case (i >= 20 && i <= 24) : {FrameSlot.SetPosX(statusDisplay, 605.6125); break;}
				}
			}
			
			statusDisplay.SetOpacity(1);
			statusDisplay.LoadImageTexture(0, m_ClientComponent.SwitchStringToIcon(iconString));
			statusDisplay.SetColorInt(m_ClientComponent.SwitchStringToColorTeam(colorTeamString));
		}
		
		ClearGroupDisplay(groupArray.Count(), true);
	}
}

//------------------------------------------------------------------------------------------------
modded class CSI_PlayerSelectionDialog : ChimeraMenuBase
{
	//------------------------------------------------------------------------------------------------
	override protected void UpdatePlayerList()
	{
		string rankVisible = m_ClientComponent.ReturnLocalCSISettings()[5];

		m_aGroupArray = m_ClientComponent.GetLocalGroupArray();

		if (m_aGroupArray.Count() <= 0) 
		{
			OnMenuBack(); 
			return; 
		}

		foreach (int i, string playerStringToSplit : m_aGroupArray) 
		{
			array<string> playerSplitArray = {};
			playerStringToSplit.Split(":", playerSplitArray, false);

			int playerID = playerSplitArray[1].ToInt();
			string colorTeamString = m_AuthorityComponent.ReturnLocalPlayerMapValue(m_PlayersGroup.GetGroupID(), playerID, "CT");
			string iconString = m_AuthorityComponent.ReturnLocalPlayerMapValue(m_PlayersGroup.GetGroupID(), playerID, "SSI");
			
			string playerName;
			CustomNamesManager customNamesMgr = CustomNamesManager.GetInstance();
			if (customNamesMgr)
			{
				string customName = customNamesMgr.GetCustomName(playerID);
				if (!customName.IsEmpty())
				{
					playerName = customName;
				}
				else
				{
					playerName = GetGame().GetPlayerManager().GetPlayerName(playerID);
				}
			}
			else
			{
				playerName = GetGame().GetPlayerManager().GetPlayerName(playerID);
			}

			TextWidget playerDisplay = TextWidget.Cast(m_wRoot.FindAnyWidget(string.Format("Player%1", i)));
			ImageWidget statusDisplay = ImageWidget.Cast(m_wRoot.FindAnyWidget(string.Format("Status%1", i)));

			if (!playerDisplay || !statusDisplay) continue;

			if (rankVisible == "true") 
			{
				string rank = m_AuthorityComponent.ReturnLocalPlayerMapValue(-1, playerID, "PR");
				if (!rank.IsEmpty()) 
					playerName = string.Format("%1 %2", rank, playerName);
			}

			playerName = CheckEllipsis(190, playerName);

			playerDisplay.SetText(playerName);
			playerDisplay.SetColorInt(m_ClientComponent.SwitchStringToColorTeam(colorTeamString));			
			statusDisplay.SetOpacity(1);
			statusDisplay.LoadImageTexture(0, m_ClientComponent.SwitchStringToIcon(iconString));
			statusDisplay.SetColorInt(m_ClientComponent.SwitchStringToColorTeam(colorTeamString));
		}
		
		for (int e = m_aGroupArray.Count(); e <= 24; e++)
		{
			TextWidget playerRemoveDisplay = TextWidget.Cast(m_wRoot.FindAnyWidget(string.Format("Player%1", e)));
			ImageWidget statusRemoveDisplay = ImageWidget.Cast(m_wRoot.FindAnyWidget(string.Format("Status%1", e)));

			if (!playerRemoveDisplay || !statusRemoveDisplay) continue;

			playerRemoveDisplay.SetText("");
			statusRemoveDisplay.SetOpacity(0);
		}
	}
}

//------------------------------------------------------------------------------------------------
modded class CSI_PlayerSettingsDialog : ChimeraMenuBase
{
	//------------------------------------------------------------------------------------------------
	override protected void UpdatePlayerIcon()
	{
		string playerName;
		CustomNamesManager customNamesMgr = CustomNamesManager.GetInstance();
		if (customNamesMgr)
		{
			string customName = customNamesMgr.GetCustomName(m_iSelectedPlayerID);
			if (!customName.IsEmpty())
			{
				playerName = customName;
			}
			else
			{
				playerName = GetGame().GetPlayerManager().GetPlayerName(m_iSelectedPlayerID);
			}
		}
		else
		{
			playerName = GetGame().GetPlayerManager().GetPlayerName(m_iSelectedPlayerID);
		}
		
		string colorTeamString = m_AuthorityComponent.ReturnLocalPlayerMapValue(m_iGroupID, m_iSelectedPlayerID, "CT");
		m_sStoredSpecialtyIcon = m_AuthorityComponent.ReturnLocalPlayerMapValue(m_iGroupID, m_iSelectedPlayerID, "SSI");
	
		if (m_sStoredSpecialtyIcon.IsEmpty()) 
			return;

		string rankVisible = m_ClientComponent.ReturnLocalCSISettings()[5];

		if (rankVisible == "true") 
		{
			string rank = m_AuthorityComponent.ReturnLocalPlayerMapValue(-1, m_iSelectedPlayerID, "PR");
			if (!rank.IsEmpty()) 
				playerName = string.Format("%1 %2", rank, playerName);
		}

		m_wIcon.LoadImageTexture(0, m_ClientComponent.SwitchStringToIcon(m_sStoredSpecialtyIcon));
		m_wIcon.SetColorInt(m_ClientComponent.SwitchStringToColorTeam(colorTeamString));
		m_wPlayerName.SetText(playerName);
		m_wPlayerName.SetColorInt(m_ClientComponent.SwitchStringToColorTeam(colorTeamString));
	}
}