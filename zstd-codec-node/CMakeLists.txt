
LIST(APPEND CMAKE_PROGRAM_PATH  $ENV{NODE_PREFIX} ...)

function(SET_CMAKEJS_PROPERTIES)
        # you can confirm properties by executing following commands
        # - debug: cmake-js print-configure --debug
        # - release: cmake-js print-configure
        set(CMAKE_JS_VERSION "7.2.1" PARENT_SCOPE)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "/Users/hirakawa.yoshihito/workspace/private/github/zstd-codec/build/Debug" PARENT_SCOPE)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" PARENT_SCOPE)
        set(CMAKE_JS_INC "/Users/hirakawa.yoshihito/.cmake-js/node-x64/v14.18.2/include/node" PARENT_SCOPE)
        set(CMAKE_JS_SRC "" PARENT_SCOPE)
        set(CMAKE_OSX_ARCHITECTURES "x86_64" PARENT_SCOPE)
        set(CMAKE_JS_LIB "" PARENT_SCOPE)
        set(CMAKE_CXX_FLAGS "-D_DARWIN_USE_64_BIT_INODE=1 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DBUILDING_NODE_EXTENSION" PARENT_SCOPE)
        set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup" PARENT_SCOPE)
endfunction(SET_CMAKEJS_PROPERTIES)


if (NOT CMAKE_JS_VERSION)
        set_cmakejs_properties()
endif ()

execute_process(
        COMMAND $ENV{NODE_PREFIX}node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

#execute_process(
#        COMMAND $ENV{NODE_PREFIX}node -p "require('node-addon-api').include"
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#        OUTPUT_VARIABLE NODE_ADDON_API_DIR
#        )
#string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
#string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

set(NODE_BIDING_SOURCES "src/binding.h"
                        "src/binding.cc"
                        "src/compress.cc")

add_library(zstd-codec-node SHARED  ${CMAKE_JS_SRC} ${NODE_BIDING_SOURCES})
target_compile_definitions(zstd-codec-node PRIVATE
        -DNAPI_VERSION=3
        -DNAPI_DISABLE_CPP_EXCEPTIONS
        )
target_include_directories(zstd-codec-node PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR})
target_link_libraries(zstd-codec-node zstd-codec-core ${CMAKE_JS_LIB})
set_target_properties(zstd-codec-node PROPERTIES PREFIX "" SUFFIX ".node")
