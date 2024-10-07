#pragma once
#include <napi.h>

namespace zstd_codec::nodejs {
    class ZstdCodecMetadataNodejsObject: public Napi::ObjectWrap<ZstdCodecMetadataNodejsObject> {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
        ZstdCodecMetadataNodejsObject(const Napi::CallbackInfo& info);

    private:
        Napi::Value zstd_version_string(const Napi::CallbackInfo& info);
        Napi::Value zstd_codec_version_string(const Napi::CallbackInfo& info);
    };
} // zstd_codec
