# Handover 010 — Multiplayer: PvPvE arena "Nachtdienst"

**Branch:** `feature/multiplayer-arena`  ·  **Author + executor:** same session (2026-07-15)  ·  **Status:** done
**Scope:** an up-to-8-player deathmatch arena in a dedicated store, with the shop's own
monsters as a shared hazard (Ben: "an arena where you're in the same AH. Like Arc Raiders").
Built in-session rather than handed over; this doc is the design record, not an execution plan.

## Decisions — locked ✅

- **Mode v1:** PvPvE deathmatch (Ben, in-session). Co-op campaign, extraction rules,
  name tags, spectator: explicitly out of scope.
- **Local-first (Ben):** everything verified against a relay on localhost; public
  hosting (Hetzner VM + Caddy wss) is a deferred follow-up round. Until then the
  deployed page's ARENA button reports "geen relay geconfigureerd".
- **Web-only:** the native build compiles the net code but has no join path.
- Toy-grade netcode by design: no interpolation buffers beyond two packets, no
  reconciliation, trust between clients. Upgrade path is a bigger round.

## Authority model

One rule: you own what you can lie about least.

| Thing | Authority | Mechanism |
|---|---|---|
| My position/yaw/health/weapon | me | streamed at 15 Hz, peers interpolate the last two packets |
| A hit I land | shooter | `HitMe` event to the victim, who applies it to themselves |
| My death + kill credit | victim | `Died {killerId}` broadcast; killer counts the frag; killerId 0 = the shop |
| The monsters | host (oldest member) | host runs `EnemiesUpdate` targeting the *nearest* shopper, streams at 10 Hz; others puppet (lerp + local anim timers) |
| Enemy damage | host | shooters send `EnemyHit` to the host, host applies `EnemyHurt` |
| Pickups | taker announces, host respawns | `PickupTaken` broadcast · host's 18 s clock → `PickupRespawn`; late joiners get a targeted taken-state catch-up |
| Rockets | shooter | `Rocket` event spawns a visual-only replica (`Projectile.remote`) on peers; splash damage is computed by the shooter only |

Host re-election is the relay's (oldest survivor); `left` carries the new hostId and a
rising `isHost` edge re-arms pickup clocks (`gNet.hostArmed`).

## Wire protocol

Relay envelope (`server/README.md`): `welcome/peer/left` + opaque `{"t":"g","from":i,"d":…}`
frames, `to:` for targeted. Game frames inside `d`: `s` state, `e` event (numbers mirror
`NetEvent` in `claude/src/net.h`), `E` batched enemy rows. JSON, ~15 KB/s per client at 8 players.

## Key files

| Concern | Where |
|---|---|
| Relay (rooms, ids, host election) | `server/Program.cs` |
| Peer/net state, event pump, host duties | `claude/src/net.h` + `net.cpp` |
| JS↔wasm bridge (exports + EM_JS) | `claude/src/web_net.cpp` |
| WebSocket client, join UI, names/scores/feed | `web/index.html` (`__net`, `__netdbg`) |
| Arena map + `LoadArena` | `claude/src/map.cpp` (`kArena`) |
| Mode plumbing, respawn flow | `claude/src/world.{h,cpp}`, `game.cpp` (`UpdateArenaRespawn`) |
| Prey targeting (nearest shopper) | `claude/src/enemy.cpp` (`AcquirePrey`/`HurtPrey`) |
| PvP damage sweeps | `claude/src/player.cpp` (`ScanHit`/`ApplyHit`) |
| Peer billboards + palette | `claude/src/render.cpp`, sprite `assets/player_klant.png` (`tools/gen_assets.py`) |
| Arena HUD (frags, standings, countdown) | `claude/src/hud.cpp` |

Strings cross the boundary in exactly one place: JS composes feed/standings (it owns
the names) and writes into `net_msg_buf` via `stringToUTF8`
(`-sEXPORTED_RUNTIME_METHODS=stringToUTF8` in `web/build.sh`).

## Design notes worth keeping

- Portrait pause is disabled in the arena (`gWebInput.paused` exception in
  `GameUpdate`): a live match waits for nobody; the rotated phone just stands there.
- Kill credit is last-blow, Doom rules — the trolleys steal kills, and that's the game.
- Enemy stream slots are `w.enemies` indices; trolley respawns reuse their Dead slot
  in place so indices never move. Cap `kNetMaxEnemies` 16.
- The arena has no doors, no keycard, no exit: `escaped` can't fire.

## Verification (all green, 2026-07-15)

- `verify_mp.js` (session scratchpad; puppeteer + local relay + two headless pages):
  roster both sides · movement propagation (4.2 units observed remotely) · 8 shared
  enemy slots at 0.00 drift · a credited PvP frag on both clients · respawn ·
  host handoff with the enemy stream continuing under the new host · 0 pageerrors.
- `verify_touch.js` solo-mobile regression: fully green (menu button `#btn-solo`).
- Native cmake build + `dotnet build server` + `./web/build.sh`: clean.

## Hosting round — DONE ✅ (2026-07-15)

The public arena went live this session. Rather than a fresh VM, the relay landed
on an existing Hetzner host as one more app behind a **shared Caddy** reverse proxy
(reusing that host's established app pattern), not the bundled
`docker-compose.prod.yml`/Caddy shape (that stays for a standalone host).

- Relay: `server/docker-compose.hetzner.yml` (relay-only, joins the host's shared
  Docker network, exposes 8080); deploy via `server/deploy.sh` (rsync + build).
- Front: shared-Caddy vhost `relay.amberglass.co → ah-relay-app:8080` (WebSocket
  upgrade passes through untouched). DNS **grey-cloud** A record → the host,
  Caddy auto-TLS (Let's Encrypt).
- Page: `RELAY_URL=wss://relay.amberglass.co ./web/build.sh`, redeployed to Pages.
- Verified: real two-browser join over the public relay (roster, movement, 8 shared
  enemies at 0.00 drift) — `scratchpad/verify_public.js`, `mp_public.png`.
- **Cloudflare answer:** the orange-cloud proxy does support `wss://`, but the host's
  existing apps are all grey-cloud with Caddy doing Let's Encrypt directly, so the
  relay matches that (grey). Orange would need a CF origin cert + has a ~100 s idle
  WebSocket cut; grey has neither.
- **Gotcha hit + fixed:** the shared Caddy's graceful reload loaded the vhost into
  HTTP routing but didn't trigger cert provisioning (a known bind-mount reload quirk
  on that host). Fix was a `--force-recreate` of the Caddy container — a ~3 s blip,
  then the cert issued cleanly.
- The shared-Caddy vhost config lives in that host's private infra repo (outside
  this game repo) — recorded and maintained there.

## Remaining (future rounds, not this one)

1. Balance after real play (trolley respawn pace, scanner lanes, spawn table).
2. Nice-to-haves parked: name tags over heads, scoreboard key, co-op campaign mode.
3. The relay is single-instance on a home-adjacent box; if it gets real traffic,
   revisit (health checks, a second instance, or a managed host).

---
**On completion:** log row 010 set to ✅ done (2026-07-15) — done in this session.
