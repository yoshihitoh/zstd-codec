cmake_minimum_required(VERSION 3.15...3.30)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
include(Cxx)
include(ZstdCodec)

project(zstd_codec_nodejs VERSION ${ZSTD_CODEC_VERSION}
                          LANGUAGES CXX)

add_definitions(-DNAPI_VERSION=9)
include_directories(${CMAKE_JS_INC})

add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME}
    PRIVATE
        FILE_SET zstd_codec_core_headers
        TYPE HEADERS
        BASE_DIRS include
        FILES
            include/zstd-codec/nodejs/binding.h
)
target_sources(${PROJECT_NAME}
    PRIVATE
        src/binding.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC ./include)
message("CMAKE_JS_VERSION: ${CMAKE_JS_VERSION}")
if (NOT CMAKE_JS_VERSION)
    message("cmake-js for local build!!")
    target_include_directories(${PROJECT_NAME}
                               PUBLIC
                               ${PROJECT_SOURCE_DIR}/../../node_modules/node-addon-api
                               ${PROJECT_SOURCE_DIR}/../../node_modules/node-api-headers/include)
endif (NOT CMAKE_JS_VERSION)

target_link_libraries(${PROJECT_NAME} PUBLIC zstd_codec_core)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
if (CMAKE_JS_SRC)
    target_sources(${PROJECT_NAME} ${CMAKE_JS_SRC})
endif (CMAKE_JS_SRC)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB})

option(ZSTD_CODEC_NODEJS_BUILD_TESTING "set ON to enable ${PROJECT_NAME} tests" ON)
if (ZSTD_CODEC_NODEJS_BUILD_TESTING)
    add_executable(${PROJECT_NAME}_test)
    target_sources(${PROJECT_NAME}_test
        PRIVATE
            test/test_binding.cpp
    )
    target_link_libraries(${PROJECT_NAME}_test PRIVATE gtest_main ${PROJECT_NAME})
endif (ZSTD_CODEC_NODEJS_BUILD_TESTING)