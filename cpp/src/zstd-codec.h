#pragma once


#include "common-types.h"
// #include "zstd-dict.h"


class ZstdCodec
{
public:
    // information api
    int CompressBound(usize src_size) const;
    int ContentSize(const Vec<u8>& src) const;

    // simple api
    int Compress(Vec<u8>& dest, const Vec<u8>& src, int compression_level) const;
    int Decompress(Vec<u8>& dest, const Vec<u8>& src) const;

    // dictionary api
    int CompressUsingDict(Vec<u8>& dest, const Vec<u8>& src, const Vec<u8>& cdict_bytes, int compression_level) const;
    int DecompressUsingDict(Vec<u8>& dest, const Vec<u8>& src, const Vec<u8>& ddict_bytes) const;
};
