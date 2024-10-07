cmake_minimum_required(VERSION 3.15...3.30)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
include(Cxx)
include(ZstdCodec)

project(zstd_codec_core VERSION ${ZSTD_CODEC_VERSION}
                        LANGUAGES CXX)

configure_file (
    "${PROJECT_SOURCE_DIR}/include/zstd-codec/core/version.h.in"
    "${PROJECT_SOURCE_DIR}/include/zstd-codec/core/version.h"
)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
        FILE_SET zstd_codec_core_headers
        TYPE HEADERS
        BASE_DIRS include
        FILES
            include/zstd-codec/core/types.h
            include/zstd-codec/core/simple.h
            include/zstd-codec/core/metadata.h
)
target_sources(${PROJECT_NAME}
    PRIVATE
        src/simple.cpp
        src/metadata.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC ./include)
target_link_libraries(${PROJECT_NAME} INTERFACE fmt::fmt-header-only tl::optional tl::expected libzstd_static)

option(ZSTD_CODEC_CORE_BUILD_TESTING "set OFF to suppress ${PROJECT_NAME} tests" ON)
if (ZSTD_CODEC_CORE_BUILD_TESTING)
    add_executable(${PROJECT_NAME}_test)
    target_sources(${PROJECT_NAME}_test
            PRIVATE
            test/test_metadata.cpp
    )
    target_link_libraries(${PROJECT_NAME}_test PRIVATE gtest_main ${PROJECT_NAME})
endif(ZSTD_CODEC_CORE_BUILD_TESTING)