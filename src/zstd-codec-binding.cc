#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstdio>
#include <vector>
#include <emscripten/bind.h>

#include "zstd.h"


using u8 = std::uint8_t;
using usize = std::size_t;

template <typename T>
using Vec = std::vector<T>;

using namespace emscripten;

static const int ERR_UNKNOWN = -1;
static const int ERR_SIZE_TOO_LARGE = -2;


void dummy()
{
    FILE* fp = fopen("dummy", "rb");
    fclose(fp);
    printf("dummy");
}


// NOTE: ref: https://github.com/kripken/emscripten/issues/5519
template<typename T>
static size_t copy_to_vector(Vec<T> &dest, const val &src)
{
    const auto length = src["length"].as<unsigned int>();

    val memory = val::module_property("buffer");
    val memory_view = src["constructor"].new_(memory, reinterpret_cast<uintptr_t>(dest.data()), length);

    dest.reserve(length);
    memory_view.call<void>("set", src);

    return length;
}


template <typename T>
static Vec<T> from_js_typed_array(const val& src)
{
    Vec<T> dest;
    dest.resize(src["length"].as<size_t>());
    auto length = copy_to_vector(dest, src);

    return std::move(dest);
}


template <typename T>
class Resource
{
public:
    using Closer = std::function<void(T*)>;

    Resource(T* resource, Closer closer)
        : resource_(resource)
        , closer_(closer)
    {
    }

    ~Resource()
    {
        if (resource_ != nullptr) {
            closer_(resource_);
        }
    }

protected:
    T* resource() const {
        return resource_;
    }

private:
    T* resource_;
    Closer closer_;
};


class ZstdCStreamResource : public Resource<ZSTD_CStream>
{
public:
    ZstdCStreamResource()
        : Resource(ZSTD_createCStream(), ZSTD_freeCStream)
    {
    }

    ZSTD_CStream* stream() const
    {
        return resource();
    }
};


class ZstdDStreamResource : public Resource<ZSTD_DStream>
{
public:
    ZstdDStreamResource()
        : Resource(ZSTD_createDStream(), ZSTD_freeDStream)
    {
    }

    ZSTD_DStream* stream() const
    {
        return resource();
    }
};


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


// ---- bindings --------------------------------------------------------------


bool is_error(int zstd_rc)
{
    return zstd_rc < 0;
}


int compress_bound(size_t src_size)
{
    const auto rc = ZSTD_compressBound(src_size);
    if (ZSTD_isError(rc)) {
        return ERR_UNKNOWN;
    }
    else if (rc >= static_cast<size_t>(INT_MAX)) {
        return ERR_SIZE_TOO_LARGE;
    }

    return static_cast<int>(rc);
}


int content_size(val src)
{
    auto src_bytes = from_js_typed_array<u8>(src);
    const auto rc = ZSTD_getFrameContentSize(&src_bytes[0], src_bytes.size());
    if (ZSTD_isError(rc)) {
        return ERR_UNKNOWN;
    }
    else if (rc >= static_cast<size_t>(INT_MAX)) {
        return ERR_SIZE_TOO_LARGE;
    }

    return static_cast<int>(rc);
}


int compress(uintptr_t dest, size_t dest_size, val src, int compression_level)
{
    auto src_bytes = from_js_typed_array<u8>(src);
    const auto rc = ZSTD_compress(reinterpret_cast<void*>(dest), dest_size,
                                  &src_bytes[0], src_bytes.size(),
                                  compression_level);
    if (ZSTD_isError(rc)) {
        return ERR_UNKNOWN;
    }
    else if (rc >= static_cast<size_t>(INT_MAX)) {
        return ERR_SIZE_TOO_LARGE;
    }

    return static_cast<int>(rc);
}


bool compress_stream(val read_callback, val write_callback, int compression_level)
{
    ZstdCStreamResource cstream;
    if (cstream.stream() == nullptr) return false;

    Vec<u8> in_bytes(ZSTD_CStreamInSize());
    Vec<u8> out_bytes(ZSTD_CStreamOutSize());

    const auto init_rc = ZSTD_initCStream(cstream.stream(), compression_level);
    if (ZSTD_isError(init_rc)) return false;

    auto read_size = in_bytes.size();
    size_t read_done;
    while ((read_done = read_from_js_callback(read_callback, in_bytes, read_size))) {
        ZSTD_inBuffer in_buff { &in_bytes[0], read_done, 0 };
        while (in_buff.pos < in_buff.size) {
            ZSTD_outBuffer out_buff { &out_bytes[0], out_bytes.size(), 0 };
            read_size = ZSTD_compressStream(cstream.stream(), &out_buff, &in_buff);
            if (ZSTD_isError(read_size)) return false;

            write_to_js_callback(write_callback, out_bytes, out_buff.pos);
        }
    }

    ZSTD_outBuffer out_buff { &out_bytes[0], out_bytes.size(), 0 };
    const auto remaining = ZSTD_endStream(cstream.stream(), &out_buff);
    if (remaining > 0u) return false;
    write_to_js_callback(write_callback, out_bytes, out_buff.pos);

    return true;
}


int decompress(uintptr_t dest, size_t dest_size, val src)
{
    auto src_bytes = from_js_typed_array<u8>(src);
    const auto rc = ZSTD_decompress(reinterpret_cast<void*>(dest), dest_size,
                                    &src_bytes[0], src_bytes.size());
    if (ZSTD_isError(rc)) {
        return ERR_UNKNOWN;
    }
    else if (rc >= static_cast<size_t>(INT_MAX)) {
        return ERR_SIZE_TOO_LARGE;
    }

    return static_cast<int>(rc);
}


bool decompress_stream(val read_callback, val write_callback)
{
    ZstdDStreamResource dstream;
    if (dstream.stream() == nullptr) return false;

    Vec<u8> in_bytes(ZSTD_DStreamInSize());
    Vec<u8> out_bytes(ZSTD_DStreamOutSize());

    const auto init_rc = ZSTD_initDStream(dstream.stream());
    if (ZSTD_isError(init_rc)) return false;

    auto read_size = init_rc;
    auto read_done = init_rc;
    while ((read_done = read_from_js_callback(read_callback, in_bytes, read_size))) {
        ZSTD_inBuffer in_buff { &in_bytes[0], read_done, 0 };
        while (in_buff.pos < in_buff.size) {
            ZSTD_outBuffer out_buff { &out_bytes[0], out_bytes.size(), 0 };
            read_size = ZSTD_decompressStream(dstream.stream(), &out_buff, &in_buff);
            if (ZSTD_isError(read_size)) return false;

            write_to_js_callback(write_callback, out_bytes, out_buff.pos);
        }
    }

    return true;
}


EMSCRIPTEN_BINDINGS(zstd) {
    constant("ERR_UNKNOWN", ERR_UNKNOWN);
    constant("ERR_SIZE_TOO_LARGE", ERR_SIZE_TOO_LARGE);

    function("is_error", &is_error);
    function("compress_bound", &compress_bound);
    function("content_size", &content_size, allow_raw_pointers());
    function("compress", &compress, allow_raw_pointers());
    function("compress_stream", &compress_stream);
    function("decompress", &decompress, allow_raw_pointers());
    function("decompress_stream", &decompress_stream);
    function("dummy", &dummy);
}

