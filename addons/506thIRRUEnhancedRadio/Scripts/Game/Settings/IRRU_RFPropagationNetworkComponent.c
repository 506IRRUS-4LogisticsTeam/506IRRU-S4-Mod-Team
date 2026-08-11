//------------------------------------------------------------------------------------------------
class IRRU_RFPropagationNetworkComponentClass : SCR_BaseGameModeComponentClass
{
};

class IRRU_RFPropagationNetworkComponent : SCR_BaseGameModeComponent
{
	protected static IRRU_RFPropagationNetworkComponent s_Instance;

	[RplProp(onRplName: "OnSettingsReceived")]
	protected bool m_bRFPropagationEnabled;

	[RplProp(onRplName: "OnSettingsReceived")]
	protected bool m_bDebugEnabled;

	//! Server-only: which frequency each currently-keyed player is holding open
	protected ref map<int, int> m_mIRRU_KeyedFreqByPlayer = new map<int, int>();

	protected static const int IRRU_SQUELCH_TICK_MS = 150;
	protected bool m_bIRRU_SquelchTickerRunning = false;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;

		if (Replication.IsServer())
		{
			IRRU_RFPropagationSettings settings = IRRU_RFPropagationSettings.GetInstance();
			m_bRFPropagationEnabled = settings.IsRFPropagationEnabled();
			m_bDebugEnabled = settings.IsDebugEnabled();

			Replication.BumpMe();

			Print(string.Format("[IRRU RFPropagation] Server settings loaded - RF: %1 | Debug: %2",
				m_bRFPropagationEnabled, m_bDebugEnabled));
		}
	}

	//------------------------------------------------------------------------------------------------
	static IRRU_RFPropagationNetworkComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSettingsReceived()
	{
		Print(string.Format("[IRRU RFPropagation] Received server settings - RF: %1 | Debug: %2",
			m_bRFPropagationEnabled, m_bDebugEnabled));
	}

	//------------------------------------------------------------------------------------------------
	static bool IsRFPropagationEnabled()
	{
		if (!s_Instance)
			return false;

		return s_Instance.m_bRFPropagationEnabled;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDebugEnabled()
	{
		if (!s_Instance)
			return false;

		return s_Instance.m_bDebugEnabled;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: relay a transmitter's key state to every client so receivers can
	//! play squelch open/close instantly, dead keys included. Sender position is
	//! stamped here so receivers can range-gate without knowing the transmitter
	//! entity (which may be outside their replication relevance).
	void IRRU_RelayKeyState(int senderPlayerId, int frequency, float range, bool keyed)
	{
		if (!Replication.IsServer())
			return;

		if (keyed)
			m_mIRRU_KeyedFreqByPlayer.Set(senderPlayerId, frequency);
		else
			m_mIRRU_KeyedFreqByPlayer.Remove(senderPlayerId);

		vector senderPos = vector.Zero;
		IEntity senderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(senderPlayerId);
		if (senderEntity)
			senderPos = senderEntity.GetOrigin();

		Rpc(RpcDo_IRRU_KeyState, senderPlayerId, frequency, range, keyed, senderPos);
		RpcDo_IRRU_KeyState(senderPlayerId, frequency, range, keyed, senderPos);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_IRRU_KeyState(int senderPlayerId, int frequency, float range, bool keyed, vector senderPos)
	{
		IRRU_RadioRxSquelch.GetInstance().OnRemoteKeyState(senderPlayerId, frequency, range, keyed, senderPos);

		// Dead-key channels produce no voice packets, so the VoNComponent ticker
		// may never run; keep timeouts advancing from here as well.
		if (GetGame().GetPlayerController())
			IRRU_EnsureSquelchTicker();
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_EnsureSquelchTicker()
	{
		if (m_bIRRU_SquelchTickerRunning)
			return;

		m_bIRRU_SquelchTickerRunning = true;
		GetGame().GetCallqueue().CallLater(IRRU_SquelchTick, IRRU_SQUELCH_TICK_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_SquelchTick()
	{
		float nowMs = GetGame().GetWorld().GetWorldTime();

		if (IRRU_RadioRxSquelch.GetInstance().Tick(nowMs))
			GetGame().GetCallqueue().CallLater(IRRU_SquelchTick, IRRU_SQUELCH_TICK_MS, false);
		else
			m_bIRRU_SquelchTickerRunning = false;
	}

	//------------------------------------------------------------------------------------------------
	//! A player who disconnects mid-key never sends the key-stop RPC; release
	//! their channel for everyone.
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);

		if (!Replication.IsServer())
			return;

		int frequency;
		if (m_mIRRU_KeyedFreqByPlayer.Find(playerId, frequency))
			IRRU_RelayKeyState(playerId, frequency, 0.0, false);
	}
}
