#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "tl/expected.hpp"
#include "tl/optional.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// context for compression
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" {
struct ZSTD_CCtx_s;
}

struct ZstdCompressContextError {
    tl::optional<size_t> code;
    std::string message;
};

template <typename T>
using ZstdCompressContextResult = tl::expected<T, ZstdCompressContextError>;

class ZstdCompressContext {
public:
    static tl::expected<std::unique_ptr<ZstdCompressContext>, ZstdCompressContextError> create();

    ~ZstdCompressContext();

    ZSTD_CCtx_s* context() const;

    ZstdCompressContextResult<void> resetSession();
    ZstdCompressContextResult<void> clearDictionary();
    ZstdCompressContextResult<void> setCompressionLevel(int compression_level);
    ZstdCompressContextResult<void> setChecksum(bool enable);
    ZstdCompressContextResult<void> setOriginalSize(uint64_t original_size);
    ZstdCompressContextResult<void> close();

private:
    ZstdCompressContext() = default;

    ZSTD_CCtx_s* context_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// compress stream
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using CompressStreamCallback = std::function<void(const std::vector<uint8_t>&)>;

struct ZstdCompressStreamError {
    tl::optional<size_t> code;
    std::string message;
};

template <typename T>
using ZstdCompressStreamResult = tl::expected<T, ZstdCompressStreamError>;

class ZstdCompressStreamImpl;

class ZstdCompressStream {
private:
    explicit ZstdCompressStream(std::unique_ptr<ZstdCompressStreamImpl>&& pimpl);

public:
    ~ZstdCompressStream();

    static ZstdCompressStreamResult<std::unique_ptr<ZstdCompressStream>> withContext(std::unique_ptr<ZstdCompressContext>&& context);

    ZstdCompressStreamResult<void> compress(const std::vector<uint8_t>& data, const CompressStreamCallback& callback);
    ZstdCompressStreamResult<void> flush(const CompressStreamCallback& callback);
    ZstdCompressStreamResult<void> complete(const CompressStreamCallback& callback);

    ZstdCompressStreamResult<void> close();

private:
    std::unique_ptr<ZstdCompressStreamImpl> pimpl_;
};
