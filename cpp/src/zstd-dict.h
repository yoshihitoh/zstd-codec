#pragma once

#include "common-types.h"
#include "raii-resource.h"


extern "C" {
struct ZSTD_CDict_s;    // orginal struct of ZSTD_CDict
struct ZSTD_DDict_s;    // orginal struct of ZSTD_DDict
}


class ZstdCompressionDict : public Resource<ZSTD_CDict_s>
{
public:
    ZstdCompressionDict(const Vec<u8>& dict_bytes, int compression_level);

    bool fail() const;
};


class ZstdDecompressionDict : public Resource<ZSTD_DDict_s>
{
public:
    ZstdDecompressionDict(const Vec<u8>& dict_bytes);

    bool fail() const;
};

