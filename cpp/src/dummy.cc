#include <cstdio>
#include <emscripten.h>

#include "zstd.h"

EMSCRIPTEN_KEEPALIVE
extern "C" void say_hello() {
    printf("Hello, World!\n");
}

EMSCRIPTEN_KEEPALIVE
extern "C" unsigned int zstd_version() {
    return ZSTD_versionNumber();
}
