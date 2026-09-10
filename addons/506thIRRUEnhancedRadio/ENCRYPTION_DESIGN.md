# Enhanced Radio — Encryption (COMSEC Fills) Design

Status: **DESIGN ONLY** — nothing in this document is implemented.
Version: 1.1, 2026-08-23. v1.0 was subjected to a 4-lens adversarial review
(35 findings: 2 blockers, 11 majors); every accepted finding is folded in below.
Based on full code exploration of the mod, the engine script API, and the
delivery-layer reverse engineering documented in the restricted HQ VoN analysis
post (claims sourced only from that RE are marked *[RE]*).

---

## 1. Summary

Per-channel crypto fills for the Enhanced Radio mod. A player selects a channel
in the VON radial, presses the fill key, and types a short numeric **fill**
(e.g. `738291`) — the same muscle memory as frequency entry. Traffic on a net
is intelligible only to receivers holding the same fill; everyone else hears
radio-colored digital noise for the duration of the transmission. The whole
feature sits behind a **server-profile master switch, OFF by default**, with a
**runtime admin kill switch** — with the switch off, the mod behaves
byte-for-byte as it does today and no encryption UI exists.

Decisions made with the project owner up front:

| Decision | Choice |
|---|---|
| Fill model | Typed numeric code per channel (reuses frequency-input UX) |
| Wrong key experience | Scrambled noise (not silence) |
| Key control | Anyone can set the fill on their own radio; custody is unit SOP |

Cardinal rule the whole design serves: **plaintext is intelligible to
everyone, and every ambiguity fails open (intelligible)** — with one deliberate
exception (§3.4, same-frame collisions) where failing open would leak encrypted
audio.

## 2. Why NOT the engine's encryption key

Enfusion has first-class radio encryption: `BaseRadioComponent.SetEncryptionKey
(string)` / `GetEncryptionKey()`, and every 506th radio prefab already carries
the static key `"chickenNuggets"` (matching `m_sFactionRadioEncryptionKey` in
all six 506th faction configs — five mission-template factions plus
`506IRRUFactions/506IRRU.conf`). Investigated and **rejected** as the
mechanism for dynamic fills:

1. **Wrong granularity.** The key is one string per *radio entity*. The MPU5
   carries four transceivers; per-channel fills are impossible at engine level.
2. **Wrong failure mode.** *[RE]* On key mismatch the engine never delivers the
   voice — no `OnReceive`, no event, no way for script to detect the rejected
   traffic. That forces the "total silence" UX we rejected and makes
   scrambled-noise theater impossible.
3. **It would break comms entirely.** Since 1.8, radio→radio voice delivery
   requires a RelayTransceiver hop *[RE]*, and relay/coverage networks are
   partitioned by exact key string (`SCR_RadioCoverageSystem` buckets sources
   in a `map<string, SCR_CoverageRadioComponent>` — verified API). Dynamically
   changing a radio's key would cut it off from every relay tower whose key
   didn't follow.
4. Runtime behavior of `SetEncryptionKey` (authority, replication, effect on
   in-flight streams) is undocumented and unverified.

**The engine key stays exactly as it is: a static, server-wide constant.** Our
encryption is an audio-presentation layer on top of unchanged delivery.

## 3. Architecture

### 3.1 The one-sentence version

The sender's key-up RPC (which already exists) carries a **hash of the
sender's fill**; every client stores it in a dedicated, ungated sender-hash
table; each receiver compares it against its own fill for that frequency when
a voice stream opens, and on mismatch writes `JamStrength = 0` into the audio
graph — which provably mutes the voice bus completely and raises the
synthesized jamming noise to full. No new audio assets, no audio-project
changes, no change to voice delivery.

### 3.2 Fill state (client-local, persisted)

New map on `SCR_IRRURadioEarSettings` (`506th_EarRouting.c`):

```
protected ref map<int, string> m_mFillByFrequency;   // kHz -> fill string
```

- **Keyed by frequency (kHz int), NOT by transceiver object.** The
  transceiver-object-keyed maps suffer respawn amnesia; frequency-keyed fills
  survive respawn and radio swaps, match the RPC/squelch layer's channel
  identity, and read as "the net has a fill."
- No entry / empty string = **PLAINTEXT**.
- A fill is a 1–8 digit numeric string. Hash: deterministic string hash to
  int (djb2-style); `0` is reserved to mean plaintext (a real hash of 0 is
  remapped to 1). Collisions at unit scale are negligible and fail open.
- Accessors: `GetFill / SetFill / ClearFill / GetFillHash` (0 = plaintext).
- **Fills persist across sessions in the MVP** (reviewer-promoted from Phase
  B): saved per-client to `$profile:IRRU_EnhancedRadio_settings.json` using
  the existing `IRRU_RadioUserSettings` pattern. Rationale: crash-relogs are
  routine on op nights, and a relog that silently drops fills turns into an
  unnoticed COMSEC break — the RTO transmits plaintext believing they're
  secure (fail-open is only safe when the state is visible; see §4.2).

### 3.3 Transport: sender-hash table decoupled from squelch

Current key-state route (all existing code):

```
506th_VONController.IRRU_NotifyKeyStart/Stop        (client key-up/release)
  -> RpcAsk_IRRU_KeyState(freq, range, keyed)        [Server, PlayerController]
  -> IRRU_RFPropagationNetworkComponent.IRRU_RelayKeyState  (relay + debounce)
  -> IRRU_BroadcastKeyState (+senderPos)             [Broadcast]
  -> IRRU_RadioRxSquelch.OnRemoteKeyState            (every client)
```

Changes:

1. **Append `int fillHash`** to `RpcAsk_IRRU_KeyState` and
   `RpcDo_IRRU_KeyState`. Sender computes it at key-up from the frequency of
   the transceiver passed to `IRRU_NotifyKeyStart` (this makes alternate-
   channel PTT correct for free: the implicit stop(primary)+start(alternate)
   sequence carries the alternate net's hash). **Receivers ignore the hash on
   `keyed=false` RPCs** — stops clean state, never write it.
2. **Dedicated hash table, NOT inside the squelch channel state** (fixes
   review BLOCKER-2). `IRRU_RadioRxSquelch` (or a small sibling class) keeps:

   ```
   ref map<int, ref IRRU_SenderKeyInfo> m_mSenderKey;  // senderPlayerId -> {frequency, fillHash, stampedMs}
   ```

   written **unconditionally on every `keyed=true` broadcast** — before the
   tuned/reachable/powered gates that (correctly) gate squelch beeps. Without
   this, a receiver who tunes onto the net, powers a radio on, or walks into
   range *mid-transmission* would have no hash and hear encrypted traffic in
   the clear for the rest of it — a repeatable, no-cheat crypto bypass.
   Cleanup: unconditionally on `keyed=false` for that sender (NOT behind the
   `m_mKeyedSenders.Contains` gate), on the server-relayed disconnect stop,
   and by a failsafe expiry of its own, refreshed by re-key broadcasts and
   set well above the squelch's 2-minute stuck-key expiry (fixes: hash of a
   >2-minute continuous transmission being deleted mid-stream).
3. **Server always rebroadcasts a re-key** (fixes the debounce swallow): in
   `IRRU_RelayKeyState`, the 300 ms stop-debounce cancel path currently
   returns without broadcasting the fresh `keyed=true` — which would drop a
   just-changed fill hash on the floor (worst case: a sender who *cleared*
   their fill still scrambles for everyone — plaintext failing closed,
   violating the cardinal rule). Fix: always rebroadcast (receivers' `Open()`
   is already idempotent — no duplicate beep), which also refreshes the
   stuck-key timestamp. Server additionally keeps last-hash per player
   alongside `m_mIRRU_KeyedFreqByPlayer`.
4. **JIP sync**: on player spawn (while a transmission is active), the server
   pushes its current keyed-player table (playerId, frequency, range, pos,
   hash) to the spawning client only, via an **owner-targeted RPC on that
   player's VON controller** — never a broadcast re-announce, which would
   reach every client on every respawn and could re-open squelch channels
   that legitimately closed by silence timeout elsewhere. Without this a JIP
   client hears in-progress encrypted traffic in the clear until the next
   key-up.
5. **Sender-side latch failsafe**: `m_iIRRU_KeyedFrequency`'s same-frequency
   early-return currently has no recovery if a stop is ever missed (a desync
   would silently suppress all future key RPCs — and with them the hash — on
   that frequency). Add: when the controller observes it is not transmitting
   while the latch is set, send the stop and reset. (Composes with change 3.)
6. **Retune-while-keyed** (existing squelch quirk, promoted to a real bug by
   crypto): if the frequency input commits a new frequency on the transceiver
   that is ACTIVELY transmitting (identity-checked against the active entry's
   transceiver, not just frequency — retuning radio B while transmitting on
   same-frequency radio A must not strip A's announced hash), send stop(old)
   + start(new, new-freq hash) — otherwise voice moves to the new frequency
   with no hash ever announced there and the encrypted net plays in the clear.
   Additionally, the kill switch ages currently-scrambling streams on disable
   so they re-verdict to plaintext within a packet, and the race-correction
   path re-verdicts with the stream's stored editor flag (a GM's mismatched
   fill must not churn spurious event restarts).

Trust model: the server remains a pure relay of client-claimed values —
acceptable on private unit servers, consistent with the rest of the mod.

### 3.4 Verdict + audio write

Site: `506th_VoNComponent.IRRU_ApplyAudioVariables()` — the single place
per-stream audio variables are written, called on stream START/RESUME per the
global-slot latch design. `isSenderEditor` must be plumbed in from `OnReceive`
(one added parameter — it is not currently in the signature).

```
verdict = INTELLIGIBLE
if EncryptionEnabled (replicated toggle)
   and receiver != null                        // radio traffic only
   and NOT isSenderEditor                      // GM transmissions always clear
   and NOT IRRU_LocalPlayerIsGM()              // GM listening always clear
then
    senderHash = GetSenderFillHash(senderPlayerId, frequency)   // 0 if unknown
    myHash     = EarSettings.GetFillHash(frequency)
    if senderHash != 0 and senderHash != myHash:
        verdict = ENCRYPTED
```

- **GM-listening bypass is based on the local player's editor state** (e.g.
  `SCR_CharacterHelper.GetCharacterControlType() == UNLIMITED_EDITOR` /
  `!editorManager.IsLimited()` — both proven patterns in our other mods), NOT
  on `receiver.GetRadio().IsEditorRadio()`: the verdict lands in shared global
  audio slots, and a GM with both an editor radio and a possessed character's
  radio tuned to the same net would otherwise get whichever transceiver's
  verdict fired first. (`IsEditorRadio()` exists in the API listing but is
  unverified in Workbench; it may serve as a secondary check.)

On `ENCRYPTED`:
- `JamStrength = 0` (script convention pre-inverts: 0 = maximum jam). Proven
  by the graph: at 0, `Voice_V` mutes the voice bus completely while `Noise_V`
  drives the Brownian+Pink generators at full, radio-coloured by the same
  filter chain. Full jam already IS pure noise with zero intelligible voice.
- `SignalQuality = max(computedQuality, 0.15)` — below ~0.3 the SignalQuality
  sig attenuates the whole radio sound (noise included, silence not scramble),
  but a hard 0.3 floor would make encrypted noise LOUDER than legitimate
  marginal-range speech on the same link. 0.15 keeps the noise present while
  still range-scaled; documented as an audibility trade-off so testers don't
  file it as a bug.
- EarRouting/ChannelVolume written as today — scrambled traffic obeys the
  receiver's ear routing and channel volume.

On `INTELLIGIBLE`: identical to today.

Timing and races (reworded per review — the honest version):
- The verdict is computed fresh per stream open (never through the 500 ms
  `IRRU_GetSenderSignals` cache — fills change between streams).
- **Guaranteed re-verdict happens only at STREAM START** (gap > 800 ms or
  frequency change). A RESUME (400–800 ms gap) re-writes the slots but the
  engine may not re-read them. A receiver-side fill change mid-incoming-stream
  therefore applies at the next START.
- **Voice-beats-RPC race**: if voice packets arrive before the key-state RPC
  (two-hop reliable RPC vs. streaming voice — becomes *more* likely on
  degraded connections, not less), the stream opens INTELLIGIBLE and, for
  continuous speech, would stay so until the next START. Mitigation designed
  in: when a `keyed=true` broadcast arrives carrying a hash that contradicts
  the verdict of that sender's currently-open stream, force the stream's next
  packet to classify as START (reset its `m_fLastPacketMs`) — one audible
  event restart in exchange for the verdict correcting within ~one packet.
- **Same-frame double-START collision** (existing global-slot residual): two
  genuine STARTs in one frame are last-writer-wins for BOTH sound events —
  with crypto this could play an encrypted stream in the clear for a whole
  event. Deliberate policy: **within a frame, an ENCRYPTED verdict wins over
  an INTELLIGIBLE one** — the one place the design fails closed, because
  briefly scrambling friendly traffic is recoverable ("say again") while
  leaking encrypted traffic breaks the feature's promise. Documented as such.

### 3.5 Server master switch + runtime kill switch

`Scripts/Game/Settings/IRRU_EncryptionSettings.c`, clone of
`IRRU_RFPropagationSettings`: `$profile:IRRU_Encryption.json` →
`{ "EncryptionEnabled": false, "DebugEnabled": false }`, server-read,
replicated via `IRRU_RFPropagationNetworkComponent` (`[RplProp] bool
m_bEncryptionEnabled`, static accessor fail-safing to `false`).

**Runtime admin kill switch** (review-promoted, the single biggest HQ
confidence feature): an admin-gated chat command (`radiocrypto on|off`, via
the existing `IRRU_RadioChatCommands` + a server RPC with admin check) flips
the replicated flag live, no restart. Everything downstream fail-safes to
plaintext the moment it goes false — if a trial melts down 40 minutes into an
op, one command ends it without ending the op.

**Discoverability** (review-promoted): hook the RplProp with an `onRplName`
callback — on a false→true transition, and once at join while true, each
client shows a one-time chat line: *"COMSEC enabled on this server — [key] on
a channel in the VON menu loads a crypto fill."* Without this, a mid-campaign
enable is invisible (plaintext works fine, so nobody ever discovers the
feature) and the S3's "secure net" is silently plaintext.

Disabled means: no fill action dispatch, no indicator, verdict short-circuits
to INTELLIGIBLE, RPC hash carried but ignored. Zero behavioral delta.

## 4. UX specification

### 4.1 Entering a fill

- New action `IRRU_SetCryptoFillAction` in `VONMenuContext`, default **LCtrl+G
  chord** — NOT a bare key. (Review blocker-adjacent: bare G is grenade
  quick-throw in the vanilla character context, which stays active under the
  VON radial; a comms keybind that can prime a grenade is unacceptable. The
  mod already has a chord precedent: LCtrl+F = alternate channel. Verify the
  final choice against vanilla contexts in Workbench before shipping.)
  Gamepad: **explicitly unsupported** for fill entry (typed digits require a
  keyboard); documented rather than accidental.
- Dispatch via the existing per-frame action table + `IRRU_GetRadialSelection()`
  (~6 lines), hint registered in `AvailableActions.conf` only while encryption
  is enabled.
- Opens `IRRU_CryptoFillInput` — reuses the frequency-input LAYOUT (no new UI
  resource) but **with a real cancel path** (fixes review BLOCKER-1; the
  confirm signal is the edit-box handler's `OnChange(finished=true)`; the
  frequency modal's
  Enter-and-Esc-both-commit quirk is safe there only because empty input is
  rejected and the prefill equals the current value — with "empty = clear
  fill" it would make a panic-Esc silently destroy the fill):
  - Only **Enter commits** (hook the EditBox confirm event); Esc, focus loss,
    or the radial closing **cancels** with no state change.
  - **Empty input is a NO-OP** (like frequency entry). Clearing is explicit:
    type `0` and Enter = clear to plaintext.
  - Set and clear produce **distinct feedback**: set plays the cycle one-shot
    plus transient text `FILL SET K7A` (check value, §4.2); clear plays the
    local-off one-shot plus `FILL CLEARED - PLAINTEXT`. Never the same beep
    for both.
  - Digit-only, max 8 chars; title "CRYPTO FILL"; shows the channel frequency;
    prefills the current fill (the modal is where the value is visible).
- Entering a fill while transmitting doesn't affect the in-progress key-up;
  the next key-up carries the new hash (see §3.3-3 for the rapid-rekey case).

### 4.2 Per-channel indicator

`506th_VONEntryRadio.Update()` appends a crypto segment while encryption is
enabled (indicator reflects the fill of the entry's currently displayed
frequency — primary vs alternate — which the frequency-keyed map handles):

- Fill loaded: **`|K7A`** — `K` plus a two-character check value derived from
  the fill hash (e.g. hash mod 1296 in base-36). Many-to-one, so it leaks
  nothing on streams/screenshots, but a changeover becomes verifiable at a
  glance and over voice: *"confirm Kilo-Seven-Alpha on Battalion."* A bare
  binary `|K` (v1.0) made mixed-fill changeovers undiagnosable — every radial
  showed the same K and the only debug channel was the broken net itself.
- No fill, encryption enabled: **`|--`** — the radial doubles as a pre-op
  checklist ("every net shows K = I'm ready") and a post-relog lost-fill state
  is visibly different. Plaintext stays the behavioral default; it just stops
  being visually invisible while the feature is live.
- Encryption disabled: nothing appended (today's string exactly).

### 4.3 Transmit-side secure cue

Real crypto radios tell the operator they're in cipher mode (SINCGARS CT/PT);
with fail-open plaintext, a sender with a cleared/typo'd fill would otherwise
get positive-sounding comms indefinitely. Cheap fix using owned machinery: a
**distinct TX key-up beep variant when transmitting on a filled channel**
(one extra mapping in `IRRU_RadioBeepHelper`'s style→event table; reuse an
existing event variant, no new assets). The operator hears secure vs plaintext
on every key-up.

### 4.4 Chat commands

Phase A (review-promoted from Phase B):
- `radiofill <MHz> <digits|0>` — set/clear without the radial (also the
  keyboard-less fallback).
- `radiocrypto on|off` — admin-gated runtime kill switch (§3.5).
- `radiosettings` — additionally lists fill state per known frequency as
  check values.
- *(Moved to Phase B in implementation: `radiocrypto who <player>` — needs a
  server→target-client query RPC; and full-digit listings — the encryption
  DebugEnabled flag is server-side only, deliberately not replicated. Interim
  GM diagnostic: with DebugEnabled the server logs every key-up's frequency
  and fill hash, and players can read each other their check values.)*

### 4.5 What the other guy hears

Mismatched traffic: squelch opens normally (RX beeps per channel style), then
the transmission renders as harsh radio-band noise at the receiver's routing
and channel volume, for the duration of the real speech. You know the net is
active; you can't understand it — and noise (vs. silence) tells you your fill
is wrong instead of whispering "nobody's talking."

## 5. Edge-case matrix

| Case | Behavior | Mechanism |
|---|---|---|
| Server toggle OFF / admin `radiocrypto off` | Exactly today's mod, immediately | verdict short-circuit + UI gated + RplProp |
| Both plaintext | Intelligible | hash 0 == 0 |
| Sender filled, receiver not | Scramble | mismatch |
| Sender plaintext, receiver filled | **Intelligible** | senderHash 0 fails open |
| Fills differ | Scramble | mismatch |
| GM/editor transmitting | Intelligible to all | `isSenderEditor` bypass |
| GM listening | Hears everything | local editor-state bypass (not per-transceiver) |
| Sender with no key RPC (other mod / vanilla path) | Intelligible | unknown hash fails open |
| **Receiver tunes in / powers on / walks into range mid-transmission** | Scrambles correctly | ungated sender-hash table (§3.3-2) |
| **JIP during a transmission** | Scrambles correctly | server JIP sync (§3.3-4) |
| Voice packet beats key RPC | Stream may open intelligible; corrected within ~one packet of the RPC arriving via forced START | §3.4 race mitigation |
| Receiver changes fill mid-incoming-stream | Applies at next stream START (>800 ms gap); RESUME writes may be ignored by the engine | latch semantics |
| **Alternate-channel PTT** | stop(primary)+start(alt) carries the alternate net's hash; different fill per net Just Works; hash on `keyed=false` ignored by receivers | §3.3-1 |
| Rapid re-key <300 ms (incl. alt-PTT taps, fill change between keys) | Fresh hash always reaches receivers | server always rebroadcasts re-keys (§3.3-3) |
| Continuous transmission >2 min | Stays scrambled | hash table has its own refreshable expiry, not the squelch's 2-min stuck-key expiry (§3.3-2) |
| **Sender retunes while keyed** | stop(old)+start(new, new hash) on frequency commit (or retune blocked while keyed) | §3.3-6 |
| Respawn / new radio | Fills retained | frequency-keyed map |
| Disconnect / reconnect | Fills retained | **persistence in MVP** (§3.2) |
| Retuning a channel to a new frequency | The NEW frequency's fill applies | fills belong to nets |
| Two radios tuned to same freq, different power states | Fix `FindTunedTransceiver` to prefer a powered match (today: first match, then null if unpowered — would drop the key-start and bypass crypto) | §3.3 note |
| Multiple same-frame STARTs, mixed verdicts | Last-writer-wins residual; **ENCRYPTED wins within the frame** (deliberate fail-closed exception) | §3.4 |
| Jammer + encryption | Scramble wins (JamStrength already 0) | min of effects |
| Relays / coverage / delivery / third-party radios | Unaffected | engine key untouched |
| Listen server | **Unsupported/untested** (unit runs dedicated only); receiver-side hash handling kept idempotent regardless | declared, not discovered |
| Accidental modal exit while "just checking" | No state change | Enter-only commit (§4.1) |
| Hacked/modified client | Could unmute (audio-layer masking is client-side) | accepted; see §8 limitations |

## 6. Implementation estimate

Phase A — MVP (two focused sessions + one 2-client rig session):
- `IRRU_EncryptionSettings.c` (new, settings clone + runtime flag plumbing)
- `IRRU_RFPropagationNetworkComponent.c` (+RplProp +onRplName notice, +RPC
  hash arg, always-rebroadcast re-keys, last-hash per player, JIP sync RPC,
  admin toggle RPC)
- `506th_VONController.c` (+action dispatch, +hash at key-up, sender latch
  failsafe, retune-while-keyed handling)
- `506th_RadioRxSquelch.c` (+ungated sender-hash table + lifecycle + accessor;
  `FindTunedTransceiver` powered-preference fix)
- `506th_VoNComponent.c` (+verdict incl. GM local-state bypass, +race
  correction, +collision policy, isSenderEditor plumbing; ~40 lines)
- `506th_EarRouting.c` (+fill map + accessors + hash + persistence hook)
- `IRRU_RadioUserSettings.c` (+fill persistence)
- `506th_CryptoFillInput.c` (new; confirmed-entry-only commit, cancel path;
  reuses the existing frequency-input layout)
- `506th_VONEntryRadio.c` (+`|K7A` / `|--` indicator)
- `IRRU_RadioBeepHelper.c` (+secure TX beep mapping)
- `IRRU_RadioChatCommands.c` (+radiofill, +radiocrypto, +radiosettings ext)
- configs: input chord, keybinding entry, hint entry
- **No new audio assets. No .acp/.sig/.conf audio changes. No prefab changes.**

Phase B — polish: GM fill-roster UI, fill presets in op-order tooling,
radio-check integration.

Phase C — EW expansion (separate design): direction finding, traffic analysis;
in-game fill compromise mechanics (capture, DF) live here, not in the MVP.

## 7. Verification plan (when implemented)

1. **Toggle OFF parity** (dedicated): full regression of beeps, routing,
   volume, squelch, jammers, propagation; no fill UI reachable.
2. **Two-client rig, ON**: matched fills both ways; receiver clears →
   scramble next transmission; sender clears (types `0`) → intelligible to
   all; GM editor TX to filled receiver; GM (editor open) hears filled
   traffic; possessed-GM with dual radios.
3. **Transport races**: tune onto an in-progress encrypted transmission →
   scrambles; JIP mid-transmission → scrambles; rapid re-key under 300 ms
   with a fill change between; alternate-PTT with a different fill per net,
   including rapid alt-taps; retune while keyed; >2-minute continuous key-up.
4. **UX**: Esc/focus-loss cancels with no state change; empty Enter no-ops;
   `0` clears with distinct feedback; check values match between two clients
   with the same fill; `|--` appears after a deliberate settings wipe.
5. **Kill switch**: `radiocrypto off` mid-transmission returns everything to
   plaintext behavior immediately; `on` re-arms and fires the client notice.
6. **Audio by OBS**: scramble = noise signature, zero intelligible voice,
   obeys ear routing and channel volume; marginal-range loudness of encrypted
   noise vs plaintext speech (the 0.15 floor trade-off). Capture rig must
   have Windows audio enhancements OFF (see the Realtek APO incident,
   2026-08-23).
7. **Dedicated-server soak** on Official Server 3 with the isolation config
   before any production rollout.

## 8. Business case for Unit HQ (liftable as-is)

**What it is.** Per-channel crypto fills, analogous to loading COMSEC into a
KY-57/SINCGARS: the RTO enters the day's fill, the net is secure, anyone
without it hears encrypted noise. Fill entry and changeover become a real RTO
task; compromise and EW scenarios become GM-directable mechanics (in-game
compromise tooling — capture, direction finding — is a later phase, stated
plainly).

**Where it pays off — named, not implied.** On standard co-op nights against
AI, encryption changes nothing anyone hears; the payoff is in TvT events,
GM-played OPFOR, and spy/compromised-net missions — the GM can issue OPFOR
the fill (they eavesdrop) or not (they only know when you transmit), which is
an entire mission mechanic. **The trial should be committed to one such op**,
not a generic co-op night where the feature is invisible.

**What it costs the unit: nothing until switched on, and one command to
switch off.** Ships OFF; with the flag false there is no UI and no behavior
change. The trial protocol is three lines: enabled by the S4 for one
designated op on Server 3; abort criterion — if comms confusion exceeds what
the GM tolerates, an admin issues `radiocrypto off` and the feature is gone
mid-op without a restart; success criterion — the op's comms plan (fills in
the op order, one changeover) executes without GM intervention.

**Why it will not create op-night chaos (the engineered defaults):**
- An un-filled soldier **can always transmit and be heard by everyone**, and
  hears all plaintext traffic. What he cannot do is monitor a *filled* net
  until he enters its fill — one number, same UX as frequency entry, briefed
  at op start. (Stated precisely: the day the unit adopts a filled command
  net, entering that fill becomes part of radio checks — that is the entire
  onboarding burden.)
- Fills persist through crashes and reconnects, and the radial shows a
  per-channel check value (`K7A`) so "am I on the right fill" is a glance,
  not a guess — changeovers are verifiable over voice.
- Wrong fill fails LOUD (noise on the net) instead of silent; forgetting to
  load one is visible (`|--`) in the radial checklist.
- Clearing is explicit (`0` + Enter) with distinct feedback; accidental
  Esc/panic-close changes nothing.
- GMs are exempt both directions, and get a diagnostic command
  (`radiocrypto who`) for "I hear noise on command net" tickets.
- Fill custody/distribution is SOP, not code: S6/RTO publishes fills in the
  op order; a rejoiner gets re-keyed by their squad lead over direct voice or
  the plaintext admin net. (The mod deliberately does not gatekeep who may
  enter a fill.)

**Honest limitations.** This is a gameplay/realism layer, not cryptographic
security: scrambling is fail-open by design — rare timing races and unmodded
senders render intelligible rather than scrambled — and a technically
modified client could recover audio. On private, password-protected unit
servers this matches every other trust assumption already made. Voice
delivery, relays, and third-party radio mods are untouched.

---

*Design references: exploration reports 2026-08-23 (audio graph, UX/RPC
plumbing, engine API — session artifacts); the restricted HQ VoN analysis
post (delivery-layer RE); `IRRU_RFPropagationSettings.c` (settings pattern);
`506th_VON.acp` / `506thVon.sig` (jam-noise mechanism). v1.1 folds in the
2026-08-23 adversarial review (35 findings; both blockers and all majors
addressed above).*
