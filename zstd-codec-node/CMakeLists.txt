
LIST(APPEND CMAKE_PROGRAM_PATH  $ENV{NODE_PREFIX} ...)

function(SET_CMAKEJS_PROPERTIES)
        # you can confirm properties by executing following commands
        # - debug: cmake-js print-configure --debug
        # - release: cmake-js print-configure
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

add_library(zstd-codec-node SHARED "src/binding.cc" ${CMAKE_JS_SRC})
target_compile_definitions(zstd-codec-node PRIVATE
        -DNAPI_VERSION=3
        -DNAPI_DISABLE_CPP_EXCEPTIONS
        )
target_include_directories(zstd-codec-node PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR})
target_link_libraries(zstd-codec-node zstd-codec ${CMAKE_JS_LIB})
set_target_properties(zstd-codec-node PROPERTIES PREFIX "" SUFFIX ".node")
