#include "decompress.h"

////////////////////////////////////////////////////////////////////////////////
//
// NodeZstdCompressContextBinding
//
////////////////////////////////////////////////////////////////////////////////

Napi::FunctionReference NodeZstdDecompressContextBinding::s_constructor;

std::unique_ptr<ZstdDecompressContext> NodeZstdDecompressContextBinding::takeContext() {
    return binding_->takeContext();
}

Napi::Object NodeZstdDecompressContextBinding::initialize(Napi::Env env, Napi::Object exports) {
    auto func = DefineClass(env, "ZstdDecompressContextBinding", {
            InstanceMethod("resetSession", &NodeZstdDecompressContextBinding::resetSession),
            InstanceMethod("clearDictionary", &NodeZstdDecompressContextBinding::clearDictionary),
    });

    s_constructor = Napi::Persistent(func);
    s_constructor.SuppressDestruct();

    exports.Set("ZstdDecompressContextBinding", func);
    return exports;
}

NodeZstdDecompressContextBinding::NodeZstdDecompressContextBinding(const Napi::CallbackInfo &info)
        : ObjectWrap(info)
        , binding_(Binding::create({info.Env()}, NodeBinder({}, {}, {})))
{
}

Napi::Value NodeZstdDecompressContextBinding::resetSession(const Napi::CallbackInfo &info) {
    binding_->updateContext({info.Env()}, &ZstdDecompressContext::resetSession);
    return info.This();
}

Napi::Value NodeZstdDecompressContextBinding::clearDictionary(const Napi::CallbackInfo &info) {
    binding_->updateContext({info.Env()}, &ZstdDecompressContext::clearDictionary);
    return info.This();
}

////////////////////////////////////////////////////////////////////////////////
//
// NodeZstdDecompressStreamBinding
//
////////////////////////////////////////////////////////////////////////////////

Napi::FunctionReference NodeZstdDecompressStreamBinding::s_constructor;

std::unique_ptr<ZstdDecompressContext>
NodeZstdDecompressStreamBinding::takeContextFrom(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() < 1) {
        Napi::Error::New(env, "requires 1 argument. (contextBinding: ZstdDecompressContextBinding)")
                .ThrowAsJavaScriptException();
        return {};
    }

    auto context = NodeZstdDecompressContextBinding::Unwrap(info[0].As<Napi::Object>());
    if (env.IsExceptionPending()) {
        return {};
    }

    return context->takeContext();
}

Napi::Object NodeZstdDecompressStreamBinding::initialize(Napi::Env env, Napi::Object exports) {
    auto func = DefineClass(env, "ZstdDecompressStreamBinding", {
            InstanceMethod("decompress", &NodeZstdDecompressStreamBinding::decompress),
            InstanceMethod("close", &NodeZstdDecompressStreamBinding::close),
    });

    s_constructor = Napi::Persistent(func);
    s_constructor.SuppressDestruct();

    exports.Set("ZstdDecompressStreamBinding", func);
    return exports;
}

NodeZstdDecompressStreamBinding::NodeZstdDecompressStreamBinding(const Napi::CallbackInfo &info)
        : ObjectWrap(info)
        , binding_(Binding::create({info.Env()}, NodeBinder({}, {}, {}), takeContextFrom(info)))
{
}

void NodeZstdDecompressStreamBinding::decompress(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() < 2 || !info[0].IsArrayBuffer() || !info[1].IsFunction()) {
        Napi::Error::New(info.Env(), "requires 2 argument. (data: ArrayBuffer, callback: (ArrayBuffer) => Void)").ThrowAsJavaScriptException();
        return;
    }

    auto data = info[0].As<Napi::ArrayBuffer>();
    auto callback = info[1].As<Napi::Function>();
    binding_->decompress({env}, data, callback);
}

void NodeZstdDecompressStreamBinding::close(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    binding_->close({env});
}
