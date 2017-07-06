#!/usr/bin/env bash
set -e -u -x -o pipefail

CONFIG=${CONFIG:-deploy/default.json}
REGION=${REGION:-eu}
SNAPSHOT=${SNAPSHOT:-build/initial.snapshot}
NAME=${1:-stu_test}

spatial upload ${NAME}
spatial cloud launch --cluster_region=${REGION} --snapshot=${SNAPSHOT} ${NAME} ${CONFIG} ${NAME}
