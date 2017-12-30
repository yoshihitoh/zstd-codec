#include "zstd-stream.h"

//
// ZstdCompressStream
//
///////////////////////////////////////////////////////////////////////////////


ZstdCompressStream::ZstdCompressStream()
    : stream_(nullptr, ZSTD_freeCStream)
    , next_read_size_()
    , src_bytes_()
    , dest_bytes_()
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
    next_read_size_ = src_bytes_.capacity();

    return true;
}


bool ZstdCompressStream::Transform(const Vec<u8>& chunk, StreamCallback callback)
{
    if (!HasStream()) return false;

    // append src bytes
    std::copy(std::begin(chunk),
              std::end(chunk), std::back_inserter(src_bytes_));

    // not enough
    if (src_bytes_.size() < next_read_size_) {
        return true;
    }

    // enough
    return Compress(callback);
}


bool ZstdCompressStream::Flush(StreamCallback callback)
{
    return Compress(callback);
}


bool ZstdCompressStream::End(StreamCallback callback)
{
    if (!HasStream()) return true;

    auto success = true;
    if (!src_bytes_.empty()) {
        success = Compress(callback);
    }

    if (success) {
        dest_bytes_.resize(dest_bytes_.capacity());
        ZSTD_outBuffer output { &dest_bytes_[0], dest_bytes_.size(), 0 };
        const auto remaining = ZSTD_endStream(stream_.get(), &output);
        if (remaining > 0u) return false;

        dest_bytes_.resize(output.pos);
        callback(dest_bytes_);
    }

    stream_.reset();
    return success;
}


bool ZstdCompressStream::HasStream() const
{
    return stream_ != nullptr;
}


bool ZstdCompressStream::Compress(const StreamCallback& callback)
{
    if (src_bytes_.empty()) return true;

    ZSTD_inBuffer input { &src_bytes_[0], src_bytes_.size(), 0 };
    while (input.pos < input.size) {
        dest_bytes_.resize(dest_bytes_.capacity());
        ZSTD_outBuffer output { &dest_bytes_[0], dest_bytes_.size(), 0};
        next_read_size_ = ZSTD_compressStream(stream_.get(), &output, &input);
        if (ZSTD_isError(next_read_size_)) return false;

        dest_bytes_.resize(output.pos);
        callback(dest_bytes_);
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
    next_read_size_ = init_rc;

    return true;
}


bool ZstdDecompressStream::Transform(const Vec<u8>& chunk, StreamCallback callback)
{
    if (!HasStream()) return false;

    // append src bytes
    std::copy(std::begin(chunk),
              std::end(chunk), std::back_inserter(src_bytes_));

    // not enough
    if (src_bytes_.size() < next_read_size_) {
        return true;
    }

    // enough
    return Decompress(callback);
}


bool ZstdDecompressStream::Flush(StreamCallback callback)
{
    return Decompress(callback);
}


bool ZstdDecompressStream::End(StreamCallback callback)
{
    if (!HasStream()) return true;

    auto success = true;
    if (!src_bytes_.empty()) {
        success = Decompress(callback);
    }

    stream_.reset();
    return success;
}


bool ZstdDecompressStream::HasStream() const
{
    return stream_ != nullptr;
}


bool ZstdDecompressStream::Decompress(const StreamCallback& callback)
{
    if (src_bytes_.empty()) return true;

    ZSTD_inBuffer input { &src_bytes_[0], src_bytes_.size(), 0 };
    while (input.pos < input.size) {
        dest_bytes_.resize(dest_bytes_.capacity());
        ZSTD_outBuffer output { &dest_bytes_[0], dest_bytes_.size(), 0};
        next_read_size_ = ZSTD_decompressStream(stream_.get(), &output, &input);
        if (ZSTD_isError(next_read_size_)) return false;

        dest_bytes_.resize(output.pos);
        callback(dest_bytes_);
    }

    src_bytes_.clear();
    return true;
}
