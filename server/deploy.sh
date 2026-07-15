#!/usr/bin/env bash
# Deploys the relay to its VM. STUB — the hosting round fills in the host.
# Expected flow once a box exists (Hetzner, per repo convention):
#   - rsync this directory to the VM
#   - docker compose -f docker-compose.prod.yml up -d --build there
set -euo pipefail

HOST="${RELAY_HOST:-}"   # e.g. root@1.2.3.4, set in the environment or below
if [ -z "$HOST" ]; then
  echo "RELAY_HOST is not set — no relay VM exists yet (hosting is a follow-up round)." >&2
  echo "Local dev instead: docker compose up  (ws://localhost:8787)" >&2
  exit 1
fi

cd "$(dirname "$0")"
rsync -az --delete --exclude bin --exclude obj ./ "$HOST":ah-relay/
ssh "$HOST" "cd ah-relay && docker compose -f docker-compose.prod.yml up -d --build"
echo "Relay deployed to $HOST"
