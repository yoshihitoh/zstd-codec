cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)
project(zstd-codec)

OPTION(ZSTD_CODEC_BUILD_TEST "Build Tests" ON)

# Zstandard
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "disable programs")
set(ZSTD_BUILD_CONTRIB  OFF CACHE BOOL "disable contrib")
set(ZSTD_BUILD_TESTS    OFF CACHE BOOL "disable tests")
set(ZSTD_BUILD_SHARED   OFF CACHE BOOL "disable shared library")   # use static library only
add_subdirectory(zstd/build/cmake)

# zstd-codec (common)
add_library(zstd-codec STATIC
            src/common-types.h
            src/raii-resource.h
            src/zstd-codec.cc
            src/zstd-codec.h
            src/zstd-dict.cc
            src/zstd-dict.h
            src/zstd-stream.cc
            src/zstd-stream.h
        )
add_dependencies(zstd-codec libzstd_static)
target_compile_definitions(zstd-codec PUBLIC -DZSTD_STATIC_LINKING_ONLY)
target_include_directories(zstd-codec PUBLIC
        ${CMAKE_SOURCE_DIR}/zstd/lib
        src
        )
target_link_libraries(zstd-codec PUBLIC libzstd_static)

# zstd-codec-wasm (for Browser)
if (EMSCRIPTEN)
    add_executable(zstd-codec-wasm src/binding/emscripten/zstd-binding.cc)
    target_link_libraries(zstd-codec-wasm PRIVATE zstd-codec)
    target_link_options(zstd-codec-wasm PRIVATE
            "SHELL: --bind"
            "SHELL: --memory-init-file 0"
            "SHELL: -s DEMANGLE_SUPPORT=1"
            "SHELL: -s MODULARIZE=1"
            "SHELL: -s WASM=1"
            "SHELL: -s SINGLE_FILE=1"
            "SHELL: -s BINARYEN_ASYNC_COMPILATION=1"
           )
endif (EMSCRIPTEN)

if (ZSTD_CODEC_BUILD_TEST)
    add_executable(test-zstd-codec
            test/main.cc
            test/catch.hpp
            )
    target_link_libraries(test-zstd-codec PUBLIC zstd-codec)
    if (EMSCRIPTEN)
        target_compile_definitions(test-zstd-codec PRIVATE
                "-DHAS_FS_WRITE=0"
                )
        target_link_options(test-zstd-codec PRIVATE
                "SHELL: --embed-file ${CMAKE_SOURCE_DIR}/test/fixtures@test/fixtures"
                "SHELL: --memory-init-file 0"
                )
    endif (EMSCRIPTEN)
endif (ZSTD_CODEC_BUILD_TEST)
