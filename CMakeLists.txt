cmake_minimum_required(VERSION 3.15...3.30)
project(zstd_codec)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Cxx)

# Options
option(ZSTD_CODEC_BUILD_TESTING "set OFF to suppress zstd-codec tests" ON)

# fmtlib/fmt
add_subdirectory(libs/external/fmt)

# tl/expected
set(EXPECTED_BUILD_TESTS OFF)
add_subdirectory(libs/external/expected)

# tl/optional
set(OPTIONAL_BUILD_TESTS OFF)
add_subdirectory(libs/external/optional)

# zstd
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "disable zstd programs")
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "disable zstd contribution")
set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL "disable zstd multithread support")
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "disable zstd shared library, use static library only")
add_subdirectory(libs/external/zstd/build/cmake)

# GoogleTest
if (ZSTD_CODEC_BUILD_TESTING)
    add_subdirectory(libs/external/googletest)
endif (ZSTD_CODEC_BUILD_TESTING)

add_subdirectory(libs/core)

# zstd-codec-nodejs
#message("CMAKE_JS_VERSION: ${CMAKE_JS_VERSION}")
#if (CMAKE_JS_VERSION)
    add_subdirectory(libs/nodejs)
#endif(CMAKE_JS_VERSION)