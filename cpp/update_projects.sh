#!/bin/env bash

CPP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -z "${ZSTD_DIR}" ]; then
    ZSTD_DIR="${CPP_DIR}/zstd"
fi

echo '------------------------------------------------------------'
premake5 gmake2 --with-zstd-dir=${ZSTD_DIR}
echo '------------------------------------------------------------'
premake5 gmake2 --with-zstd-dir=${ZSTD_DIR} --with-emscripten
