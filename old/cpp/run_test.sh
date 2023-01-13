#!/bin/bash

CPP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_TYPE=$1

if [ "${BUILD_TYPE}" == "" ]; then
    BUILD_TYPE="Release"
fi

mkdir -p "$CPP_DIR/test/tmp"

cd $CPP_DIR
./build-gmake/bin/${BUILD_TYPE}/test-zstd-codec
