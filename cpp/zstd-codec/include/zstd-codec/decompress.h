#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "tl/expected.hpp"
#include "tl/optional.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// context for decompression
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" {
struct ZSTD_DCtx_s;
}

struct ZstdDecompressContextError {
    tl::optional<size_t> code;
    std::string message;
};

template <typename T>
using ZstdDecompressContextResult = tl::expected<T, ZstdDecompressContextError>;

class ZstdDecompressContext {
public:
    static tl::expected<std::unique_ptr<ZstdDecompressContext>, ZstdDecompressContextError> create();

    ~ZstdDecompressContext();

    ZSTD_DCtx_s* context() const;

    ZstdDecompressContextResult<void> close();
    ZstdDecompressContextResult<void> resetSession();
    ZstdDecompressContextResult<void> clearDictionary();

private:
    ZstdDecompressContext() = default;

    ZSTD_DCtx_s* context_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// decompress stream
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using DecompressStreamCallback = std::function<void(const std::vector<uint8_t>&)>;

struct ZstdDecompressStreamError {
    tl::optional<size_t> code;
    std::string message;
};

template <typename T>
using ZstdDecompressStreamResult = tl::expected<T, ZstdDecompressStreamError>;

class ZstdDecompressStreamImpl;

class ZstdDecompressStream {
private:
    explicit ZstdDecompressStream(std::unique_ptr<ZstdDecompressStreamImpl>&& pimpl);

public:
    ~ZstdDecompressStream();

    static ZstdDecompressStreamResult<std::unique_ptr<ZstdDecompressStream>> fromContext(std::unique_ptr<ZstdDecompressContext> context);
    static ZstdDecompressStreamResult<tl::optional<uint64_t>> getFrameContentSize(const std::vector<uint8_t>& data);

    ZstdDecompressStreamResult<void> decompress(const std::vector<uint8_t>& data, const DecompressStreamCallback& callback);

    ZstdDecompressStreamResult<void> close();

private:
    std::unique_ptr<ZstdDecompressStreamImpl> pimpl_;
};
