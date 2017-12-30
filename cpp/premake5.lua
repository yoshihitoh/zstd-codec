newoption {
    trigger = "with-emscripten",
    description = "Generate Makefiles for Emscripten platform"
}


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

workspace "zstd-codec"
    configurations {"Debug", "Release"}

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "action:gmake*"
        buildoptions {"-std=c++1z"}

    filter { "action:gmake*", "options:with-emscripten" }
        location "./build-emscripten"

    filter { "action:gmake*", "options:not with-emscripten" }
        location "./build-gmake"


externalproject "zstd"
    location "./zstd"
    kind "SharedLib"
    language "C"
    targetdir "./zstd/lib"

    filter "options:with-emscripten"
        targetextension ".bc"


project "zstd-codec"
    language "C++"

    dependson "zstd"

    includedirs {
        "zstd/lib",
    }

    files {
        "src/**.h",
        "src/**.hpp",
        "src/**.c",
        "src/**.cc",
    }

    libdirs {
        "zstd/lib",
    }

    removefiles {
        "src/binding/**"
    }

    filter "options:with-emscripten"
        kind "StaticLib"
        prebuildcommands {
            "{COPY} %{wks.location}/../zstd/lib/libzstd.dylib %{wks.location}/../zstd/lib/libzstd.bc",
        }

    filter "options:not with-emscripten"
        kind "SharedLib"

        links {
            "zstd"
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
            "-s USE_CLOSURE_COMPILER=1",
        }

    -- NOTE: don't know how to exclude this project on other platofrms.
    filter "options:not with-emscripten"
        files {
            "src/binding/others/**.cc",
        }
