//------------------------------------------------------------------------------------------------
// Disable GM Budgets
//------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------
// GameMode Budget Toggle
//------------------------------------------------------------------------------------------------

modded class SCR_BaseGameMode
{
	[RplProp(onRplName: "DisableGMBudget_OnBroadcastValueUpdated")]
	protected bool m_DisableGMBudgets_BudgetsEnabled = true;

	static bool s_bDisableGMBudgets_BudgetsEnabled = true;

	static ref set<EEditableEntityBudget> s_DisableGMBudgets_Budgets;

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!s_DisableGMBudgets_Budgets)
			s_DisableGMBudgets_Budgets = new set<EEditableEntityBudget>();

		s_DisableGMBudgets_Budgets.Clear();

		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.PROPS);
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.AI);
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.VEHICLES);
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.WAYPOINTS);
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.SYSTEMS);

		s_bDisableGMBudgets_BudgetsEnabled = m_DisableGMBudgets_BudgetsEnabled;

		PrintFormat(
			"[DisableGMBudgets] GameMode initialized. Budgets enabled: %1",
			m_DisableGMBudgets_BudgetsEnabled
		);
	}

	//------------------------------------------------------------------------------------------------
	void DisableGMBudget_SetBudgetsEnabled(bool enabled)
	{
		m_DisableGMBudgets_BudgetsEnabled = enabled;
		s_bDisableGMBudgets_BudgetsEnabled = enabled;

		Replication.BumpMe();

		DisableGMBudget_OnBroadcastValueUpdated();

		PrintFormat(
			"[DisableGMBudgets] Budget state changed. Budgets enabled: %1",
			enabled
		);
	}

	//------------------------------------------------------------------------------------------------
	bool DisableGMBudget_AreBudgetsEnabled()
	{
		return m_DisableGMBudgets_BudgetsEnabled;
	}

	//------------------------------------------------------------------------------------------------
	void DisableGMBudget_OnBroadcastValueUpdated()
	{
		s_bDisableGMBudgets_BudgetsEnabled = m_DisableGMBudgets_BudgetsEnabled;

		PrintFormat(
			"[DisableGMBudgets] Replicated budget state updated. Budgets enabled: %1",
			m_DisableGMBudgets_BudgetsEnabled
		);

		SCR_BudgetEditorComponent budgetManager =
			SCR_BudgetEditorComponent.Cast(
				SCR_BudgetEditorComponent.GetInstance(
					SCR_BudgetEditorComponent,
					false,
					true
				)
			);

		if (!budgetManager)
			return;

		budgetManager.DisableGMBudget_BudgetsUpdated(
			m_DisableGMBudgets_BudgetsEnabled
		);
	}
}


//------------------------------------------------------------------------------------------------
// Editor Attribute Support
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DisableGMBudget_BudgetsEnabledAttribute : SCR_BaseEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(
		Managed item,
		SCR_AttributesManagerEditorComponent manager
	)
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(item);

		if (!gamemode)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(
			gamemode.DisableGMBudget_AreBudgetsEnabled()
		);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(
		Managed item,
		SCR_BaseEditorAttributeVar var,
		SCR_AttributesManagerEditorComponent manager,
		int playerID
	)
	{
		if (!var)
			return;

		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(item);

		if (!gamemode)
			return;

		gamemode.DisableGMBudget_SetBudgetsEnabled(
			var.GetBool()
		);
	}
}


//------------------------------------------------------------------------------------------------
// Budget Editor Component
//------------------------------------------------------------------------------------------------

modded class SCR_BudgetEditorComponent
{
	protected ref map<EEditableEntityBudget, int> m_DisableGMBudget_OriginalMaxBudgets =
		new map<EEditableEntityBudget, int>();

	//------------------------------------------------------------------------------------------------
	override void EOnEditorInit()
	{
		super.EOnEditorInit();

		m_DisableGMBudget_OriginalMaxBudgets.Clear();

		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			m_DisableGMBudget_OriginalMaxBudgets.Set(
				maxBudget.GetBudgetType(),
				maxBudget.GetBudgetValue()
			);

			PrintFormat(
				"[DisableGMBudgets] Saved original budget. Type: %1 Value: %2",
				maxBudget.GetBudgetType(),
				maxBudget.GetBudgetValue()
			);
		}

		SCR_BaseGameMode game =
			SCR_BaseGameMode.Cast(
				GetGame().GetGameMode()
			);

		if (!game)
			return;

		DisableGMBudget_BudgetsUpdated(
			game.DisableGMBudget_AreBudgetsEnabled()
		);
	}

	//------------------------------------------------------------------------------------------------
	void DisableGMBudget_BudgetsUpdated(bool enabled)
	{
		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			EEditableEntityBudget budgetType =
				maxBudget.GetBudgetType();

			if (!SCR_BaseGameMode.s_DisableGMBudgets_Budgets.Contains(budgetType))
				continue;

			if (!m_DisableGMBudget_OriginalMaxBudgets.Contains(budgetType))
				continue;

			int originalBudget =
				m_DisableGMBudget_OriginalMaxBudgets.Get(
					budgetType
				);

			if (enabled)
			{
				maxBudget.SetBudgetValue(
					originalBudget
				);

				PrintFormat(
					"[DisableGMBudgets] Restored budget. Type: %1 Value: %2",
					budgetType,
					originalBudget
				);
			}
			else
			{
				maxBudget.SetBudgetValue(
					originalBudget
				);

				PrintFormat(
					"[DisableGMBudgets] Budget enforcement disabled. Type: %1 Display value remains: %2",
					budgetType,
					originalBudget
				);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected bool IsBudgetCapEnabled()
	{
		if (this.ClassName() == "SCR_CampaignBuildingBudgetEditorComponent")
			return super.IsBudgetCapEnabled();

		if (!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled)
		{
			Print(
				"[DisableGMBudgets] IsBudgetCapEnabled -> FALSE"
			);

			return false;
		}

		return super.IsBudgetCapEnabled();
	}
}


//------------------------------------------------------------------------------------------------
// Placing Editor Component
//------------------------------------------------------------------------------------------------

modded class SCR_PlacingEditorComponent
{
	//------------------------------------------------------------------------------------------------
	override bool IsThereEnoughBudgetToSpawn(
		IEntityComponentSource entitySource
	)
	{
		if (!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled)
		{
			Print(
				"[DisableGMBudgets] IsThereEnoughBudgetToSpawn -> TRUE"
			);

			return true;
		}

		return super.IsThereEnoughBudgetToSpawn(
			entitySource
		);
	}

	//------------------------------------------------------------------------------------------------
	override void CheckBudgetOwner()
	{
		if (!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled)
		{
			Print(
				"[DisableGMBudgets] CheckBudgetOwner bypassed"
			);

			return;
		}

		super.CheckBudgetOwner();
	}

	//------------------------------------------------------------------------------------------------
	override void OnBudgetMaxReached(
		EEditableEntityBudget entityBudget,
		bool maxReached
	)
	{
		if (
			!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled &&
			SCR_BaseGameMode.s_DisableGMBudgets_Budgets.Contains(
				entityBudget
			)
		)
		{
			PrintFormat(
				"[DisableGMBudgets] OnBudgetMaxReached ignored. Type: %1 MaxReached: %2",
				entityBudget,
				maxReached
			);

			return;
		}

		super.OnBudgetMaxReached(
			entityBudget,
			maxReached
		);
	}
}