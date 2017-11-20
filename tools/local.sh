#!/usr/bin/env bash
set -e -u -x -o pipefail

CONFIG=${CONFIG:-deploy/default.json}
SNAPSHOT=${SNAPSHOT:-build/initial.snapshot}
LOCAL_IP=${LOCAL_IP:-127.0.0.1}

spatial local launch --runtime_ip=${LOCAL_IP} --snapshot=${SNAPSHOT} ${CONFIG}
