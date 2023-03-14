#include "zstd-codec/binding/error.h"

#include "binding.h"
#include "compress.h"

////////////////////////////////////////////////////////////////////////////////
//
// NodeZstdCompressContextBinding
//
////////////////////////////////////////////////////////////////////////////////

Napi::FunctionReference NodeZstdCompressContextBinding::s_constructor;

std::unique_ptr<ZstdCompressContext> NodeZstdCompressContextBinding::takeContext() {
    return binding_->takeContext();
}

Napi::Object NodeZstdCompressContextBinding::initialize(Napi::Env env, Napi::Object exports) {
    auto func = DefineClass(env, "ZstdCompressContextBinding", {
            InstanceMethod("resetSession", &NodeZstdCompressContextBinding::resetSession),
            InstanceMethod("clearDictionary", &NodeZstdCompressContextBinding::clearDictionary),
            InstanceMethod("setCompressionLevel", &NodeZstdCompressContextBinding::setCompressionLevel),
            InstanceMethod("setChecksum", &NodeZstdCompressContextBinding::setChecksum),
            InstanceMethod("setOriginalSize", &NodeZstdCompressContextBinding::setOriginalSize),
    });

    s_constructor = Napi::Persistent(func);
    s_constructor.SuppressDestruct();

    exports.Set("ZstdCompressContextBinding", func);
    return exports;
}

NodeZstdCompressContextBinding::NodeZstdCompressContextBinding(const Napi::CallbackInfo &info)
        : ObjectWrap(info)
        , binding_(Binding::create({info.Env()}, NodeBinder({}, {}, {})))
{
}

Napi::Value NodeZstdCompressContextBinding::resetSession(const Napi::CallbackInfo& info) {
    binding_->updateContext({info.Env()}, &ZstdCompressContext::resetSession);
    return info.This();
}

Napi::Value NodeZstdCompressContextBinding::clearDictionary(const Napi::CallbackInfo& info) {
    binding_->updateContext({info.Env()}, &ZstdCompressContext::clearDictionary);
    return info.This();
}

Napi::Value NodeZstdCompressContextBinding::setCompressionLevel(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::Error::New(info.Env(), "requires 1 argument. (compressionLevel: number)").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto compression_level = info[0].As<Napi::Number>().Int32Value();
    binding_->updateContext({env}, [compression_level](ZstdCompressContext& context) {
        return context.setCompressionLevel(compression_level);
    });

    return info.This();
}

Napi::Value NodeZstdCompressContextBinding::setChecksum(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    if (info.Length() < 1 || !info[0].IsBoolean()) {
        Napi::Error::New(info.Env(), "requires 1 argument. (enable: boolean)").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto enable = info[0].As<Napi::Boolean>().Value();
    binding_->updateContext({env}, [enable](ZstdCompressContext& context) {
        return context.setChecksum(enable);
    });

    return info.This();
}

Napi::Value NodeZstdCompressContextBinding::setOriginalSize(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::Error::New(info.Env(), "requires 1 argument. (originalSize: number)").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto original_size = info[0].As<Napi::Number>().Int64Value();
    if (original_size < 0) {
        Napi::Error::New(env, "originalSize must be zero or positive number.").ThrowAsJavaScriptException();
        return env.Null();
    }

    binding_->updateContext({env}, [original_size](ZstdCompressContext& context) {
        return context.setOriginalSize(original_size);
    });

    return info.This();
}

////////////////////////////////////////////////////////////////////////////////
//
// NodeZstdCompressStreamBinding
//
////////////////////////////////////////////////////////////////////////////////

Napi::FunctionReference NodeZstdCompressStreamBinding::s_constructor;

std::unique_ptr<ZstdCompressContext> NodeZstdCompressStreamBinding::takeContextFrom(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() < 1) {
        Napi::Error::New(env, "requires 1 argument. (contextBinding: ZstdCompressContextBinding)")
                .ThrowAsJavaScriptException();
        return {};
    }

    auto context = NodeZstdCompressContextBinding::Unwrap(info[0].As<Napi::Object>());
    if (env.IsExceptionPending()) {
        return {};
    }

    return context->takeContext();
}

Napi::Object NodeZstdCompressStreamBinding::initialize(Napi::Env env, Napi::Object exports) {
    auto func = DefineClass(env, "ZstdCompressStreamBinding", {
            InstanceMethod("compress", &NodeZstdCompressStreamBinding::compress),
            InstanceMethod("flush", &NodeZstdCompressStreamBinding::flush),
            InstanceMethod("complete", &NodeZstdCompressStreamBinding::complete),
            InstanceMethod("close", &NodeZstdCompressStreamBinding::close),
    });

    s_constructor = Napi::Persistent(func);
    s_constructor.SuppressDestruct();

    exports.Set("ZstdCompressStreamBinding", func);
    return exports;
}

NodeZstdCompressStreamBinding::NodeZstdCompressStreamBinding(const Napi::CallbackInfo &info)
        : ObjectWrap(info)
        , binding_(Binding::create({info.Env()}, NodeBinder({}, {}, {}), takeContextFrom(info)))
{
}

void NodeZstdCompressStreamBinding::compress(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() < 2 || !info[0].IsArrayBuffer() || !info[1].IsFunction()) {
        Napi::Error::New(info.Env(), "requires 2 argument. (data: ArrayBuffer, callback: (ArrayBuffer) => Void)").ThrowAsJavaScriptException();
        return;
    }

    auto data = info[0].As<Napi::ArrayBuffer>();
    auto callback = info[1].As<Napi::Function>();
    binding_->compress({env}, data, callback);
}

void NodeZstdCompressStreamBinding::flush(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::Error::New(info.Env(), "requires 1 argument. (callback: (ArrayBuffer) => Void)").ThrowAsJavaScriptException();
        return;
    }

    auto callback = info[0].As<Napi::Function>();
    binding_->flush({env}, callback);
}

void NodeZstdCompressStreamBinding::complete(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::Error::New(info.Env(), "requires 1 argument. (callback: (ArrayBuffer) => Void)").ThrowAsJavaScriptException();
        return;
    }

    auto callback = info[0].As<Napi::Function>();
    binding_->complete({env}, callback);
}

void NodeZstdCompressStreamBinding::close(const Napi::CallbackInfo &info) {
    auto env = info.Env();
    binding_->close({env});
}
