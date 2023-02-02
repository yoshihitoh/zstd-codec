#include <emscripten/bind.h>

#include "../../zstd-codec.h"
#include "../../zstd-dict.h"
#include "../../zstd-stream.h"


using namespace emscripten;


// stream bindings (declarations)

class ZstdCompressStreamBinding
{
public:
    ZstdCompressStreamBinding();
    ~ZstdCompressStreamBinding();

    bool Begin(int compression_level);
    bool BeginUsingDict(const ZstdCompressionDict& cdict);
    bool Transform(val chunk, val callback);
    bool Flush(val callback);
    bool End(val callback);

private:
    ZstdCompressStream  stream_;
};


class ZstdDecompressStreamBinding
{
public:
    ZstdDecompressStreamBinding();
    ~ZstdDecompressStreamBinding();

    bool Begin();
    bool BeginUsingDict(const ZstdDecompressionDict& ddict);
    bool Transform(val chunk, val callback);
    bool Flush(val callback);
    bool End(val callback);

private:
    ZstdDecompressStream    stream_;
};


// ==== IMPLEMENTATIONS =======================================================
//

static val heap_buffer()
{
    val heap = val::module_property("HEAPU8");
    return heap["buffer"];
}



// NOTE: ref: https://github.com/kripken/emscripten/issues/5519
template<typename T>
static size_t copy_to_vector(Vec<T>& dest, const val& src)
{
    const auto length = src["length"].as<unsigned int>();

    val memory = heap_buffer();
    val memory_view = src["constructor"].new_(memory, reinterpret_cast<uintptr_t>(dest.data()), length);

    dest.reserve(length);
    memory_view.call<void>("set", src);

    return length;
}


template <typename T>
static void resize_to_fit(Vec<T>& dest, const val& src)
{
    dest.resize(src["length"].as<size_t>());
}


template <typename T>
static Vec<T> from_js_typed_array(const val& src)
{
    Vec<T> dest;
    resize_to_fit(dest, src);
    auto length = copy_to_vector(dest, src);

    return std::move(dest);
}


static size_t read_from_js_callback(val callback, Vec<u8>& buffer, size_t read_size)
{
    const auto read_bytes = callback(read_size);
    const auto read_done = read_bytes["length"].as<unsigned>();
    copy_to_vector(buffer, read_bytes);

    return read_done;
}


static void write_to_js_callback(val callback, const Vec<u8>& buffer, size_t write_size)
{
    callback(val(typed_memory_view(write_size, &buffer[0])));
}


// ---- binding helper functions ----------------------------------------------

// NOTE: dummy implementation, to include FS module.
void Dummy()
{
    FILE* fp = fopen("dummy", "rb");
    fclose(fp);
    printf("dummy");
}


void CloneToVector(Vec<u8>& dest, val src)
{
    const auto length = src["length"].as<unsigned int>();
    dest.resize(length);

    val memory = heap_buffer();
    val memory_view = src["constructor"].new_(memory, reinterpret_cast<uintptr_t>(dest.data()), length);

    memory_view.call<void>("set", src);
}


val CloneAsTypedArray(const Vec<u8>& src)
{
    val heapu8 = val::module_property("HEAPU8");
    val src_buffer = heap_buffer();
    val src_view = heapu8["constructor"].new_(src_buffer, reinterpret_cast<uintptr_t>(&src[0]), src.size());

    val dest_buffer = src_buffer["constructor"].new_(src.size());
    val dest_view = heapu8["constructor"].new_(dest_buffer);

    dest_view.call<void>("set", src_view);
    return dest_view;
}


val ToTypedArrayView(const Vec<u8>& src)
{
    val memory = heap_buffer();
    val heapu8 = val::module_property("HEAPU8");
    val memory_view = heapu8["constructor"].new_(memory, reinterpret_cast<uintptr_t>(&src[0]), src.size());
    return memory_view;
}


// --- dictionary bindings (implementations) ----------------------------------


ZstdCompressionDict* CreateCompressionDict(val dict_bytes, int compression_level)
{
    return new ZstdCompressionDict(from_js_typed_array<u8>(dict_bytes), compression_level);
}


ZstdDecompressionDict* CreateDecompressionDict(val dict_bytes)
{
    return new ZstdDecompressionDict(from_js_typed_array<u8>(dict_bytes));
}


// ---- stream bindings (implementations) -------------------------------------

//
// ZstdCompressStreamBinding
//
///////////////////////////////////////////////////////////////////////////////

ZstdCompressStreamBinding::ZstdCompressStreamBinding()
    : stream_()
{
}


ZstdCompressStreamBinding::~ZstdCompressStreamBinding()
{
}


bool ZstdCompressStreamBinding::Begin(int compression_level)
{
    return stream_.Begin(compression_level);
}


bool ZstdCompressStreamBinding::BeginUsingDict(const ZstdCompressionDict& cdict)
{
    return stream_.Begin(cdict);
}


bool ZstdCompressStreamBinding::Transform(val chunk, val callback)
{
    // use local vector to ensure thread-safety
    Vec<u8> chunk_vec;
    CloneToVector(chunk_vec, chunk);

    return stream_.Transform(chunk_vec, [&callback](const Vec<u8>& compressed_vec) {
        val compressed = CloneAsTypedArray(compressed_vec);
        callback(compressed);
    });
}


bool ZstdCompressStreamBinding::Flush(val callback)
{
    return stream_.Flush([&callback](const Vec<u8>& compressed_vec) {
        val compressed = CloneAsTypedArray(compressed_vec);
        callback(compressed);
    });
}


bool ZstdCompressStreamBinding::End(val callback)
{
    return stream_.End([&callback](const Vec<u8>& compressed_vec) {
        val compressed = CloneAsTypedArray(compressed_vec);
        callback(compressed);
    });
}


//
// ZstdDecompressStreamBinding
//
///////////////////////////////////////////////////////////////////////////////

ZstdDecompressStreamBinding::ZstdDecompressStreamBinding()
    : stream_()
{
}


ZstdDecompressStreamBinding::~ZstdDecompressStreamBinding()
{
}


bool ZstdDecompressStreamBinding::Begin()
{
    return stream_.Begin();
}


bool ZstdDecompressStreamBinding::BeginUsingDict(const ZstdDecompressionDict& ddict)
{
    return stream_.Begin(ddict);
}


bool ZstdDecompressStreamBinding::Transform(val chunk, val callback)
{
    // use local vector to ensure thread-safety
    Vec<u8> chunk_vec;
    CloneToVector(chunk_vec, chunk);

    return stream_.Transform(chunk_vec, [&callback](const Vec<u8>& decompressed_vec) {
        val decompressed = CloneAsTypedArray(decompressed_vec);
        callback(decompressed);
    });
}


bool ZstdDecompressStreamBinding::Flush(val callback)
{
    return stream_.Flush([&callback](const Vec<u8>& decompressed_vec) {
        val decompressed = CloneAsTypedArray(decompressed_vec);
        callback(decompressed);
    });
}


bool ZstdDecompressStreamBinding::End(val callback)
{
    return stream_.End([&callback](const Vec<u8>& decompressed_vec) {
        val decompressed = CloneAsTypedArray(decompressed_vec);
        callback(decompressed);
    });
}


// ---- bindings --------------------------------------------------------------

EMSCRIPTEN_BINDINGS(zstd) {
    register_vector<u8>("VectorU8");

    function("dummy", &Dummy);
    function("cloneToVector", &CloneToVector);
    function("cloneAsTypedArray", &CloneAsTypedArray);
    function("toTypedArrayView", &ToTypedArrayView);

    class_<ZstdCompressionDict>("ZstdCompressionDict");
    function("createCompressionDict", &CreateCompressionDict, allow_raw_pointers());

    class_<ZstdDecompressionDict>("ZstdDecompressionDict");
    function("createDecompressionDict", &CreateDecompressionDict, allow_raw_pointers());

    class_<ZstdCodec>("ZstdCodec")
        .constructor<>()
        .function("compressBound", &ZstdCodec::CompressBound)
        .function("contentSize", &ZstdCodec::ContentSize)
        .function("compress", &ZstdCodec::Compress)
        .function("decompress", &ZstdCodec::Decompress)
        .function("compressUsingDict", &ZstdCodec::CompressUsingDict)
        .function("decompressUsingDict", &ZstdCodec::DecompressUsingDict)
        ;

    class_<ZstdCompressStreamBinding>("ZstdCompressStreamBinding")
        .constructor<>()
        .function("begin", &ZstdCompressStreamBinding::Begin)
        .function("beginUsingDict", &ZstdCompressStreamBinding::BeginUsingDict)
        .function("transform", &ZstdCompressStreamBinding::Transform)
        .function("flush", &ZstdCompressStreamBinding::Flush)
        .function("end", &ZstdCompressStreamBinding::End)
        ;

    class_<ZstdDecompressStreamBinding>("ZstdDecompressStreamBinding")
        .constructor<>()
        .function("begin", &ZstdDecompressStreamBinding::Begin)
        .function("beginUsingDict", &ZstdDecompressStreamBinding::BeginUsingDict)
        .function("transform", &ZstdDecompressStreamBinding::Transform)
        .function("flush", &ZstdDecompressStreamBinding::Flush)
        .function("end", &ZstdDecompressStreamBinding::End)
        ;
}

