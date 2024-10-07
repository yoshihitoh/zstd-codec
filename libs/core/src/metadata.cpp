#include "zstd-codec/core/metadata.h"
#include "zstd-codec/core/version.h"

#include "../../external/zstd/lib/zstd.h"

namespace zstd_codec::core {
    static const string kZstdVersionString = ZSTD_versionString();
    const string& ZstdCodecMetadata::zstd_version_string() {
        return kZstdVersionString;
    }

    static const string kZstdCodecVersionString = ZSTD_CODEC_VERSION;
    const string& ZstdCodecMetadata::zstd_codec_version_string() {
        return kZstdCodecVersionString;
    }
}
