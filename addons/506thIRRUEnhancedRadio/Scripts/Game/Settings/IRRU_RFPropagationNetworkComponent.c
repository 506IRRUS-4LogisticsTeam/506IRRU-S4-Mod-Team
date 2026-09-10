//------------------------------------------------------------------------------------------------
class IRRU_PendingKeyStop
{
	int m_iFrequency;
	float m_fQueuedAtMs;
}

//------------------------------------------------------------------------------------------------
//! Server-side record of a currently-keyed player, kept so late joiners and
//! mid-transmission tuners can be told about in-progress transmissions
//! (frequency, range and crypto fill hash all ride the key-state broadcast).
class IRRU_ServerKeyedState
{
	int m_iFrequency;
	float m_fRange;
	int m_iFillHash;
}

//------------------------------------------------------------------------------------------------
class IRRU_RFPropagationNetworkComponentClass : SCR_BaseGameModeComponentClass
{
};

class IRRU_RFPropagationNetworkComponent : SCR_BaseGameModeComponent
{
	protected static IRRU_RFPropagationNetworkComponent s_Instance;

	[RplProp()]
	protected bool m_bRFPropagationEnabled;

	[RplProp()]
	protected bool m_bDebugEnabled;

	//! Master switch for the COMSEC fill feature; server-authored from
	//! $profile:IRRU_Encryption.json and flippable at runtime by a GM via
	//! the radiocrypto chat command. Everything downstream fail-safes to
	//! plaintext (= today's behavior) while this is false.
	[RplProp(onRplName: "IRRU_OnEncryptionEnabledRpl")]
	protected bool m_bEncryptionEnabled;

	//! Server-only: state of each currently-keyed player
	protected ref map<int, ref IRRU_ServerKeyedState> m_mIRRU_KeyedByPlayer = new map<int, ref IRRU_ServerKeyedState>();

	//! Server-only: key-stops held briefly before broadcast; a re-key inside the
	//! window cancels the stop so PTT spam reads as one continuous transmission
	//! for receivers instead of a close beep per cycle (and generates no traffic).
	protected ref map<int, ref IRRU_PendingKeyStop> m_mIRRU_PendingStops = new map<int, ref IRRU_PendingKeyStop>();
	protected static const int IRRU_KEY_STOP_DEBOUNCE_MS = 300;

	protected static const int IRRU_SQUELCH_TICK_MS = 150;
	protected bool m_bIRRU_SquelchTickerRunning = false;

	protected static const int IRRU_NOTICE_RETRY_MS = 2000;
	protected static const int IRRU_NOTICE_MAX_TRIES = 30;
	protected int m_iIRRU_NoticeTries = 0;
	//! -1 = nothing announced yet this session; otherwise last announced state (0/1)
	protected int m_iIRRU_AnnouncedEncryptionState = -1;

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

			IRRU_EncryptionSettings encryptionSettings = IRRU_EncryptionSettings.GetInstance();
			m_bEncryptionEnabled = encryptionSettings.IsEncryptionEnabled();

			Replication.BumpMe();

			// SCR_BaseGameModeComponent has no OnPlayerSpawned override point;
			// the game mode's invoker carries (playerId, controlledEntity)
			SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetOwner());
			if (gameMode)
				gameMode.GetOnPlayerSpawned().Insert(IRRU_OnPlayerSpawnedHook);

			Print(string.Format("[IRRU RFPropagation] Server settings loaded - RF: %1 | Debug: %2 | Encryption: %3",
				m_bRFPropagationEnabled, m_bDebugEnabled, m_bEncryptionEnabled));
		}
	}

	//------------------------------------------------------------------------------------------------
	static IRRU_RFPropagationNetworkComponent GetInstance()
	{
		return s_Instance;
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
	//! Fail-safes to false (= plaintext, today's behavior) when the component
	//! is absent, so a missing game-mode override can never turn crypto on.
	static bool IsEncryptionEnabled()
	{
		if (!s_Instance)
			return false;

		return s_Instance.m_bEncryptionEnabled;
	}

	//------------------------------------------------------------------------------------------------
	//! Replication callback: announce enable/disable to this client once per
	//! transition. Also covers late joiners (initial replication of true).
	protected void IRRU_OnEncryptionEnabledRpl()
	{
		m_iIRRU_NoticeTries = 0;
		IRRU_TryShowEncryptionNotice();

		// Kill-switch immediacy: a stream that is currently scrambling would
		// otherwise keep JamStrength=0 until its next natural event restart.
		// Age encrypted streams so their next packet re-verdicts to plaintext.
		if (!m_bEncryptionEnabled)
		{
			PlayerController playerController = GetGame().GetPlayerController();
			if (playerController)
			{
				SCR_VoNComponent von = SCR_VoNComponent.Cast(playerController.FindComponent(SCR_VoNComponent));
				if (von)
					von.IRRU_ForceReverdictEncryptedStreams();
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Public, self-guarded announce entry: called from the radio-check path
	//! as a reliable join-time fallback (initial-replication onRplName timing
	//! is engine-defined; this path runs once the local player exists)
	void IRRU_AnnounceEncryptionState()
	{
		IRRU_TryShowEncryptionNotice();
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_TryShowEncryptionNotice()
	{
		int state = 0;
		if (m_bEncryptionEnabled)
			state = 1;

		if (m_iIRRU_AnnouncedEncryptionState == state)
			return;

		// A disable notice is only useful if the client ever saw the enable
		if (state == 0 && m_iIRRU_AnnouncedEncryptionState == -1)
		{
			m_iIRRU_AnnouncedEncryptionState = 0;
			return;
		}

		PlayerController playerController = GetGame().GetPlayerController();
		SCR_ChatComponent chatComponent;
		if (playerController)
			chatComponent = SCR_ChatComponent.Cast(playerController.FindComponent(SCR_ChatComponent));

		if (!chatComponent)
		{
			// Replication can land before the local player exists; retry briefly
			m_iIRRU_NoticeTries++;
			if (m_iIRRU_NoticeTries < IRRU_NOTICE_MAX_TRIES)
				GetGame().GetCallqueue().CallLater(IRRU_TryShowEncryptionNotice, IRRU_NOTICE_RETRY_MS, false);
			return;
		}

		m_iIRRU_AnnouncedEncryptionState = state;

		if (state == 1)
			chatComponent.ShowMessage("[COMSEC] Encryption ENABLED on this server - select a channel in the VON menu and press the crypto fill key (default LCtrl+G) to load a fill. No fill = you talk and hear plaintext as normal.");
		else
			chatComponent.ShowMessage("[COMSEC] Encryption DISABLED - all radio traffic is plaintext.");
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a GM asks to flip the master switch at runtime (the op-night
	//! kill switch). Non-GM requests are refused.
	void IRRU_RequestSetEncryption(int requestingPlayerId, bool enabled)
	{
		if (!Replication.IsServer())
			return;

		if (!IRRU_IsPlayerGM(requestingPlayerId))
		{
			Print(string.Format("[IRRU Encryption] radiocrypto request from player %1 refused - not a GM", requestingPlayerId), LogLevel.WARNING);
			return;
		}

		if (m_bEncryptionEnabled == enabled)
			return;

		m_bEncryptionEnabled = enabled;
		Replication.BumpMe();

		Print(string.Format("[IRRU Encryption] Encryption toggled to %1 by player %2", enabled, requestingPlayerId), LogLevel.WARNING);

		// onRplName does not fire on the authority; keep listen-host parity.
		// On a dedicated server there is no local player - skip, or the
		// notice retry chain spins uselessly for a minute per toggle.
		if (GetGame().GetPlayerController())
			IRRU_OnEncryptionEnabledRpl();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IRRU_IsPlayerGM(int playerId)
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return false;

		SCR_EditorManagerEntity manager = core.GetEditorManager(playerId);
		if (!manager)
			return false;

		return !manager.IsLimited();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: relay a transmitter's key state to every client so receivers can
	//! play squelch open/close instantly, dead keys included. Sender position is
	//! stamped here so receivers can range-gate without knowing the transmitter
	//! entity (which may be outside their replication relevance). fillHash rides
	//! along so receivers can verdict crypto without any extra traffic (0 =
	//! plaintext; meaningful only when keyed=true).
	void IRRU_RelayKeyState(int senderPlayerId, int frequency, float range, bool keyed, int fillHash)
	{
		if (!Replication.IsServer())
			return;

		if (keyed)
		{
			IRRU_PendingKeyStop pending;
			if (m_mIRRU_PendingStops.Find(senderPlayerId, pending))
			{
				m_mIRRU_PendingStops.Remove(senderPlayerId);

				// Re-key of a DIFFERENT frequency: receivers never saw the
				// close of the old one, send it now. A same-frequency re-key
				// deliberately falls through to a fresh broadcast (instead of
				// the old early-return): the fill hash may have changed
				// between key-ups, and receivers' Open() is idempotent, so
				// the extra broadcast costs nothing and keeps hashes current.
				if (pending.m_iFrequency != frequency)
					IRRU_BroadcastKeyState(senderPlayerId, pending.m_iFrequency, 0.0, false, 0);
			}

			IRRU_ServerKeyedState state = new IRRU_ServerKeyedState();
			state.m_iFrequency = frequency;
			state.m_fRange = range;
			state.m_iFillHash = fillHash;
			m_mIRRU_KeyedByPlayer.Set(senderPlayerId, state);

			if (IRRU_EncryptionSettings.GetInstance().IsDebugEnabled())
				Print(string.Format("[IRRU Encryption] key-up player=%1 freq=%2 hash=%3", senderPlayerId, frequency, fillHash));

			IRRU_BroadcastKeyState(senderPlayerId, frequency, range, true, fillHash);
		}
		else
		{
			IRRU_PendingKeyStop pending = new IRRU_PendingKeyStop();
			pending.m_iFrequency = frequency;
			pending.m_fQueuedAtMs = GetGame().GetWorld().GetWorldTime();
			m_mIRRU_PendingStops.Set(senderPlayerId, pending);
			GetGame().GetCallqueue().CallLater(IRRU_FlushPendingStop, IRRU_KEY_STOP_DEBOUNCE_MS, false, senderPlayerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_FlushPendingStop(int playerId)
	{
		IRRU_PendingKeyStop pending;
		if (!m_mIRRU_PendingStops.Find(playerId, pending))
			return;

		// A newer stop queued during our delay has its own flush call coming;
		// the 50ms margin absorbs CallLater frame jitter on our own entry.
		float nowMs = GetGame().GetWorld().GetWorldTime();
		if (nowMs - pending.m_fQueuedAtMs < IRRU_KEY_STOP_DEBOUNCE_MS - 50)
			return;

		m_mIRRU_PendingStops.Remove(playerId);
		m_mIRRU_KeyedByPlayer.Remove(playerId);
		IRRU_BroadcastKeyState(playerId, pending.m_iFrequency, 0.0, false, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void IRRU_BroadcastKeyState(int senderPlayerId, int frequency, float range, bool keyed, int fillHash)
	{
		vector senderPos = vector.Zero;
		IEntity senderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(senderPlayerId);
		if (senderEntity)
			senderPos = senderEntity.GetOrigin();

		Rpc(RpcDo_IRRU_KeyState, senderPlayerId, frequency, range, keyed, senderPos, fillHash);
		RpcDo_IRRU_KeyState(senderPlayerId, frequency, range, keyed, senderPos, fillHash);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_IRRU_KeyState(int senderPlayerId, int frequency, float range, bool keyed, vector senderPos, int fillHash)
	{
		IRRU_RadioRxSquelch.GetInstance().OnRemoteKeyState(senderPlayerId, frequency, range, keyed, senderPos, fillHash);

		// Dead-key channels produce no voice packets, so the VoNComponent ticker
		// may never run; keep timeouts advancing from here as well.
		if (GetGame().GetPlayerController())
			IRRU_EnsureSquelchTicker();
	}

	//------------------------------------------------------------------------------------------------
	void IRRU_EnsureSquelchTicker()
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
	//! A player spawning in while someone is mid-transmission missed that
	//! transmission's key-up broadcast (JIP, or connected after it started).
	//! Push every currently keyed player's state to THE SPAWNING CLIENT ONLY
	//! (owner-targeted RPC on their VON controller): a broadcast re-announce
	//! would reach every client on every respawn and could re-open squelch
	//! channels that legitimately closed by silence timeout elsewhere.
	//! Subscribed to SCR_BaseGameMode.GetOnPlayerSpawned() server-side only.
	protected void IRRU_OnPlayerSpawnedHook(int playerId, IEntity controlledEntity)
	{
		if (m_mIRRU_KeyedByPlayer.IsEmpty())
			return;

		PlayerController playerController = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!playerController)
			return;

		SCR_VONController vonController = SCR_VONController.Cast(playerController.FindComponent(SCR_VONController));
		if (!vonController)
			return;

		foreach (int keyedPlayerId, IRRU_ServerKeyedState state : m_mIRRU_KeyedByPlayer)
		{
			if (keyedPlayerId == playerId)
				continue;

			vector senderPos = vector.Zero;
			IEntity senderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(keyedPlayerId);
			if (senderEntity)
				senderPos = senderEntity.GetOrigin();

			vonController.IRRU_ServerSyncKeyState(keyedPlayerId, state.m_iFrequency, state.m_fRange, senderPos, state.m_iFillHash);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A player who disconnects mid-key never sends the key-stop RPC; release
	//! their channel for everyone.
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);

		if (!Replication.IsServer())
			return;

		IRRU_ServerKeyedState state;
		if (m_mIRRU_KeyedByPlayer.Find(playerId, state))
			IRRU_RelayKeyState(playerId, state.m_iFrequency, 0.0, false, 0);
	}
}
