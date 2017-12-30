#pragma once


#include "common-types.h"


class ZstdCodec
{
public:
    int CompressBound(usize src_size) const;
    int ContentSize(const Vec<u8>& src) const;
    int Compress(Vec<u8>& dest, const Vec<u8>& src, int compression_level) const;
    int Decompress(Vec<u8>& dest, const Vec<u8>& src) const;
};
