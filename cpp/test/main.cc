#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <iostream>
#include <string>

#include "zstd-codec.h"
#include "zstd-dict.h"
#include "zstd-stream.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


class FileResource : public Resource<FILE>
{
public:
    FileResource(const std::string& path, const char* mode)
        : FileResource(path.c_str(), mode)
    {
    }

    FileResource(const char* path, const char* mode)
        : Resource(fopen(path, mode), fclose)
    {
    }
};


static std::string fixturePath(const char* name)
{
    static const std::string kFixturePath("test/fixtures");
    return kFixturePath + "/" + name;
}


static std::string tempPath(const char* name)
{
    static const std::string kTempPath("test/tmp");
    return kTempPath + "/" + name;
}


static Vec<u8> loadFixture(const char* name)
{
    const auto path = fixturePath(name);
    std::ifstream stream(path.c_str(), std::ios::in | std::ios::binary);
    return Vec<u8>((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}


TEST_CASE("Zstd-Dictionary-Interfaces", "[zstd][compress][decompress][dictionary]")
{
    const auto dict = loadFixture("sample-dict");
    const auto sample_books = loadFixture("sample-books.json");

    const auto compression_level = 5;
    ZstdCompressionDict cdict(dict, compression_level);
    ZstdDecompressionDict ddict(dict);

    ZstdCodec codec;
    Vec<u8> compressed_bytes(codec.CompressBound(sample_books.size()));

    // compress with dictionary
    // auto rc = codec.CompressUsingDict(compressed_bytes, sample_books, cdict);
    auto rc = codec.CompressUsingDict(compressed_bytes, sample_books, dict, compression_level);
    REQUIRE(rc >= 0);

    REQUIRE(rc < sample_books.size());
    compressed_bytes.resize(rc);

    REQUIRE(codec.ContentSize(compressed_bytes) == sample_books.size());

    // decompress with dictionary

    // NOTE: ensure enough buffer to test return code (avoid truncation)
    Vec<u8> decompressed_bytes(sample_books.size() * 2);
    // rc = codec.DecompressUsingDict(decompressed_bytes, compressed_bytes, ddict);
    rc = codec.DecompressUsingDict(decompressed_bytes, compressed_bytes, dict);
    REQUIRE(rc >= 0);

    REQUIRE(rc == sample_books.size());
    decompressed_bytes.resize(rc);

    REQUIRE(decompressed_bytes == sample_books);

    // cannot decompress without dictionary
    rc = codec.Decompress(decompressed_bytes, compressed_bytes);
    REQUIRE(rc < 0);
}


TEST_CASE("ZstdCompressStream", "[zstd][compress][stream]")
{
    const auto block_size = 1024;
    size_t read_size;
    Vec<u8> read_buff(block_size);

    Vec<u8> content_bytes;
    content_bytes.reserve(1 * 1024 * 1024);

    Vec<u8> result_bytes;
    result_bytes.reserve(1 * 1024 * 1024);

    const StreamCallback callback = [&result_bytes](const Vec<u8>& compressed) {
        std::copy(std::begin(compressed),
                  std::end(compressed), std::back_inserter(result_bytes));
    };

    ZstdCompressStream stream;
    REQUIRE(stream.Begin(3));

    FileResource bmp_file(fixturePath("dance_yorokobi_mai_man.bmp"), "rb");
    while ((read_size = fread(&read_buff[0], 1, read_buff.size(), bmp_file.get()))) {
        read_buff.resize(read_size);
        REQUIRE(stream.Transform(read_buff, callback));

        std::copy(std::begin(read_buff),
                  std::end(read_buff), std::back_inserter(content_bytes));

        read_buff.resize(read_buff.capacity());
    }

    REQUIRE(stream.End(callback));
    REQUIRE(result_bytes.size() < content_bytes.size());
    bmp_file.Close();

    ZstdCodec codec;
    Vec<u8> decompressed_bytes(content_bytes.size());
    REQUIRE(codec.Decompress(decompressed_bytes, result_bytes) == content_bytes.size());
    REQUIRE(decompressed_bytes == content_bytes);

    FileResource result_file(tempPath("dance_yorokobi_mai_man.bmp.zst"), "wb");
    fwrite(&result_bytes[0], result_bytes.size(), 1, result_file.get());
    result_file.Close();
}


TEST_CASE("ZstdDecompressStream", "[zstd][decompress][stream]")
{

    const auto block_size = 1024;
    size_t read_size;
    Vec<u8> read_buff(block_size);

    Vec<u8> compressed_bytes;
    compressed_bytes.reserve(1 * 1024 * 1024);

    Vec<u8> result_bytes;
    result_bytes.reserve(1 * 1024 * 1024);

    const StreamCallback callback = [&result_bytes](const Vec<u8>& decompressed) {
        std::copy(std::begin(decompressed),
                  std::end(decompressed), std::back_inserter(result_bytes));
    };

    ZstdDecompressStream stream;
    REQUIRE(stream.Begin());

    FileResource zst_file(fixturePath("dance_yorokobi_mai_woman.bmp.zst"), "rb");
    while ((read_size = fread(&read_buff[0], 1, read_buff.size(), zst_file.get()))) {
        read_buff.resize(read_size);
        REQUIRE(stream.Transform(read_buff, callback));

        std::copy(std::begin(read_buff),
                  std::end(read_buff), std::back_inserter(compressed_bytes));

        read_buff.resize(read_buff.capacity());
    }

    REQUIRE(stream.End(callback));
    REQUIRE(result_bytes.size() > compressed_bytes.size());
    zst_file.Close();

    ZstdCodec codec;
    const auto content_size = codec.ContentSize(compressed_bytes);
    REQUIRE(result_bytes.size() == content_size);

    Vec<u8> content_bytes(content_size);
    REQUIRE(codec.Decompress(content_bytes, compressed_bytes) == content_size);
    REQUIRE(content_bytes == result_bytes);

    FileResource result_file(tempPath("dance_yorokobi_mai_woman.bmp"), "wb");
    fwrite(&result_bytes[0], result_bytes.size(), 1, result_file.get());
    result_file.Close();
}
