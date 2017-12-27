#!/bin/env/bash

BUILD_ZSTD_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SRC_DIR=$(greadlink -f ${BUILD_ZSTD_DIR}/src)
LIB_DIR=$(greadlink -f ${BUILD_ZSTD_DIR}/lib)
BUILD_DIR=$(greadlink -f ${BUILD_ZSTD_DIR}/build)

cd "${BUILD_ZSTD_DIR}"
docker image build -t "yoshihitoh/zstd-emscripten" .

# TODO: run if container does not exists
docker container rm zstd-emscripten
docker container run \
    --name zstd-emscripten \
    -v ${SRC_DIR}:/emscripten/src \
    -v ${BUILD_DIR}:/emscripten/build \
    -v ${LIB_DIR}:/emscripten/artifacts \
    yoshihitoh/zstd-emscripten
