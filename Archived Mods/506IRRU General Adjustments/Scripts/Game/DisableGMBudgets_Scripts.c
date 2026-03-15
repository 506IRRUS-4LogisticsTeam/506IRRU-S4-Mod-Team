
modded class SCR_BaseGameMode {
	[RplProp(onRplName: "DisableGMBudget_OnBroadcastValueUpdated")]
	protected bool m_DisableGMBudgets_BudgetsEnabled = true;
	
	static bool s_bDisableGMBudgets_BudgetsEnabled = true;
	
	static ref set<EEditableEntityBudget> s_DisableGMBudgets_Budgets;
	
	override void EOnInit(IEntity owner) {
		super.EOnInit(owner);
		
		s_DisableGMBudgets_Budgets = new set<EEditableEntityBudget>;
		
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.PROPS);
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.AI);
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.VEHICLES);
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.WAYPOINTS);
		s_DisableGMBudgets_Budgets.Insert(EEditableEntityBudget.SYSTEMS);
	};
	
	void DisableGMBudget_SetBudgetsEnabled(bool enabled) {
		m_DisableGMBudgets_BudgetsEnabled = enabled;
		Replication.BumpMe();
		
		DisableGMBudget_OnBroadcastValueUpdated();
	};
	
	bool DisableGMBudget_AreBudgetsEnabled() {
		return m_DisableGMBudgets_BudgetsEnabled;
	};
	
	void DisableGMBudget_OnBroadcastValueUpdated() {
		s_bDisableGMBudgets_BudgetsEnabled = m_DisableGMBudgets_BudgetsEnabled;
		
		SCR_BudgetEditorComponent budgetManager = SCR_BudgetEditorComponent.Cast(SCR_BudgetEditorComponent.GetInstance(SCR_BudgetEditorComponent, false, true));
		if (!budgetManager)
			return;

		// budgetManager.DisableGMBudget_BudgetsUpdated(m_DisableGMBudgets_BudgetsEnabled);
	};
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DisableGMBudget_BudgetsEnabledAttribute : SCR_BaseEditorAttribute
{	
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		//If opened in global attributes
		
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(item);
		if (!gamemode)
			return null;
		
		return SCR_BaseEditorAttributeVar.CreateBool(gamemode.DisableGMBudget_AreBudgetsEnabled());
	}
	
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) 
			return;
		
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(item);
		if (!gamemode)
			return;
		
		gamemode.DisableGMBudget_SetBudgetsEnabled(var.GetBool());
	}
};

modded class SCR_BudgetEditorComponent {
	protected ref map<EEditableEntityBudget, int> m_DisableGMBudget_OriginalMaxBudgets = new map<EEditableEntityBudget, int>();
	
	override void EOnEditorInit() {
		super.EOnEditorInit();
		
		foreach	(SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			m_DisableGMBudget_OriginalMaxBudgets.Set(maxBudget.GetBudgetType(), maxBudget.GetBudgetValue());
		};
		
		SCR_BaseGameMode game = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!game || game.DisableGMBudget_AreBudgetsEnabled())
			return;
			
		DisableGMBudget_BudgetsUpdated(false);
	};

	void DisableGMBudget_BudgetsUpdated(bool enabled) {
		foreach	(SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			if (enabled)
				maxBudget.SetBudgetValue(m_DisableGMBudget_OriginalMaxBudgets.Get(maxBudget.GetBudgetType()));
			else
				maxBudget.SetBudgetValue(m_DisableGMBudget_OriginalMaxBudgets.Get(maxBudget.GetBudgetType()) * 500);
		};
	};
	
	override protected bool IsBudgetCapEnabled()
	{
		if (this.ClassName() == "SCR_CampaignBuildingBudgetEditorComponent")
			return super.IsBudgetCapEnabled();
		
		if (!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled)
			return false;

		return super.IsBudgetCapEnabled();
	}
	
//	override bool IsBudgetCapEnabled()
//	{
//		if (!m_game) {
//			m_game = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
//			if (!m_game)
//				return super.IsBudgetCapEnabled();
//		};
//		
//		return m_game.DisableGMBudget_AreBudgetsEnabled();
//	}
};

modded class SCR_PlacingEditorComponent {
	override bool IsThereEnoughBudgetToSpawn(IEntityComponentSource entitySource)
	{
		if (!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled)
			return true;
		
		return super.IsThereEnoughBudgetToSpawn(entitySource);
	}
	
//	override protected bool CanPlaceEntityServer(IEntityComponentSource editableEntitySource, out EEditableEntityBudget blockingBudget, bool updatePreview, bool showNotification)
//	{
//		if (!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled)
//			return true;
//
//		return super.CanPlaceEntityServer(editableEntitySource, blockingBudget, updatePreview, showNotification);
//	}
//	
    override void CheckBudgetOwner()
    {
		if (!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled)
			return;
		
		super.CheckBudgetOwner();
    }

    override void OnBudgetMaxReached(EEditableEntityBudget entityBudget, bool maxReached)
    {
		if (SCR_BaseGameMode.s_DisableGMBudgets_Budgets.Contains(entityBudget) && !SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled)
			return;
		
		super.OnBudgetMaxReached(entityBudget, maxReached);
    }
}

//modded class SCR_ContentBrowserEditorComponent {
//	override bool CanPlace(int prefabID, out notnull array<ref SCR_EntityBudgetValue> budgetCosts, out SCR_UIInfo blockingBudgetInfo, bool showNotification = false)
//	{
//		bool output = super.CanPlace(prefabID, budgetCosts, blockingBudgetInfo, showNotification);
//
//		if (!SCR_BaseGameMode.s_bDisableGMBudgets_BudgetsEnabled) {
//			foreach (SCR_EntityBudgetValue budget : budgetCosts) {
//				if (!SCR_BaseGameMode.s_DisableGMBudgets_Budgets.Contains(budget.GetBudgetValue()))
//					return true;
//			}
//			
//			return false;
//		}
//		
//		return output;
//	}
//}