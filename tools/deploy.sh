#!/usr/bin/env bash
set -e -u -x -o pipefail

PROJECT=alpha_zebra_pizza_956
CONFIG=${CONFIG:-deploy/default.json}
CLUSTER=${CLUSTER:-eu3-prod}
SNAPSHOT=${SNAPSHOT:-build/initial.snapshot}
NAME=${1:-stu_test}

spatial upload ${NAME}
spatial deployment launch --cluster=${CLUSTER} --snapshot=${SNAPSHOT} ${PROJECT} ${NAME} ${CONFIG} ${NAME}
