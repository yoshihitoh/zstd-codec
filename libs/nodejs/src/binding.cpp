#include "zstd-codec/nodejs/binding.h"
#include "zstd-codec/core/metadata.h"
#include "napi.h"

using namespace zstd_codec::core;

namespace zstd_codec::nodejs {
    Napi::Object ZstdCodecMetadataNodejsObject::Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "ZstdCodecMetadata", {
            InstanceMethod("zstdVersionString", &ZstdCodecMetadataNodejsObject::zstd_version_string),
            InstanceMethod("zstdCodecVersionString", &ZstdCodecMetadataNodejsObject::zstd_codec_version_string),
        });

        auto constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("ZstdCodecMetadata", func);
        return exports;
    }

    ZstdCodecMetadataNodejsObject::ZstdCodecMetadataNodejsObject(const Napi::CallbackInfo &info)
        : Napi::ObjectWrap<ZstdCodecMetadataNodejsObject>(info)
    {
    }

    Napi::Value ZstdCodecMetadataNodejsObject::zstd_version_string(const Napi::CallbackInfo &info) {
        return Napi::String::New(info.Env(), ZstdCodecMetadata::zstd_version_string());
    }

    Napi::Value ZstdCodecMetadataNodejsObject::zstd_codec_version_string(const Napi::CallbackInfo &info) {
        return Napi::String::New(info.Env(), ZstdCodecMetadata::zstd_codec_version_string());
    }
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    return zstd_codec::nodejs::ZstdCodecMetadataNodejsObject::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)
