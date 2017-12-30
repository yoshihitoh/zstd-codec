#!/bin/env bash

set -e

CPP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd ${CPP_DIR} && \
    bash update_projects.sh && \
    cd build-emscripten && \
    emmake make -j$(sysctl -n hw.ncpu) config=release verbose=1
