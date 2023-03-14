set(ZSTD_CODEC_HEADERS
        include/zstd-codec/zstd-codec.h
        include/zstd-codec/compress.h
        include/zstd-codec/decompress.h
        include/zstd-codec/binding/bindable.h
        include/zstd-codec/binding/binder.h
        include/zstd-codec/binding/compress.h
        include/zstd-codec/binding/decompress.h
        include/zstd-codec/binding/error.h
        )

set(ZSTD_CODEC_SOURCES
        src/zstd-codec.cc
        src/compress.cc
        src/decompress.cc
        )

# add zstd-codec
add_library(zstd-codec-core STATIC  ${ZSTD_CODEC_HEADERS}
                                    ${ZSTD_CODEC_SOURCES})
target_include_directories(zstd-codec-core PUBLIC include
                                             ${EXPECTED_INCLUDE_DIR}
                                             ${OPTIONAL_INCLUDE_DIR}
                                             ${ZSTD_INCLUDE_DIR}
                                             )
target_link_libraries(zstd-codec-core PUBLIC libzstd_static fmt::fmt-header-only)

add_executable(zstd-codec-sandbox src/main.cc)
target_link_libraries(zstd-codec-sandbox zstd-codec-core)

if (EMSCRIPTEN)
    target_link_options(zstd-codec-sandbox PRIVATE "-sNODERAWFS=1" "-sINITIAL_MEMORY=314572800" "-flto")
endif()
