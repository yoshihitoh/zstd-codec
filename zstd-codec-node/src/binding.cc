#include <cstdint>
#include <iostream>
#include <vector>

#include "napi.h"

#include "binding.h"
#include "compress.h"
#include "decompress.h"

Napi::Object initialize(Napi::Env env, Napi::Object exports) {
    // compression
    NodeZstdCompressContextBinding::initialize(env, exports);
    NodeZstdCompressStreamBinding::initialize(env, exports);

    // decompression
    NodeZstdDecompressContextBinding::initialize(env, exports);
    NodeZstdDecompressStreamBinding::initialize(env, exports);

    // エクスポート結果を返却
    return exports;
}

NODE_API_MODULE(NodeZstdCodec, initialize);
