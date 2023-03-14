#include <cstdint>
#include <iostream>
#include <vector>

#include "napi.h"

#include "binding.h"
#include "compress.h"

Napi::Object initialize(Napi::Env env, Napi::Object exports)
{
    // ラッパークラスを初期化する
    NodeZstdCompressContextBinding::initialize(env, exports);
    NodeZstdCompressStreamBinding::initialize(env, exports);

    // エクスポート結果を返却
    return exports;
}

NODE_API_MODULE(NodeZstdCodec, initialize);
