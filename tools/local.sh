#!/usr/bin/env bash
set -e -u -x -o pipefail

CONFIG=${CONFIG:-deploy/default.json}
SNAPSHOT=${SNAPSHOT:-build/initial.snapshot}

spatial local launch --snapshot=${SNAPSHOT} ${CONFIG}
