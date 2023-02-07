cmake_minimum_required(VERSION 3.10)

project(zstd-codec VERSION 0.2.0
                   DESCRIPTION "Zstandard codec for Node.js and Web, powered by Emscripten."
                   LANGUAGES CXX)

# common settings
set(CMAKE_CXX_STANDARD 14)

# enable tl/optional and tl/expected
set(EXPECTED_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/expected/include")
set(OPTIONAL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/optional/include")

# add zstd library as a library
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "Disable zstd shared library to enforce using static library")
add_subdirectory(lib/zstd/build/cmake)
get_directory_property(ZSTD_LIBRARY_DIR DIRECTORY lib/zstd/build/cmake DEFINITION LIBRARY_DIR)
set(ZSTD_INCLUDE_DIR "${ZSTD_LIBRARY_DIR}"
                     "${ZSTD_LIBRARY_DIR}/common"
                     )

# add fmtlib/fmt
add_subdirectory(lib/fmt EXCLUDE_FROM_ALL)

# add zstd-codec library (common)
set_property(GLOBAL PROPERTY expected_include_dir lib/expected/include)
add_subdirectory(zstd-codec)
