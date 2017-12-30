#!/bin/env bash

set -e

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CPP_DIR=$(greadlink -f ${ROOT_DIR}/cpp)

docker stop zstd-emscripten || true && \
    docker rm zstd-emscripten || true

docker container run \
    --name zstd-emscripten \
    -v ${CPP_DIR}:/emscripten/src \
    yoshihitoh/zstd-emscripten \
    /bin/bash --login /emscripten/src/build-emscripten-release.sh
