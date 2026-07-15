# AH: Hell Aisle — room relay

The multiplayer arena's WebSocket server. A dumb router: it manages rooms,
assigns ids, keeps the host election consistent (oldest member hosts — the host
runs the monsters), and forwards opaque game frames between browsers. It never
parses gameplay, so client changes don't need a server deploy.

## Endpoints

| Route | What |
|---|---|
| `GET /ws?room=CODE&name=NAME&color=0-7` | the WebSocket (room capped at 8 players) |
| `GET /healthz` | `{ "rooms": n }` liveness probe |

## Protocol envelope

```
relay → client   {"t":"welcome","id":i,"hostId":h,"roster":[{id,name,color},…]}
                 {"t":"peer","id":i,"name":n,"color":c}    someone joined
                 {"t":"left","id":i,"hostId":h}            someone left, h = new host
                 {"t":"g","from":i,"d":{…}}                a game frame
client → relay   {"t":"g","d":{…}}                         broadcast to the others
                 {"t":"g","to":i,"d":{…}}                  one recipient (snapshots)
```

`from` is stamped by the relay and cannot be forged; everything inside `d` is the
game's business (defined in `web/index.html`).

## Run

```sh
dotnet run                # ws://localhost:8787 (dev default)
docker compose up         # same port, containerized
```

The web client (`web/index.html`) falls back to `ws://<hostname>:8787`
automatically when the page itself isn't served from Cloudflare Pages, so
local play is: relay up, `python3 -m http.server 8080` in `web/dist`, two tabs.

## Production (not live yet)

`docker-compose.prod.yml` + `deploy.sh` are the prepared shape for the hosting
round: relay behind Caddy with automatic Let's Encrypt TLS on `$RELAY_DOMAIN`,
because the game page is https and can only open `wss://`. Until that round,
multiplayer exists on localhost/LAN only.
