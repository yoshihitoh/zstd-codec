#pragma once

#include <memory>

#include "napi.h"

#include "zstd-codec/binding/compress.h"
#include "binding.h"

using NodeBinder = ZstdCodecBinder<
        NodeContext,
        NodeBytesBinding<NodeContext>,
        NodeBytesCallbackBinding<NodeContext>,
        NodeErrorThrowable<NodeContext>
>;

class NodeZstdCompressContextBinding : public Napi::ObjectWrap<NodeZstdCompressContextBinding> {
private:
    using Binding = ZstdCompressContextBinding<
            NodeContext,
            NodeBytesBinding<NodeContext>,
            NodeBytesCallbackBinding<NodeContext>,
            NodeErrorThrowable<NodeContext>
    >;

    static Napi::FunctionReference s_constructor;
    std::unique_ptr<Binding> binding_;

public:
    // inner methods
    std::unique_ptr<ZstdCompressContext> takeContext();

public:
    // binding methods
    static Napi::Object initialize(Napi::Env env, Napi::Object exports);

    explicit NodeZstdCompressContextBinding(const Napi::CallbackInfo& info);

    Napi::Value resetSession(const Napi::CallbackInfo& info);
    Napi::Value clearDictionary(const Napi::CallbackInfo& info);
    Napi::Value setCompressionLevel(const Napi::CallbackInfo& info);
    Napi::Value setChecksum(const Napi::CallbackInfo& info);
    Napi::Value setOriginalSize(const Napi::CallbackInfo& info);
};

class NodeZstdCompressStreamBinding : public Napi::ObjectWrap<NodeZstdCompressStreamBinding> {
private:
    using Binding = ZstdCompressStreamBinding<
            NodeContext,
            NodeBytesBinding<NodeContext>,
            NodeBytesCallbackBinding<NodeContext>,
            NodeErrorThrowable<NodeContext>
    >;

    static Napi::FunctionReference s_constructor;
    std::unique_ptr<Binding> binding_;

    static std::unique_ptr<ZstdCompressContext> takeContextFrom(const Napi::CallbackInfo& info);

public:
    static Napi::Object initialize(Napi::Env env, Napi::Object exports);

    explicit NodeZstdCompressStreamBinding(const Napi::CallbackInfo& info);

    void compress(const Napi::CallbackInfo& info);
    void flush(const Napi::CallbackInfo& info);
    void complete(const Napi::CallbackInfo& info);
    void close(const Napi::CallbackInfo& info);
};

