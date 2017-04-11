#!/usr/bin/env bash
set -e -u -x -o pipefail

spatial local launch --snapshot=build/initial.snapshot deploy/default.json
