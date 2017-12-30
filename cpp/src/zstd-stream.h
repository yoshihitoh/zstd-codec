#pragma once

#include <functional>

#include "common-types.h"
#include "zstd.h"


using StreamCallback = std::function<void(const Vec<u8>&)>;


class ZstdCompressStream
{
public:
    ZstdCompressStream();
    ~ZstdCompressStream();

    bool Begin(int compression_level);
    bool Transform(const Vec<u8>& chunk, StreamCallback callback);
    bool Flush(StreamCallback callback);
    bool End(StreamCallback callback);

private:
    using CStreamPtr = std::unique_ptr<ZSTD_CStream, decltype(&ZSTD_freeCStream)>;

    bool HasStream() const;
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
    bool Transform(const Vec<u8>& chunk, StreamCallback callback);
    bool Flush(StreamCallback callback);
    bool End(StreamCallback callback);

private:
    using DStreamPtr = std::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)>;

    bool HasStream() const;
    bool Decompress(const StreamCallback& callback);

    DStreamPtr  stream_;
    size_t      next_read_size_;
    Vec<u8>     src_bytes_;
    Vec<u8>     dest_bytes_;
};

