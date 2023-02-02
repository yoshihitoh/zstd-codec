#!/bin/env bash

set -e

CPP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd ${CPP_DIR} && \
    bash update_projects.sh && \
    cd build-emscripten && \
    emmake make -j$(sysctl -n hw.ncpu) config=debug


# cd "${PROJ_DIR}"
# docker image build -t "yoshihitoh/zstd-emscripten" .
# 
# # TODO: run if container does not exists
# docker container rm zstd-emscripten
# docker container run \
#     --name zstd-emscripten \
#     -v ${SRC_DIR}:/emscripten/src \
#     -v ${BUILD_DIR}/:/emscripten/build/ \
#     -v ${LIB_DIR}:/emscripten/artifacts \
#     yoshihitoh/zstd-emscripten \
#     /bin/bash --login /emscripten/build/build_zstd_binding.sh
