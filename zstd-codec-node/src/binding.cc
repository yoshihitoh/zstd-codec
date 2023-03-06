#include <iostream>

#include "napi.h"
#include "zstd-codec/binding/compress.h"
#include "zstd-codec/binding/decompress.h"

class NodeZstdStreamBinding : public Napi::ObjectWrap<NodeZstdStreamBinding> {
public:
    static Napi::Object initialize(Napi::Env env, Napi::Object exports);

    NodeZstdStreamBinding(const Napi::CallbackInfo& info);

    Napi::Value hello(const Napi::CallbackInfo &info);

private:
    static Napi::FunctionReference s_constructor;
};

Napi::FunctionReference NodeZstdStreamBinding::s_constructor;

Napi::Object NodeZstdStreamBinding::initialize(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "ZstdStreamBinding", {
            InstanceMethod("hello", &NodeZstdStreamBinding::hello)
    });

    // コンストラクタを用意する
    s_constructor = Napi::Persistent(func);
    // コンストラクタは破棄されないようにする
    s_constructor.SuppressDestruct();

    // エクスポート
    exports.Set("ZstdStreamBinding", func);

    return exports;
}

NodeZstdStreamBinding::NodeZstdStreamBinding(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<NodeZstdStreamBinding>(info)
{
}

Napi::Value NodeZstdStreamBinding::hello(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    return Napi::String::New(env, "Hello N-API World!!");
}

Napi::Object initialize(Napi::Env env, Napi::Object exports)
{
    // ラッパークラスを初期化する
    NodeZstdStreamBinding::initialize(env, exports);

    // エクスポート結果を返却
    return exports;
}

NODE_API_MODULE(NodeZstdCodec, initialize);
