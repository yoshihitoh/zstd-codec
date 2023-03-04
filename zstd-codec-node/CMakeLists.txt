function(PARSE_CMAKEJS_PROPERTIES)
        function(GET_VARIABLE INPUT_STRING VARIABLE_TO_SELECT OUTPUT_VARIABLE)
                set(SEARCH_STRING "${VARIABLE_TO_SELECT}=\"")
                string(LENGTH "${SEARCH_STRING}" SEARCH_STRING_LENGTH)
                string(LENGTH "${INPUT_STRING}" INPUT_STRING_LENGTH)

                string(FIND "${INPUT_STRING}" "${VARIABLE_TO_SELECT}=\"" SEARCH_STRING_INDEX)

                math(EXPR SEARCH_STRING_INDEX "${SEARCH_STRING_INDEX}+${SEARCH_STRING_LENGTH}")

                string(SUBSTRING "${INPUT_STRING}" ${SEARCH_STRING_INDEX} ${INPUT_STRING_LENGTH} AFTER_SEARCH_STRING)
                string(FIND "${AFTER_SEARCH_STRING}" "\"" QUOTE_INDEX)
                string(SUBSTRING "${AFTER_SEARCH_STRING}" "0" "${QUOTE_INDEX}" RESULT_STRING)
                set("${OUTPUT_VARIABLE}" "${RESULT_STRING}" PARENT_SCOPE)
        endfunction(GET_VARIABLE)

        string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
        if (CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
                exec_program($ENV{NODE_PREFIX}npx cmake-js ${CMAKE_CURRENT_SOURCE_DIR}
                        ARGS print-configure --debug
                        OUTPUT_VARIABLE CMAKE_JS_OUTPUT
                        )
        else()
                exec_program($ENV{NODE_PREFIX}npx cmake-js ${CMAKE_CURRENT_SOURCE_DIR}
                        ARGS print-configure
                        OUTPUT_VARIABLE CMAKE_JS_OUTPUT
                        )
        endif ()

        get_variable("${CMAKE_JS_OUTPUT}" "CMAKE_JS_INC" CMAKE_JS_INC)
        set(CMAKE_JS_INC "${CMAKE_JS_INC}" PARENT_SCOPE)

        get_variable("${CMAKE_JS_OUTPUT}" "CMAKE_LIBRARY_OUTPUT_DIRECTORY" CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}" PARENT_SCOPE)
endfunction(PARSE_CMAKEJS_PROPERTIES)


if (NOT CMAKE_JS_VERSION)
        parse_cmakejs_properties()
endif ()

execute_process(
        COMMAND $ENV{NODE_PREFIX}node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )

string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
message(STATUS "NODE_ADDON_API_DIR: ${NODE_ADDON_API_DIR}")

add_library(zstd-codec-node SHARED "src/binding.cc" ${CMAKE_JS_SRC})
target_compile_definitions(zstd-codec-node PRIVATE -DNAPI_VERSION=3)
target_include_directories(zstd-codec-node PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR})
target_link_libraries(zstd-codec-node zstd-codec ${CMAKE_JS_LIB})
set_target_properties(zstd-codec-node PROPERTIES PREFIX "" SUFFIX ".node")
