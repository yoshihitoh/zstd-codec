#pragma once

#include <memory>

#include "napi.h"

#include "zstd-codec/binding/decompress.h"
#include "binding.h"

class NodeZstdDecompressContextBinding : public Napi::ObjectWrap<NodeZstdDecompressContextBinding> {
private:
    using Binding = ZstdDecompressContextBinding<
            NodeContext,
            NodeBytesBinding<NodeContext>,
            NodeBytesCallbackBinding<NodeContext>,
            NodeErrorThrowable<NodeContext>
    >;

    static Napi::FunctionReference s_constructor;
    std::unique_ptr<Binding> binding_;

public:
    // inner methods
    std::unique_ptr<ZstdDecompressContext> takeContext();

public:
    // binding methods
    static Napi::Object initialize(Napi::Env env, Napi::Object exports);

    explicit NodeZstdDecompressContextBinding(const Napi::CallbackInfo& info);

    Napi::Value resetSession(const Napi::CallbackInfo& info);
    Napi::Value clearDictionary(const Napi::CallbackInfo& info);
};

class NodeZstdDecompressStreamBinding : public Napi::ObjectWrap<NodeZstdDecompressStreamBinding> {
private:
    using Binding = ZstdDecompressStreamBinding<
            NodeContext,
            NodeBytesBinding<NodeContext>,
            NodeBytesCallbackBinding<NodeContext>,
            NodeErrorThrowable<NodeContext>
    >;

    static Napi::FunctionReference s_constructor;
    std::unique_ptr<Binding> binding_;

    static std::unique_ptr<ZstdDecompressContext> takeContextFrom(const Napi::CallbackInfo& info);

public:
    static Napi::Object initialize(Napi::Env env, Napi::Object exports);

    explicit NodeZstdDecompressStreamBinding(const Napi::CallbackInfo& info);

    void decompress(const Napi::CallbackInfo& info);
    void close(const Napi::CallbackInfo& info);
};

