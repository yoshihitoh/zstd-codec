#pragma once
#include "zstd-codec/core/types.h"

namespace zstd_codec::core {

    class ZstdCodecMetadata {
    public:
        static const string& zstd_version_string();
        static const string& zstd_codec_version_string();
    };

} // zstd_codec
