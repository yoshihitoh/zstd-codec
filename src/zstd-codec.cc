#include <cstdio>
#include "zstd.h"
#include "zstd-codec.h"


static const int ERR_UNKNOWN = -1;
static const int ERR_SIZE_TOO_LARGE = -2;


int ZstdCodec::CompressBound(usize src_size) const
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


int ZstdCodec::ContentSize(const Vec<u8>& src) const
{
    const auto rc = ZSTD_getFrameContentSize(&src[0], src.size());
    if (ZSTD_isError(rc)) {
        return ERR_UNKNOWN;
    }
    else if (rc >= static_cast<size_t>(INT_MAX)) {
        return ERR_SIZE_TOO_LARGE;
    }

    return static_cast<int>(rc);
}


int ZstdCodec::Compress(Vec<u8>& dest, const Vec<u8>& src, int compression_level) const
{
    const auto rc = ZSTD_compress(&dest[0], dest.size(),
                                  &src[0], src.size(), compression_level);
    if (ZSTD_isError(rc)) {
        return ERR_UNKNOWN;
    }
    else if (rc >= static_cast<size_t>(INT_MAX)) {
        return ERR_SIZE_TOO_LARGE;
    }

    return static_cast<int>(rc);
}


int ZstdCodec::Decompress(Vec<u8>& dest, const Vec<u8>& src) const
{
    const auto rc = ZSTD_decompress(&dest[0], dest.size(),
                                    &src[0], src.size());
    if (ZSTD_isError(rc)) {
        const auto error_name = ZSTD_getErrorName(rc);
        printf("## decompress error = %s\n", error_name);
        return ERR_UNKNOWN;
    }
    else if (rc >= static_cast<size_t>(INT_MAX)) {
        return ERR_SIZE_TOO_LARGE;
    }

    return static_cast<int>(rc);
}
