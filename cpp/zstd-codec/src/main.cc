#include <algorithm>
#include <cstdint>
#include <fstream>
#include <istream>
#include <iostream>
#include <vector>

#include "zstd.h"

#include "zstd-codec/compress.h"
#include "zstd-codec/decompress.h"

using namespace std;

static const size_t kMaxChunkSize = 100 * 1024;

static vector<uint8_t> readFile(const char* path) {
    ifstream ifs(path, ios::in | ios::binary);
    vector<uint8_t> data;
    copy(istreambuf_iterator<char>(ifs),
         istreambuf_iterator<char>(),
         back_inserter(data));
    ifs.close();
    return std::move(data);
}

void writeFile(const char* path, vector<uint8_t>&& data) {
    ofstream ofs(path, ios::out | ios::binary);

    auto p = static_cast<const char*>(static_cast<const void*>(&data[0]));
    ofs.write(p, data.size());
    ofs.close();
}

static tl::expected<vector<uint8_t>, string> compress(const vector<uint8_t>& data) {
    auto r_context = ZstdCompressContext::create();
    if (!r_context) {
        return tl::make_unexpected(r_context.error().message);
    }

    auto r_stream = ZstdCompressStream::sized(std::move(*r_context), data.size());
    if (!r_stream) {
        return tl::make_unexpected(r_stream.error().message);
    }

    auto s = std::move(*r_stream);

    size_t i = 0;
    std::vector<uint8_t> buff;
    buff.reserve(kMaxChunkSize);

    std::vector<uint8_t> output;
    output.reserve(data.size());

    while (i < data.size()) {
        buff.clear();

        auto it = begin(data);
        const auto take = i + kMaxChunkSize < data.size() ? kMaxChunkSize : data.size() - i;
        advance(it, i);
        copy(it, it + take, back_inserter(buff));
        i += take;

        const auto r_compress = s->compress(buff, [take, &output](const std::vector<::uint8_t>& compressed) {
            cout << "[compress]: " << take << "B → " << compressed.size() << "B."
                 << " output.size(): " << output.size() << " → " << output.size() + compressed.size()
                 << endl;
            output.insert(output.end(), begin(compressed), end(compressed));
        });
        if (!r_compress) {
            return tl::make_unexpected(r_compress.error().message);
        }
    }

    const auto r_compress = s->complete([&output](const std::vector<::uint8_t>& compressed) {
        cout << "[complete(end)]: flush remaining data " << compressed.size() << "B."
             << " output.size(): " << output.size() << " → " << output.size() + compressed.size()
             << endl;
        output.insert(output.end(), begin(compressed), end(compressed));
    });
    if (!r_compress) {
        return tl::make_unexpected(r_compress.error().message);
    }

    return output;
}

static tl::expected<void, string> compressFile(const char* src_path) {
    std::string dest_path(src_path);
    dest_path += ".zstd";

    auto data = readFile(src_path);
    auto r = compress(data);
    if (!r) {
        return tl::make_unexpected(r.error());
    }

    auto output = std::move(*r);
    cout << "data.size(): " << data.size() << endl;
    cout << "output.size(): " << output.size() << endl;
    writeFile(dest_path.c_str(), std::move(output));

    return {};
}

static tl::expected<vector<uint8_t>, string> decompress(const vector<uint8_t>& data) {
    auto r_context = ZstdDecompressContext::create();
    if (!r_context) {
        return tl::make_unexpected(r_context.error().message);
    }

    auto r_stream = ZstdDecompressStream::fromContext(std::move(*r_context));
    if (!r_stream) {
        return tl::make_unexpected(r_stream.error().message);
    }

    auto s = std::move(*r_stream);

    size_t i = 0;
    std::vector<uint8_t> buff;
    buff.reserve(kMaxChunkSize);

    std::vector<uint8_t> output;
    const auto r_size = ZstdDecompressStream::getFrameContentSize(data);
    if (r_size && *r_size) {
        output.reserve(**r_size);
    }

    while (i < data.size()) {
        buff.clear();

        auto it = begin(data);
        const auto take = i + kMaxChunkSize < data.size() ? kMaxChunkSize : data.size() - i;
        advance(it, i);
        copy(it, it + take, back_inserter(buff));
        i += take;

        const auto r_decompress = s->decompress(buff, [take, &output](const std::vector<::uint8_t>& decompressed) {
            cout << "[decompress]: " << take << "B → " << decompressed.size() << "B."
                 << " output.size(): " << output.size() << " → " << output.size() + decompressed.size()
                 << endl;
            output.insert(output.end(), begin(decompressed), end(decompressed));
        });
        if (!r_decompress) {
            return tl::make_unexpected(r_decompress.error().message);
        }
    }

    return output;
}

static tl::expected<void, string> decompressFile(const char* src_path) {
    std::string dest_path(src_path);
    dest_path += ".decompressed";

    auto data = readFile(src_path);
    auto r = decompress(data);
    if (!r) {
        return tl::make_unexpected(r.error());
    }

    auto output = std::move(*r);
    cout << "data.size(): " << data.size() << endl;
    cout << "output.size(): " << output.size() << endl;
    writeFile(dest_path.c_str(), std::move(output));

    return {};
}

int main(int argc, char** argv) {
    compressFile("./build/zstd-codec/libzstd-codec.a");
    decompressFile("./build/zstd-codec/libzstd-codec.a.zstd");

    return 0;
}
