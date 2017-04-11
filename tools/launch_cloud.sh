#!/usr/bin/env bash
set -e -u -x -o pipefail

PROJECT=alpha_zebra_pizza_956
CONFIG=deploy/default.json
CLUSTER=eu1-prod
SNAPSHOT=build/initial.snapshot

spatial upload $1
spatial deployment launch --cluster=${CLUSTER} --snapshot=${SNAPSHOT} ${PROJECT} $1 ${CONFIG} $1
