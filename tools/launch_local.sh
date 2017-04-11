#!/usr/bin/env bash
set -e -u -x -o pipefail

CONFIG=deploy/default.json
SNAPSHOT=build/initial.snapshot

spatial local launch --snapshot=${SNAPSHOT} ${CONFIG}
