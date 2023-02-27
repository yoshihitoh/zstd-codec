#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <functional>
#include <istream>
#include <iostream>
#include <vector>

#include "zstd.h"
#include "fmt/format.h"

#include "zstd-codec/compress.h"
#include "zstd-codec/decompress.h"

#include "zstd-codec/binding/compress.h"
#include "zstd-codec/binding/decompress.h"

using namespace std;

static const size_t kMaxChunkSize = 100 * 1024;

class VectorBytesBinding {
public:
    using WireType = vector<uint8_t>;
    using ValueType = vector<uint8_t>;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"
    ValueType fromWireType(const WireType& wire) {
        return wire;
    }

    WireType intoWireType(const ValueType& value) {
        return value;
    }
#pragma clang diagnostic pop
};

class FnBytesCallbackBinding {
public:
    using WireType = function<void(const vector<uint8_t>&)>;
    using ValueType = function<void(const vector<uint8_t>&)>;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-convert-member-functions-to-static"
    ValueType fromWireType(const WireType& wire) {
        return [wire](const vector<uint8_t>& data) {
            wire(data);
        };
    }
#pragma clang diagnostic pop
};

struct ThrowErrorBinding {
    static void throwError(const ZstdCodecBindingError& error) {
        auto code = error.code ? fmt::format("Some({})", *error.code) : "None";
        auto message = fmt::format("ZstdCodecBindingError. code={}, message={}", code, error.message);
        throw std::runtime_error(message);
    }
};

using SandboxBinder = ZstdCodecBinder<VectorBytesBinding, FnBytesCallbackBinding, ThrowErrorBinding>;

SandboxBinder sandboxBinder() {
    return SandboxBinder((VectorBytesBinding()), FnBytesCallbackBinding(), ThrowErrorBinding());
}

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

using Result = tl::expected<vector<uint8_t>, string>;
using Handler = std::function<Result(const vector<uint8_t>& data)>;

static Result compress(const vector<uint8_t>& data) {
    auto r_context = ZstdCompressContext::create();
    if (!r_context) {
        return tl::make_unexpected(r_context.error().message);
    }

    auto context = std::move(*r_context);
    context->setOriginalSize(data.size());

    auto r_stream = ZstdCompressStream::withContext(std::move(context));
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

static Result compressWithBindings(const vector<uint8_t>& data) {
    using CompressContextBinding = ZstdCompressContextBinding<VectorBytesBinding, FnBytesCallbackBinding, ThrowErrorBinding>;
    using CompressStreamBinding = ZstdCompressStreamBinding<VectorBytesBinding, FnBytesCallbackBinding, ThrowErrorBinding>;

    auto context = CompressContextBinding ::create(sandboxBinder());
    context->setOriginalSize(data.size());

    auto s = CompressStreamBinding ::create(sandboxBinder(), context->takeContext());

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

        s->compress(buff, [take, &output](const std::vector<::uint8_t>& compressed) {
            cout << "[compress]: " << take << "B → " << compressed.size() << "B."
                 << " output.size(): " << output.size() << " → " << output.size() + compressed.size()
                 << endl;
            output.insert(output.end(), begin(compressed), end(compressed));
        });
    }

    s->complete([&output](const std::vector<::uint8_t>& compressed) {
        cout << "[complete(end)]: flush remaining data " << compressed.size() << "B."
             << " output.size(): " << output.size() << " → " << output.size() + compressed.size()
             << endl;
        output.insert(output.end(), begin(compressed), end(compressed));
    });

    s->close();
    return output;
}

static tl::expected<void, string> compressFile(const char* src_path, Handler&& handler) {
    std::string dest_path(src_path);
    dest_path += ".zstd";

    auto data = readFile(src_path);
    auto r = handler(data);
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

    auto r_stream = ZstdDecompressStream::withContext(std::move(*r_context));
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

static tl::expected<vector<uint8_t>, string> decompressWithBindings(const vector<uint8_t>& data) {
    using ContextBinding = ZstdDecompressContextBinding<VectorBytesBinding, FnBytesCallbackBinding, ThrowErrorBinding>;
    using StreamBinding = ZstdDecompressStreamBinding<VectorBytesBinding, FnBytesCallbackBinding, ThrowErrorBinding>;

    auto context = ContextBinding ::create(sandboxBinder());
    auto s = StreamBinding ::create(sandboxBinder(), context->takeContext());

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

        s->decompress(buff, [take, &output](const std::vector<::uint8_t>& decompressed) {
            cout << "[decompress]: " << take << "B → " << decompressed.size() << "B."
                 << " output.size(): " << output.size() << " → " << output.size() + decompressed.size()
                 << endl;
            output.insert(output.end(), begin(decompressed), end(decompressed));
        });
    }
    s->close();
    return output;
}

static tl::expected<void, string> decompressFile(const char* src_path, Handler&& handler) {
    std::string dest_path(src_path);
    dest_path += ".decompressed";

    auto data = readFile(src_path);
    auto r = handler(data);
    if (!r) {
        return tl::make_unexpected(r.error());
    }

    auto output = std::move(*r);
    cout << "data.size(): " << data.size() << endl;
    cout << "output.size(): " << output.size() << endl;
    writeFile(dest_path.c_str(), std::move(output));

    return {};
}

static void testBinder() {
    ZstdCodecBinder<VectorBytesBinding, FnBytesCallbackBinding, ThrowErrorBinding> b((VectorBytesBinding()), FnBytesCallbackBinding(), ThrowErrorBinding());
    auto xs = b.intoBytesVector(std::vector<uint8_t>{1,2,3});
    auto fn = b.intoBytesCallbackFn([](const vector<uint8_t>& data) {
        cout << "[callback]: [";
        auto first = true;
        for (const auto x : data) {
            if (first) {
                first = false;
            } else {
                cout << ", ";
            }
            cout << static_cast<int32_t>(x);
        }
        cout << "]" << endl;
    });
    fn(xs);

    ZstdCodecBindingError e {123, "TEST ERROR MESSAGE"};
    try {
        b.throwError(e);
    }
    catch (const std::runtime_error& e) {
        cout << "## caught runtime error: " << e.what() << endl;
    }
}

int main(int argc, char** argv) {
    compressFile("./build/zstd-codec/libzstd-codec.a", compress);
    decompressFile("./build/zstd-codec/libzstd-codec.a.zstd", decompress);

    testBinder();
    compressFile("./build/zstd-codec/libzstd-codec.a", compressWithBindings);
    decompressFile("./build/zstd-codec/libzstd-codec.a.zstd", decompressWithBindings);

    return 0;
}
