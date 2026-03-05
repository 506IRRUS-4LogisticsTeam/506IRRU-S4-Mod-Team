//! Settings configuration for pneumothorax system

[BaseContainerProps(configRoot: true)]
class IRRU_PneumothoraxSettings
{
	protected const float DEFAULT_TRIGGER_CHANCE = 0.20;
	protected const float DEFAULT_PROGRESSION_TIME = 90.0;
	protected const float DEFAULT_RESILIENCE_DRAIN_RATE = 2.0;
	protected const float DEFAULT_STAMINA_DRAIN_STAGE1 = 0.01;
	protected const float DEFAULT_STAMINA_DRAIN_STAGE2 = 0.03;

	[Attribute(defvalue: "0.20", desc: "Chance (0-1) of pneumothorax on chest damage", category: "Pneumothorax",
		uiwidget: UIWidgets.Slider, params: "0 1 0.01")]
	float m_fTriggerChance;

	[Attribute(defvalue: "90", desc: "Seconds before Stage 1 progresses to Stage 2", category: "Pneumothorax",
		uiwidget: UIWidgets.Slider, params: "60 120 1")]
	float m_fProgressionTime;

	[Attribute(defvalue: "2.0", desc: "Resilience HP drained per second in Stage 2 (Tension)", category: "Pneumothorax",
		uiwidget: UIWidgets.Slider, params: "0.5 10 0.1")]
	float m_fResilienceDrainRate;

	[Attribute(defvalue: "0.01", desc: "Stamina drained per second in Stage 1 (0-1 scale)", category: "Pneumothorax",
		uiwidget: UIWidgets.Slider, params: "0 0.1 0.001")]
	float m_fStaminaDrainStage1;

	[Attribute(defvalue: "0.03", desc: "Stamina drained per second in Stage 2 (0-1 scale)", category: "Pneumothorax",
		uiwidget: UIWidgets.Slider, params: "0 0.1 0.001")]
	float m_fStaminaDrainStage2;

	[Attribute(defvalue: "1", desc: "Enable verbose debug output to the RPT log", category: "Pneumothorax",
		uiwidget: UIWidgets.CheckBox)]
	bool m_bDebugEnabled;

	protected static ref IRRU_PneumothoraxSettings s_Instance;
	protected static bool s_bInitialized = false;

	//------------------------------------------------------------------------------------------------
	void IRRU_PneumothoraxSettings()
	{
		if (m_fTriggerChance <= 0)
			m_fTriggerChance = DEFAULT_TRIGGER_CHANCE;
		if (m_fProgressionTime <= 0)
			m_fProgressionTime = DEFAULT_PROGRESSION_TIME;
		if (m_fResilienceDrainRate <= 0)
			m_fResilienceDrainRate = DEFAULT_RESILIENCE_DRAIN_RATE;
	}

	//------------------------------------------------------------------------------------------------
	static IRRU_PneumothoraxSettings GetInstance()
	{
		if (!s_Instance)
		{
			Resource holder = BaseContainerTools.LoadContainer("{3853A4CFFC42A99E}Configs/IRRU_PneumothoraxSettings.conf");
			if (holder && holder.GetResource())
			{
				BaseContainer container = holder.GetResource().ToBaseContainer();
				if (container)
				{
					s_Instance = IRRU_PneumothoraxSettings.Cast(
						BaseContainerTools.CreateInstanceFromContainer(container));
				}
			}

			if (!s_Instance)
			{
				if (!s_bInitialized)
					Print("[Pneumothorax] Config not found, using defaults", LogLevel.WARNING);

				s_Instance = new IRRU_PneumothoraxSettings();
				s_Instance.m_fTriggerChance = DEFAULT_TRIGGER_CHANCE;
				s_Instance.m_fProgressionTime = DEFAULT_PROGRESSION_TIME;
				s_Instance.m_fResilienceDrainRate = DEFAULT_RESILIENCE_DRAIN_RATE;
				s_Instance.m_fStaminaDrainStage1 = DEFAULT_STAMINA_DRAIN_STAGE1;
				s_Instance.m_fStaminaDrainStage2 = DEFAULT_STAMINA_DRAIN_STAGE2;
				s_Instance.m_bDebugEnabled = true;
			}

			if (!s_bInitialized)
			{
				s_bInitialized = true;
				Print(string.Format("[Pneumothorax] Settings initialized - Chance: %1%%, Progression: %2s, ResDrain: %3/s",
					s_Instance.m_fTriggerChance * 100,
					s_Instance.m_fProgressionTime,
					s_Instance.m_fResilienceDrainRate));
			}
		}

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDebugEnabled()
	{
		IRRU_PneumothoraxSettings settings = GetInstance();
		if (settings)
			return settings.m_bDebugEnabled;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static float GetTriggerChance()
	{
		IRRU_PneumothoraxSettings settings = GetInstance();
		if (settings)
			return Math.Clamp(settings.m_fTriggerChance, 0.0, 1.0);
		return DEFAULT_TRIGGER_CHANCE;
	}

	//------------------------------------------------------------------------------------------------
	static float GetProgressionTime()
	{
		IRRU_PneumothoraxSettings settings = GetInstance();
		if (settings)
			return Math.Clamp(settings.m_fProgressionTime, 30.0, 300.0);
		return DEFAULT_PROGRESSION_TIME;
	}

	//------------------------------------------------------------------------------------------------
	static float GetResilienceDrainRate()
	{
		IRRU_PneumothoraxSettings settings = GetInstance();
		if (settings)
			return settings.m_fResilienceDrainRate;
		return DEFAULT_RESILIENCE_DRAIN_RATE;
	}

	//------------------------------------------------------------------------------------------------
	static float GetStaminaDrainStage1()
	{
		IRRU_PneumothoraxSettings settings = GetInstance();
		if (settings)
			return settings.m_fStaminaDrainStage1;
		return DEFAULT_STAMINA_DRAIN_STAGE1;
	}

	//------------------------------------------------------------------------------------------------
	static float GetStaminaDrainStage2()
	{
		IRRU_PneumothoraxSettings settings = GetInstance();
		if (settings)
			return settings.m_fStaminaDrainStage2;
		return DEFAULT_STAMINA_DRAIN_STAGE2;
	}
}
