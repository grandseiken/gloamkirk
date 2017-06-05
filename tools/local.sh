#!/usr/bin/env bash
set -e -u -x -o pipefail

CONFIG=${CONFIG:-deploy/default.json}
SNAPSHOT=${SNAPSHOT:-build/initial.snapshot}

spatial local launch --enable_remote_snapshot_flow=true --snapshot=${SNAPSHOT} ${CONFIG}
