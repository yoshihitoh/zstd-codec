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
#define CATCH_CONFIG_NO_POSIX_SIGNALS
#include "catch.hpp"

#ifndef HAS_FS_WRITE
#define HAS_FS_WRITE (1)
#endif // HAS_FS_WRITE

#define UNUSED(x) (void)x


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


static void writeTo(const std::string& path, const Vec<u8>& src)
{
#if HAS_FS_WRITE
    FileResource dest_file(path, "wb");
    fwrite(&src[0], src.size(), 1, dest_file.get());
    dest_file.Close();
#else
    UNUSED(path);
    UNUSED(src);
#endif
}


TEST_CASE("Zstd-Dictionary-Interfaces", "[zstd][compress][decompress][dictionary]")
{
    const auto dict_bytes = loadFixture("sample-dict");
    const auto sample_books = loadFixture("sample-books.json");

    const auto compression_level = 5;
    ZstdCompressionDict cdict(dict_bytes, compression_level);
    ZstdDecompressionDict ddict(dict_bytes);

    ZstdCodec codec;
    Vec<u8> compressed_bytes(codec.CompressBound(sample_books.size()));

    // compress with dictionary
    auto rc = codec.CompressUsingDict(compressed_bytes, sample_books, cdict);
    REQUIRE(rc >= 0);

    REQUIRE(rc < sample_books.size());
    compressed_bytes.resize(rc);

    REQUIRE(codec.ContentSize(compressed_bytes) == sample_books.size());

    // decompress with dictionary

    // NOTE: ensure enough buffer to test return code (avoid truncation)
    Vec<u8> decompressed_bytes(sample_books.size() * 2);
    rc = codec.DecompressUsingDict(decompressed_bytes, compressed_bytes, ddict);
    REQUIRE(rc >= 0);

    REQUIRE(rc == sample_books.size());
    decompressed_bytes.resize(rc);

    REQUIRE(decompressed_bytes == sample_books);

    // cannot decompress without dictionary
    rc = codec.Decompress(decompressed_bytes, compressed_bytes);
    REQUIRE(rc < 0);
}


TEST_CASE("Stream using dictionary", "[zstd][compress][decompress][dictionary][stream]")
{
    const auto dict_bytes = loadFixture("sample-dict");
    const auto sample_books = loadFixture("sample-books.json");

    const auto compression_level = 5;
    ZstdCompressionDict cdict(dict_bytes, compression_level);
    ZstdDecompressionDict ddict(dict_bytes);

    Vec<u8> compressed_bytes;
    compressed_bytes.reserve(1 * 1024 * 1024);

    const auto append_bytes = [](Vec<u8>& dest, const Vec<u8>& src) {
        std::copy(std::begin(src), std::end(src), std::back_inserter(dest));
    };

    const StreamCallback cstream_callback = [&append_bytes, &compressed_bytes](const Vec<u8>& compressed) {
        append_bytes(compressed_bytes, compressed);
    };

    ZstdCompressStream cstream;
    REQUIRE(cstream.Begin(cdict));
    REQUIRE(cstream.Transform(sample_books, cstream_callback));
    REQUIRE(cstream.End(cstream_callback));
    REQUIRE(compressed_bytes.size() < sample_books.size());

    Vec<u8> content_bytes;
    content_bytes.reserve(1 * 1024 * 1024);

    const StreamCallback dstream_callback = [&append_bytes, &content_bytes](const Vec<u8>& decompressed) {
        append_bytes(content_bytes, decompressed);
    };

    ZstdDecompressStream dstream;
    REQUIRE(dstream.Begin(ddict));
    REQUIRE(dstream.Transform(compressed_bytes, dstream_callback));
    REQUIRE(dstream.End(dstream_callback));
    REQUIRE(compressed_bytes.size() < content_bytes.size());
    REQUIRE(content_bytes == sample_books);
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

    writeTo(tempPath("dance_yorokobi_mai_man.bmp.zst"), result_bytes);
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

    writeTo(tempPath("dance_yorokobi_mai_woman.bmp"), result_bytes);
}
