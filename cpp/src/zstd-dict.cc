#include "zstd.h"
#include "zstd-dict.h"


static void CloseCDict(ZSTD_CDict_s* cdict)
{
    ZSTD_freeCDict(cdict);
}


static void CloseDDict(ZSTD_DDict_s* ddict)
{
    ZSTD_freeDDict(ddict);
}


//
// ZstdCompressionDict
//
////////////////////////////////////////////////////////////////////////////////

ZstdCompressionDict::ZstdCompressionDict(const Vec<u8>& dict_bytes, int compression_level)
    : Resource(ZSTD_createCDict(&dict_bytes[0], dict_bytes.size(), compression_level), CloseCDict)
{
}


bool ZstdCompressionDict::fail() const
{
    return get() == nullptr;
}


//
// ZstdDecompressionDict
//
////////////////////////////////////////////////////////////////////////////////

ZstdDecompressionDict ::ZstdDecompressionDict(const Vec<u8>& dict_bytes)
    : Resource(ZSTD_createDDict(&dict_bytes[0], dict_bytes.size()), CloseDDict)
{
}


bool ZstdDecompressionDict::fail() const
{
    return get() == nullptr;
}

