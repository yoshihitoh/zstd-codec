newoption {
    trigger = "with-emscripten",
    description = "Generate Makefiles for Emscripten platform",
}


newoption {
    trigger = "with-zstd-dir",
    description = "Absolute path to zstd directory",
    value = "/full/path/to/zstd",
}


if premake.modules.gmake2 then
    premake.override(premake.modules.gmake2.cpp, "linkCmd", function(base, cfg, toolset)
        local is_emscripten = _OPTIONS["with-emscripten"]
        local is_staticlib = cfg.kind == premake.STATICLIB
        local is_non_universal = not(cfg.architecture == premake.UNIVERSAL)
        if is_emscripten and is_staticlib and is_non_universal then
            premake.outln('LINKCMD = $(AR) rcs "$@" $(OBJECTS)')
        else
            base(cfg, toolset)
        end
    end)
end


function zstd_root_dir()
    if _OPTIONS["with-zstd-dir"] then
        return _OPTIONS["with-zstd-dir"]
    else
        return './zstd'
    end
end


function zstd_lib_dir()
    return string.format("%s/lib", zstd_root_dir())
end


function zstd_lib_name()
    if os.istarget("macosx") then
        return 'libzstd.dylib'
    else
        return 'libzstd.so'
    end
end


workspace "zstd-codec"
    configurations {"Debug", "Release"}

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "Full"

    filter "action:gmake*"
        buildoptions {"-std=c++1z"}

    filter { "action:gmake*", "options:with-emscripten" }
        location "./build-emscripten"

    filter { "action:gmake*", "options:not with-emscripten" }
        location "./build-gmake"


externalproject "zstd"
    location (zstd_root_dir())
    kind "StaticLib"
    language "C"
    targetdir (zstd_lib_dir())

    filter "options:with-emscripten"
        targetextension ".bc"


project "zstd-codec"
    language "C++"
    kind "StaticLib"

    dependson "zstd"

    defines {
        "ZSTD_STATIC_LINKING_ONLY",
    }

    includedirs {
        zstd_lib_dir(),
    }

    files {
        "src/**.h",
        "src/**.hpp",
        "src/**.c",
        "src/**.cc",
    }

    libdirs {
        zstd_lib_dir(),
    }

    removefiles {
        "src/binding/**"
    }

    filter "options:with-emscripten"
        prebuildcommands {
            string.format("{COPY} %s/%s %s/libzstd.bc", zstd_lib_dir(), zstd_lib_name(), zstd_lib_dir()),
        }


project "test-zstd-codec"
    kind "ConsoleApp"
    language "C++"
    targetdir "%{wks.location}/bin/%{cfg.buildcfg}"

    includedirs {
        "zstd/lib",
        "src",
    }

    files {
        "test/**.h",
        "test/**.hpp",
        "test/**.c",
        "test/**.cc",
    }

    links {
        "zstd",
        "zstd-codec",
    }


project "zstd-codec-binding"
    kind "SharedLib"
    language "C++"
    targetdir "%{wks.location}/bin/%{cfg.buildcfg}"

    -- NOTE: avoid build bindings on non-Emscripten platform
    filter "options:with-emscripten"
        targetprefix    ""
        targetextension ".js"

        includedirs {
            "zstd/lib",
        }

        files {
            "src/binding/emscripten/**.cc",
        }

        links {
            "zstd-codec",
            "zstd",
        }

        linkoptions {
            "--bind",
            "--memory-init-file 0",
            "-s DEMANGLE_SUPPORT=1",
            "-s 'EXTRA_EXPORTED_RUNTIME_METHODS=[\"FS\"]'",
            "-s MODULARIZE=1",
            "-s WASM=0",
        }

    filter {"options:with-emscripten", "configurations:Release"}
        linkoptions {
            "-O2",
            "-s USE_CLOSURE_COMPILER=1",
        }

    -- NOTE: don't know how to exclude this project on other platofrms.
    filter "options:not with-emscripten"
        files {
            "src/binding/others/**.cc",
        }

project "zstd-codec-binding-wasm"
    kind "SharedLib"
    language "C++"
    targetdir "%{wks.location}/bin/%{cfg.buildcfg}"

    -- NOTE: avoid build bindings on non-Emscripten platform
    filter "options:with-emscripten"
        targetprefix    ""
        targetextension ".js"

        includedirs {
            "zstd/lib",
        }

        files {
            "src/binding/emscripten/**.cc",
        }

        links {
            "zstd-codec",
            "zstd",
        }

        linkoptions {
            "--bind",
            "--memory-init-file 0",
            "-s DEMANGLE_SUPPORT=1",
            "-s 'EXTRA_EXPORTED_RUNTIME_METHODS=[\"FS\"]'",
            "-s MODULARIZE=1",
            "-s WASM=1",
            "-s SINGLE_FILE=1",
            "-s BINARYEN_ASYNC_COMPILATION=0",
        }

    filter {"options:with-emscripten", "configurations:Release"}
        linkoptions {
            "-O2",
            "-s USE_CLOSURE_COMPILER=1",
        }

    -- NOTE: don't know how to exclude this project on other platofrms.
    filter "options:not with-emscripten"
        files {
            "src/binding/others/**.cc",
        }
