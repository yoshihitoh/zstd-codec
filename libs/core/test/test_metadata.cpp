#include "gtest/gtest.h"
#include "zstd-codec/core/metadata.h"

using namespace zstd_codec::core;

TEST(ZstdCodecMetadata, ZstdVersionString) {
    EXPECT_EQ("1.5.6", ZstdCodecMetadata::zstd_version_string());
}

TEST(ZstdCodecMetadata, ZstdCodecVersionString) {
    EXPECT_EQ("0.2.0", ZstdCodecMetadata::zstd_codec_version_string());
}
