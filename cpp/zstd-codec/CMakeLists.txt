set(ZSTD_CODEC_HEADERS
        include/zstd-codec/zstd-codec.h
        include/zstd-codec/compress.h
        include/zstd-codec/decompress.h
        )

set(ZSTD_CODEC_SOURCES
        src/zstd-codec.cc
        src/compress.cc
        src/decompress.cc
        )

# add zstd-codec
add_library(zstd-codec STATIC   ${ZSTD_CODEC_HEADERS}
                                ${ZSTD_CODEC_SOURCES})
target_include_directories(zstd-codec PUBLIC include
                                             ${EXPECTED_INCLUDE_DIR}
                                             ${OPTIONAL_INCLUDE_DIR}
                                             ${ZSTD_INCLUDE_DIR}
                                             )
target_link_libraries(zstd-codec PUBLIC libzstd_static fmt::fmt-header-only)
