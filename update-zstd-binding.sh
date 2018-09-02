#!/bin/env bash

set -e

ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CPP_DIR=$(greadlink -f ${ROOT_DIR}/cpp)
JS_DIR=$(greadlink -f ${ROOT_DIR}/js)

CONTAINER_NAME="zstd-emscripten"
IMAGE_NAME="yoshihitoh/zstd-emscripten"

CONTAINER_ID="$(docker container ls -qa -f name=${CONTAINER_NAME})"

# move to root directory
cd ${ROOT_DIR}

# build image
echo "build docker image on $(pwd)..."
docker image build -t "yoshihitoh/zstd-emscripten" .

# check container image, using latest image or not.
if [ ${CONTAINER_ID} ]; then
    # stop and remove if container exists
    echo "removing existing container..."

    docker container stop "${CONTAINER_NAME}" || true && \
        docker container rm "${CONTAINER_NAME}" || true
fi

# run new container
echo "run new container and build sources..."
docker container run \
    --name "${CONTAINER_NAME}" \
    -v ${CPP_DIR}:/emscripten/src \
    "${IMAGE_NAME}" \
    /bin/bash --login /emscripten/src/build-emscripten-release.sh

# copy compiled binindg into js dir
echo "copying compiled binding into js/lib..."
docker container cp \
    ${CONTAINER_NAME}:/emscripten/src/build-emscripten/bin/Release/zstd-codec-binding.js \
    ${JS_DIR}/lib

docker container cp \
    ${CONTAINER_NAME}:/emscripten/src/build-emscripten/bin/Release/zstd-codec-binding-wasm.js \
    ${JS_DIR}/lib

echo "done!"
