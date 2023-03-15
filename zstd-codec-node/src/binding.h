#pragma once

#include <cstdint>
#include <vector>

#include "napi.h"
#include "fmt/format.h"

#include "zstd-codec/binding/bindable.h"
#include "zstd-codec/binding/binder.h"
#include "zstd-codec/binding/error.h"

struct NapiEnvContext {
    Napi::Env env;
};

struct NodeContext {
    using WireContext = NapiEnvContext;
};

template <Contextual C>
struct NodeBytesBinding {};

template <>
struct NodeBytesBinding<NodeContext> {
    using WireType = Napi::Value;
    using ValueType = std::vector<uint8_t>;

    ValueType fromWireType(NodeContext::WireContext, WireType wire) {
        assert(wire.IsArrayBuffer());
        auto array = wire.As<Napi::ArrayBuffer>();
        auto data = static_cast<const uint8_t*>(array.Data());
        return {data, data + array.ByteLength()};
    }

    WireType intoWireType(NodeContext::WireContext context, ValueType value) {
        return Napi::ArrayBuffer::New(context.env, value.data(), value.size());
    }
};

template <Contextual C>
struct NodeBytesCallbackBinding {};

template <>
struct NodeBytesCallbackBinding<NodeContext> {
    using WireType = Napi::Value;
    using ValueType = std::function<void(const std::vector<uint8_t>&)>;

    NodeBytesBinding<NodeContext> bytes_;

    ValueType fromWireType(NodeContext::WireContext wire_context, WireType wire) {
        assert(wire.IsFunction());
        return [this, wire_context, wire](const std::vector<uint8_t>& data) {
            auto wired_data = bytes_.intoWireType(wire_context, data);

            auto fn = wire.As<Napi::Function>();
            auto args = std::vector<Napi::Value> { wired_data };
            fn.Call(args);
        };
    }
};

template <Contextual C>
struct NodeErrorThrowable {};

template <>
struct NodeErrorThrowable<NodeContext> {
    NodeErrorThrowable<NodeContext>() {}

    void throwError(NodeContext::WireContext wire_context, const ZstdCodecBindingError& error) {
        auto code = error.code ? fmt::format("Some({})", *error.code) : fmt::format("None");
        auto message = fmt::format("code={}, message={}", code, error.message);

        auto env = wire_context.env;
        Napi::Error::New(env, message.c_str()).ThrowAsJavaScriptException();
    }
};

using NodeBinder = ZstdCodecBinder<
        NodeContext,
        NodeBytesBinding<NodeContext>,
        NodeBytesCallbackBinding<NodeContext>,
        NodeErrorThrowable<NodeContext>
>;
