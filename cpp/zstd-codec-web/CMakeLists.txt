
add_executable(zstd-codec-web "src/binding.cc")
target_link_libraries(zstd-codec-web zstd-codec)
target_link_options(zstd-codec-web PRIVATE
        "--bind"
        "-flto"
#        "-sMODULARIZE=1"
        "-sNODEJS_CATCH_EXIT=0"
        "-sNODEJS_CATCH_REJECTION=0"
        "-sDEMANGLE_SUPPORT=1"
        )

if (NOT EMSCRIPTEN)
    # For development purpose. It doesn't work, can't build a binary, but convenient for writing code.
    target_include_directories(zstd-codec-web PRIVATE
            "$ENV{EMSDK}/upstream/emscripten/cache/sysroot/include"
            )
endif()
