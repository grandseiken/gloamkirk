#!/usr/bin/env bash
set -e -u -x -o pipefail

export BUILD_CONFIG="Release"
export BUILD_GENERATOR="Unix Makefiles"
export BUILD_DEFINITIONS="-m64"
export MAKEFLAGS="-j 8"

git submodule update --init
spatial build --log_level=debug
