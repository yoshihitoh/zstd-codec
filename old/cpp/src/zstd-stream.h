#pragma once

#include <functional>

#include "common-types.h"
#include "zstd.h"


using StreamCallback = std::function<void(const Vec<u8>&)>;


class ZstdCompressionDict;
class ZstdDecompressionDict;


class ZstdCompressStream
{
public:
    ZstdCompressStream();
    ~ZstdCompressStream();

    bool Begin(int compression_level);
    bool Begin(const ZstdCompressionDict& cdict);
    bool Transform(const Vec<u8>& chunk, StreamCallback callback);
    bool Flush(StreamCallback callback);
    bool End(StreamCallback callback);

private:
    using CStreamPtr = std::unique_ptr<ZSTD_CStream, decltype(&ZSTD_freeCStream)>;
    using CStreamInitializer = std::function<size_t(ZSTD_CStream*)>;

    bool HasStream() const;
    bool Begin(CStreamInitializer initializer);
    bool Compress(const StreamCallback& callback);

    CStreamPtr  stream_;
    size_t      next_read_size_;
    Vec<u8>     src_bytes_;
    Vec<u8>     dest_bytes_;
};


class ZstdDecompressStream
{
public:
    ZstdDecompressStream();
    ~ZstdDecompressStream();

    bool Begin();
    bool Begin(const ZstdDecompressionDict& ddict);
    bool Transform(const Vec<u8>& chunk, StreamCallback callback);
    bool Flush(StreamCallback callback);
    bool End(StreamCallback callback);

private:
    using DStreamPtr = std::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)>;
    using DStreamInitializer = std::function<size_t(ZSTD_DStream*)>;

    bool HasStream() const;
    bool Begin(DStreamInitializer initializer);
    bool Decompress(const StreamCallback& callback);

    DStreamPtr  stream_;
    size_t      next_read_size_;
    Vec<u8>     src_bytes_;
    Vec<u8>     dest_bytes_;
};

