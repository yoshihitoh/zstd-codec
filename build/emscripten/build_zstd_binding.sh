#!/bin/env bash

set -evx

ROOT_DIR=/emscripten
ZSTD_DIR=${ROOT_DIR}/zstd
LIB_DIR=${ROOT_DIR}/lib
SRC_DIR=${ROOT_DIR}/src
ARTIFACTS_DIR=${ROOT_DIR}/artifacts

ZSTD_LIB=libzstd.bc

# build binding
OPT_LEVEL=3
USE_CLOSURE=1
EMCC_FLAGS="--bind -O${OPT_LEVEL} -std=c++1z --memory-init-file 0 -I${ZSTD_DIR}/lib"

cd ${SRC_DIR}
em++ ${EMCC_FLAGS} \
    -o ${ARTIFACTS_DIR}/zstd-binding.js \
    ${LIB_DIR}/${ZSTD_LIB} *.cc \
    -s DEMANGLE_SUPPORT=1 \
    -s 'EXTRA_EXPORTED_RUNTIME_METHODS=["FS"]' \
    -s MODULARIZE=1 \
    -s USE_CLOSURE_COMPILER=${USE_CLOSURE} \
    -s ALLOW_MEMORY_GROWTH=1
