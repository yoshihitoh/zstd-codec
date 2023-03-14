#include <cstdint>
#include <functional>
#include <vector>

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

#include "fmt/format.h"

#include "zstd-codec/compress.h"
#include "zstd-codec/binding/compress.h"
#include "zstd-codec/binding/decompress.h"

using namespace emscripten;

static val heap() {
    return val::module_property("HEAPU8");
}

static val heapBuffer() {
    return heap()["buffer"];
}

struct EmptyContext {};

struct EmscriptenContext {
    using WireContext = EmptyContext;
};

template <Contextual C>
struct EmscriptenBytesBinding {
    using WireType = val;
    using ValueType = std::vector<uint8_t>;

    std::vector<uint8_t> buffer_;

    ValueType fromWireType(typename C::WireContext, WireType wire) {
        const auto length = wire["length"].as<unsigned int>();
        buffer_.resize(length);

        auto memory = heapBuffer();
        auto memory_view = wire["constructor"].new_(memory, reinterpret_cast<uintptr_t>(buffer_.data()), length);
        memory_view.call<void>("set", wire);

        return buffer_;
    }

    WireType intoWireType(typename C::WireContext, ValueType value) {
        auto heapu8 = heap();
        auto src_buffer = heapBuffer();
        auto src_view = heapu8["constructor"].new_(src_buffer, reinterpret_cast<uintptr_t>(&value[0]), value.size());

        auto dest_buffer = src_buffer["constructor"].new_(value.size());
        auto dest_view = heapu8["constructor"].new_(dest_buffer);

        dest_view.call<void>("set", src_view);
        return dest_view;
    }
};

template <Contextual C>
struct EmscriptenBytesCallbackBinding {
    using WireType = val;
    using ValueType = std::function<void(const std::vector<uint8_t>&)>;

    EmscriptenBytesBinding<C> bytes_;

    ValueType fromWireType(typename C::WireContext, WireType wire) {
        return [this, wire](const std::vector<uint8_t>& data) {
            auto wired_data = bytes_.intoWireType({}, data);
            wire(wired_data);
        };
    }
};

EM_JS(void, zstdCodecThrowError, (const char* code, const char* message), {
    // FIXME: replace built-in Error with custom error.
    throw new Error(`code=${UTF8ToString(code)}, message=${UTF8ToString(message)}`);
});

template <Contextual C>
struct EmscriptenErrorThrowable {
    void throwError(typename C::WireContext, const ZstdCodecBindingError& error) {
        auto code = error.code ? fmt::format("Some({})", *error.code) : fmt::format("None");
        zstdCodecThrowError(code.c_str(), error.message.c_str());
    }
};

using EmscriptenZstdCompressContextBinding = ZstdCompressContextBinding<
        EmscriptenContext,
        EmscriptenBytesBinding<EmscriptenContext>,
        EmscriptenBytesCallbackBinding<EmscriptenContext>,
        EmscriptenErrorThrowable<EmscriptenContext>
        >;
using EmscriptenZstdCompressStreamBinding = ZstdCompressStreamBinding<
        EmscriptenContext,
        EmscriptenBytesBinding<EmscriptenContext>,
        EmscriptenBytesCallbackBinding<EmscriptenContext>,
        EmscriptenErrorThrowable<EmscriptenContext>
        >;

using EmscriptenZstdDecompressContextBinding = ZstdDecompressContextBinding<
        EmscriptenContext,
        EmscriptenBytesBinding<EmscriptenContext>,
        EmscriptenBytesCallbackBinding<EmscriptenContext>,
        EmscriptenErrorThrowable<EmscriptenContext>
        >;
using EmscriptenZstdDecompressStreamBinding = ZstdDecompressStreamBinding<
        EmscriptenContext,
        EmscriptenBytesBinding<EmscriptenContext>,
        EmscriptenBytesCallbackBinding<EmscriptenContext>,
        EmscriptenErrorThrowable<EmscriptenContext>
        >;

using EmscriptenZstdCodecBinder = ZstdCodecBinder<
        EmscriptenContext,
        EmscriptenBytesBinding<EmscriptenContext>,
        EmscriptenBytesCallbackBinding<EmscriptenContext>,
        EmscriptenErrorThrowable<EmscriptenContext>
        >;

static EmscriptenZstdCodecBinder emscriptenBinder() {
    return EmscriptenZstdCodecBinder({}, {}, {});
}

static std::unique_ptr<EmscriptenZstdCompressContextBinding> compressContextBinding() {
    return EmscriptenZstdCompressContextBinding::create({}, emscriptenBinder());
}

static std::unique_ptr<EmscriptenZstdCompressStreamBinding> compressStreamBinding(std::unique_ptr<EmscriptenZstdCompressContextBinding> context) {
    return EmscriptenZstdCompressStreamBinding ::create({}, emscriptenBinder(), context->takeContext());
}

static std::unique_ptr<EmscriptenZstdDecompressContextBinding> decompressContextBinding() {
    return EmscriptenZstdDecompressContextBinding::create({}, emscriptenBinder());
}

static std::unique_ptr<EmscriptenZstdDecompressStreamBinding> decompressStreamBinding(std::unique_ptr<EmscriptenZstdDecompressContextBinding> context) {
    return EmscriptenZstdDecompressStreamBinding ::create({}, emscriptenBinder(), context->takeContext());
}

EMSCRIPTEN_BINDINGS(zstdCodec) {
    // compress context
    class_<EmscriptenZstdCompressContextBinding>("ZstdCompressContextBinding")
            .constructor(&compressContextBinding)
            .function("takeContext", &EmscriptenZstdCompressContextBinding::takeContext)
            .function("resetSession", &EmscriptenZstdCompressContextBinding::resetSession)
            .function("clearDictionary", &EmscriptenZstdCompressContextBinding::clearDictionary)
            .function("setCompressionLevel", &EmscriptenZstdCompressContextBinding::setCompressionLevel)
            .function("setChecksum", &EmscriptenZstdCompressContextBinding::setChecksum)
            .function("setOriginalSize", &EmscriptenZstdCompressContextBinding::setOriginalSize)
            ;

    // compress stream
    class_<EmscriptenZstdCompressStreamBinding>("ZstdCompressStreamBinding")
            .constructor(&compressStreamBinding)
            .function("compress", &EmscriptenZstdCompressStreamBinding::compress)
            .function("flush", &EmscriptenZstdCompressStreamBinding::flush)
            .function("complete", &EmscriptenZstdCompressStreamBinding::complete)
            .function("close", &EmscriptenZstdCompressStreamBinding::close)
            ;

    // decompress context
    class_<EmscriptenZstdDecompressContextBinding>("ZstdDecompressContextBinding")
            .constructor(&decompressContextBinding)
            .function("takeContext", &EmscriptenZstdDecompressContextBinding::takeContext)
            .function("resetSession", &EmscriptenZstdDecompressContextBinding::resetSession)
            ;

    // decompress stream
    class_<EmscriptenZstdDecompressStreamBinding>("ZstdDecompressStreamBinding")
            .constructor(&decompressStreamBinding)
            .function("decompress", &EmscriptenZstdDecompressStreamBinding::decompress)
            .function("close", &EmscriptenZstdDecompressStreamBinding::close)
            ;
}
