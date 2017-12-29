#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <memory>
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
static size_t copy_to_vector(Vec<T>& dest, const val& src)
{
    const auto length = src["length"].as<unsigned int>();

    val memory = val::module_property("buffer");
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


// ---- basic apis ------------------------------------------------------------


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


// ---- streaming apis (declarations) -----------------------------------------


class ZstdCompressStream
{
public:
    ZstdCompressStream();
    ~ZstdCompressStream();

    bool Begin(int compression_level);
    bool Transform(val chunk, val write_callback);
    bool Flush(val write_callback);
    bool End(val write_callback);

private:
    using CStreamPtr = std::unique_ptr<ZSTD_CStream, decltype(&ZSTD_freeCStream)>;

    bool HasStream() const;
    bool Compress(val write_callback);

    CStreamPtr  stream_;
    size_t      next_read_size_;
    Vec<u8>     src_bytes_;
    Vec<u8>     dest_bytes_;
    Vec<u8>     copy_bytes_;
};


class ZstdDecompressStream
{
public:
    ZstdDecompressStream();
    ~ZstdDecompressStream();

    bool Begin();
    bool Transform(val chunk, val write_callback);
    bool Flush(val write_callback);
    bool End(val write_callback);

private:
    using DStreamPtr = std::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)>;

    bool HasStream() const;
    bool Decompress(val write_callback);

    DStreamPtr  stream_;
    size_t      next_read_size_;
    Vec<u8>     src_bytes_;
    Vec<u8>     dest_bytes_;
    Vec<u8>     copy_bytes_;
};


// ---- streaming apis (implementations) --------------------------------------

//
// ZstdCompressStream
//
///////////////////////////////////////////////////////////////////////////////


ZstdCompressStream::ZstdCompressStream()
    : stream_(nullptr, ZSTD_freeCStream)
    , next_read_size_()
    , src_bytes_()
    , dest_bytes_()
    , copy_bytes_()
{
}


ZstdCompressStream::~ZstdCompressStream()
{
}


bool ZstdCompressStream::Begin(int compression_level)
{
    if (HasStream()) return true;

    CStreamPtr stream(ZSTD_createCStream(), ZSTD_freeCStream);
    if (stream == nullptr) return false;

    const auto init_rc = ZSTD_initCStream(stream.get(), compression_level);
    if (ZSTD_isError(init_rc)) return false;

    stream_ = std::move(stream);
    src_bytes_.reserve(ZSTD_CStreamInSize());
    dest_bytes_.resize(ZSTD_CStreamOutSize());  // resize
    copy_bytes_.reserve(src_bytes_.size());
    next_read_size_ = src_bytes_.size();

    return true;
}


bool ZstdCompressStream::Transform(val chunk, val write_callback)
{
    if (!HasStream()) return false;

    // copy from TypedArray(js) to Vector(c++)
    resize_to_fit(copy_bytes_, chunk);
    copy_to_vector(copy_bytes_, chunk);

    // append src bytes
    std::copy(std::begin(copy_bytes_),
              std::end(copy_bytes_), std::back_inserter(src_bytes_));

    // not enough
    if (src_bytes_.size() + copy_bytes_.size() < next_read_size_) {
        return true;
    }

    // enough
    return Compress(write_callback);
}


bool ZstdCompressStream::Flush(val write_callback)
{
    return Compress(write_callback);
}


bool ZstdCompressStream::End(val write_callback)
{
    if (!HasStream()) return true;

    auto success = true;
    if (!src_bytes_.empty()) {
        success = Compress(write_callback);
    }

    if (success) {
        ZSTD_outBuffer output { &dest_bytes_[0], dest_bytes_.size(), 0 };
        const auto remaining = ZSTD_endStream(stream_.get(), &output);
        if (remaining > 0u) return false;
        write_to_js_callback(write_callback, dest_bytes_, output.pos);
    }

    stream_.reset();
    return success;
}


bool ZstdCompressStream::HasStream() const
{
    return stream_ != nullptr;
}


bool ZstdCompressStream::Compress(val write_callback)
{
    if (src_bytes_.empty()) return true;

    ZSTD_inBuffer input { &src_bytes_[0], src_bytes_.size(), 0 };
    while (input.pos < input.size) {
        ZSTD_outBuffer output { &dest_bytes_[0], dest_bytes_.size(), 0};
        next_read_size_ = ZSTD_compressStream(stream_.get(), &output, &input);
        if (ZSTD_isError(next_read_size_)) return false;

        write_to_js_callback(write_callback, dest_bytes_, output.pos);
    }

    src_bytes_.clear();
    return true;
}


//
// ZstdDecompressStream
//
///////////////////////////////////////////////////////////////////////////////

ZstdDecompressStream::ZstdDecompressStream()
    : stream_(nullptr, ZSTD_freeDStream)
    , next_read_size_()
    , src_bytes_()
    , dest_bytes_()
{
}


ZstdDecompressStream::~ZstdDecompressStream()
{
}


bool ZstdDecompressStream::Begin()
{
    if (HasStream()) return true;

    DStreamPtr stream(ZSTD_createDStream(), ZSTD_freeDStream);
    if (stream == nullptr) return false;

    const auto init_rc = ZSTD_initDStream(stream.get());
    if (ZSTD_isError(init_rc)) return false;

    stream_ = std::move(stream);
    src_bytes_.reserve(ZSTD_DStreamInSize());
    dest_bytes_.resize(ZSTD_DStreamOutSize());  // resize
    copy_bytes_.reserve(src_bytes_.size());
    next_read_size_ = init_rc;

    return true;
}


bool ZstdDecompressStream::Transform(val chunk, val write_callback)
{
    if (!HasStream()) return false;

    // copy from TypedArray(js) to Vector(c++)
    resize_to_fit(copy_bytes_, chunk);
    copy_to_vector(copy_bytes_, chunk);

    // append src bytes
    std::copy(std::begin(copy_bytes_),
              std::end(copy_bytes_), std::back_inserter(src_bytes_));

    // not enough
    if (src_bytes_.size() + copy_bytes_.size() < next_read_size_) {
        return true;
    }

    // enough
    return Decompress(write_callback);
}


bool ZstdDecompressStream::Flush(val write_callback)
{
    return Decompress(write_callback);
}


bool ZstdDecompressStream::End(val write_callback)
{
    if (!HasStream()) return true;

    auto success = true;
    if (!src_bytes_.empty()) {
        success = Decompress(write_callback);
    }

    stream_.reset();
    return success;
}


bool ZstdDecompressStream::HasStream() const
{
    return stream_ != nullptr;
}


bool ZstdDecompressStream::Decompress(val write_callback)
{
    if (src_bytes_.empty()) return true;

    ZSTD_inBuffer input { &src_bytes_[0], src_bytes_.size(), 0 };
    while (input.pos < input.size) {
        ZSTD_outBuffer output { &dest_bytes_[0], dest_bytes_.size(), 0};
        next_read_size_ = ZSTD_decompressStream(stream_.get(), &output, &input);
        if (ZSTD_isError(next_read_size_)) return false;

        write_to_js_callback(write_callback, dest_bytes_, output.pos);
    }

    src_bytes_.clear();
    return true;
}


// ---- bindings --------------------------------------------------------------

EMSCRIPTEN_BINDINGS(zstd) {
    constant("ERR_UNKNOWN", ERR_UNKNOWN);
    constant("ERR_SIZE_TOO_LARGE", ERR_SIZE_TOO_LARGE);

    function("is_error", &is_error);
    function("compress_bound", &compress_bound);
    function("content_size", &content_size, allow_raw_pointers());
    function("compress", &compress, allow_raw_pointers());
    function("decompress", &decompress, allow_raw_pointers());
    function("dummy", &dummy);

    class_<ZstdCompressStream>("ZstdCompressStream")
        .constructor<>()
        .function("Begin", &ZstdCompressStream::Begin)
        .function("Transform", &ZstdCompressStream::Transform)
        .function("Flush", &ZstdCompressStream::Flush)
        .function("End", &ZstdCompressStream::End)
        ;

    class_<ZstdDecompressStream>("ZstdDecompressStream")
        .constructor<>()
        .function("Begin", &ZstdDecompressStream::Begin)
        .function("Transform", &ZstdDecompressStream::Transform)
        .function("Flush", &ZstdDecompressStream::Flush)
        .function("End", &ZstdDecompressStream::End)
        ;
}

