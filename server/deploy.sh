#!/usr/bin/env bash
# Deploy the relay to the shared-Caddy Hetzner box (AMB-HZ-001), reached over
# Tailscale. The box's shared Caddy fronts it at wss://relay.amberglass.co (the
# vhost lives in Amberglass.Infra's shared Caddyfile). Idempotent: rsync the
# source, then rebuild + (re)start the container in place.
#
#   ./deploy.sh                     # deploy to the default box
#   RELAY_HOST=root@1.2.3.4 ./deploy.sh   # override target
set -euo pipefail

HOST="${RELAY_HOST:-root@100.77.229.32}"   # AMB-HZ-001 tailnet address
DEST="${RELAY_DEST:-/opt/ah-relay}"

cd "$(dirname "$0")"
rsync -az --delete --exclude bin --exclude obj ./ "$HOST:$DEST/"
ssh "$HOST" "cd '$DEST' && docker compose -f docker-compose.hetzner.yml up -d --build"
echo "Relay deployed to $HOST:$DEST — fronted by shared Caddy at wss://relay.amberglass.co"
