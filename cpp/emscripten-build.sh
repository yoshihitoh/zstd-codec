#!/bin/env/bash

set -e

PROJ_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=$(greadlink -f ${PROJ_DIR}/build/emscripten)
SRC_DIR=$(greadlink -f ${PROJ_DIR}/src)
LIB_DIR=$(greadlink -f ${PROJ_DIR}/lib)

cd "${PROJ_DIR}"
docker image build -t "yoshihitoh/zstd-emscripten" .

# TODO: run if container does not exists
docker container rm zstd-emscripten
docker container run \
    --name zstd-emscripten \
    -v ${SRC_DIR}:/emscripten/src \
    -v ${BUILD_DIR}/:/emscripten/build/ \
    -v ${LIB_DIR}:/emscripten/artifacts \
    yoshihitoh/zstd-emscripten \
    /bin/bash --login /emscripten/build/build_zstd_binding.sh
