#include <cstdio>
#include <functional>

#include "zstd.h"
#include "zstd-codec.h"
#include "zstd-dict.h"
#include "raii-resource.h"

#if DEBUG
# define USE_DEBUG_ERROR_HANDLER (1)
#endif


static const int ERR_UNKNOWN = -1;
static const int ERR_SIZE_TOO_LARGE = -2;

static const int ERR_ALLOCATE_CCTX = -3;
static const int ERR_ALLOCATE_DCTX = -4;

static const int ERR_LOAD_CDICT = -5;
static const int ERR_LOAD_DDICT = -6;


class IErrorHandler
{
public:
    virtual void OnZstdError(size_t rc) = 0;
    virtual void OnSizeError(size_t rc) = 0;
};


#if USE_DEBUG_ERROR_HANDLER
class DebugErrorHandler : public IErrorHandler
{
public:
    virtual void OnZstdError(size_t rc)
    {
        printf("## zstd error: %s\n", ZSTD_getErrorName(rc));
    }

    virtual void OnSizeError(size_t rc)
    {
        printf("## size error: %s\n", ZSTD_getErrorName(rc));
    }
};


static DebugErrorHandler s_debug_handler;

#endif // USE_DEBUG_ERROR_HANDLER


class CompressContext : public Resource<ZSTD_CCtx>
{
public:
    CompressContext()
        : Resource(ZSTD_createCCtx(), CloseContext)
    {
    }

    bool fail() const
    {
        return get() == nullptr;
    }

private:
    static void CloseContext(ZSTD_CCtx* cctx)
    {
        ZSTD_freeCCtx(cctx);
    }
};


class DecompressContext : public Resource<ZSTD_DCtx>
{
public:
    DecompressContext()
        : Resource(ZSTD_createDCtx(), CloseContext)
    {
    }

    bool fail() const
    {
        return get() == nullptr;
    }

private:
    static void CloseContext(ZSTD_DCtx* dctx)
    {
        ZSTD_freeDCtx(dctx);
    }
};


static int ToResult(size_t rc, IErrorHandler* error_handler = nullptr)
{
#if USE_DEBUG_ERROR_HANDLER
    if (error_handler == nullptr) {
        error_handler = &s_debug_handler;
    }
#endif

    if (ZSTD_isError(rc)) {
        if (error_handler != nullptr) error_handler->OnZstdError(rc);
        return ERR_UNKNOWN;
    }
    else if (rc >= static_cast<size_t>(INT_MAX)) {
        if (error_handler != nullptr) error_handler->OnSizeError(rc);
        return ERR_SIZE_TOO_LARGE;
    }

    return static_cast<int>(rc);
}


int ZstdCodec::CompressBound(usize src_size) const
{
    const auto rc = ZSTD_compressBound(src_size);
    return ToResult(rc);
}


int ZstdCodec::ContentSize(const Vec<u8>& src) const
{
    const auto rc = ZSTD_getFrameContentSize(&src[0], src.size());
    return ToResult(rc);
}


int ZstdCodec::Compress(Vec<u8>& dest, const Vec<u8>& src, int compression_level) const
{
    const auto rc = ZSTD_compress(&dest[0], dest.size(),
                                  &src[0], src.size(), compression_level);
    return ToResult(rc);
}


int ZstdCodec::Decompress(Vec<u8>& dest, const Vec<u8>& src) const
{
    const auto rc = ZSTD_decompress(&dest[0], dest.size(),
                                    &src[0], src.size());
    return ToResult(rc);
}


int ZstdCodec::CompressUsingDict(Vec<u8>& dest, const Vec<u8>& src, const ZstdCompressionDict& cdict) const
{
    CompressContext context;
    if (context.fail()) return ERR_ALLOCATE_CCTX;

    const auto rc = ZSTD_compress_usingCDict(context.get(),
                                             &dest[0], dest.size(),
                                             &src[0], src.size(),
                                             cdict.get());
    return ToResult(rc);
}


int ZstdCodec::DecompressUsingDict(Vec<u8>& dest, const Vec<u8>& src, const ZstdDecompressionDict& ddict) const
{
    DecompressContext context;
    if (context.fail()) return ERR_ALLOCATE_DCTX;

    const auto rc = ZSTD_decompress_usingDDict(context.get(),
                                               &dest[0], dest.size(),
                                               &src[0], src.size(),
                                               ddict.get());
    return ToResult(rc);
}

