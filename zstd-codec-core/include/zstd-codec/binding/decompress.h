#pragma once

#include <cassert>
#include <functional>

#include "bindable.h"
#include "binder.h"
#include "error.h"

#include "../decompress.h"

template <Contextual C, BytesBindable<C> B, BytesCallbackBindable<C> F, ErrorThrowable<C> E>
class ZstdDecompressContextBinding {
private:
    ZstdCodecBinder<C, B, F, E> binding_;
    std::unique_ptr<ZstdDecompressContext> context_;

    static ZstdCodecBindingError errorFrom(ZstdDecompressContextError&& e) {
        return ZstdCodecBindingError {
                e.code,
                std::move(e.message),
        };
    }

    void throwError(typename C::WireContext wire_context, ZstdDecompressContextError&& e) {
        const auto error = errorFrom(std::forward<ZstdDecompressContextError&&>(e));
        binding_.throwError(wire_context, error);
    }

    ZstdDecompressContextBinding(ZstdCodecBinder<C, B, F, E>&& binder, std::unique_ptr<ZstdDecompressContext>&& context)
            : binding_(binder)
            , context_(std::forward<std::unique_ptr<ZstdDecompressContext>>(context))
    {
    }

    typedef ZstdDecompressContextResult<void> (ZstdDecompressContext::*ContextAction)();

    void updateContext(typename C::WireContext wire_context, ContextAction action) {
        assert(context_ != nullptr && "cannot update empty context.");
        auto result = ((*context_).*action)();
        if (!result) {
            throwError(wire_context, std::forward<ZstdDecompressContextError>(result.error()));
        }
    }

    void updateContext(typename C::WireContext wire_context, std::function<ZstdDecompressContextResult<void>()>&& action) {
        assert(context_ != nullptr && "cannot update empty context.");
        auto result = action();
        if (!result) {
            throwError(wire_context, std::forward<ZstdDecompressContextError>(result.error()));
        }
    }

public:
    using BinderType = ZstdCodecBinder<C, B, F, E>;
    using BindingType = ZstdDecompressContextBinding<C, B, F, E>;
    using ContextPtr = std::unique_ptr<ZstdDecompressContext>;

    static std::unique_ptr<BindingType> create(typename C::WireContext wire_context, BinderType binder) {
        auto r = ZstdDecompressContext::create();
        if (!r) {
            const auto error = errorFrom(std::forward<ZstdDecompressContextError&&>(r.error()));
            binder.throwError(wire_context, error);
            return nullptr;
        }

        auto binding = new ZstdDecompressContextBinding(std::move(binder), std::forward<ContextPtr>(*r));
        return std::unique_ptr<BindingType>(binding);
    }

    std::unique_ptr<ZstdDecompressContext> takeContext() {
        assert(context_ != nullptr && "cannot update empty context.");
        return std::unique_ptr<ZstdDecompressContext>(context_.release());
    }

    void resetSession(typename C::WireContext wire_context) {
        updateContext(wire_context, &ZstdDecompressContext::resetSession);
    }

    void clearDictionary(typename C::WireContext wire_context) {
        updateContext(wire_context, &ZstdDecompressContext::clearDictionary);
    }
};

template <Contextual C, BytesBindable<C> B, BytesCallbackBindable<C> F, ErrorThrowable<C> E>
class ZstdDecompressStreamBinding {
private:
    ZstdCodecBinder<C, B, F, E> binding_;
    std::unique_ptr<ZstdDecompressStream> stream_;

    static ZstdCodecBindingError errorFrom(ZstdDecompressStreamError&& e) {
        return ZstdCodecBindingError {
                e.code,
                std::move(e.message),
        };
    }

    void throwError(typename C::WireContext wire_context, ZstdDecompressStreamError&& e) {
        const auto error = errorFrom(std::forward<ZstdDecompressStreamError&&>(e));
        binding_.throwError(wire_context, error);
    }

    ZstdDecompressStreamBinding(ZstdCodecBinder<C, B, F, E>&& binder, std::unique_ptr<ZstdDecompressStream>&& stream)
            : binding_(binder)
            , stream_(std::forward<std::unique_ptr<ZstdDecompressStream>>(stream))
    {
    }

public:
    using BinderType = ZstdCodecBinder<C, B, F, E>;
    using BindingType = ZstdDecompressStreamBinding<C, B, F, E>;
    using ContextPtr = std::unique_ptr<ZstdDecompressContext>;
    using StreamPtr = std::unique_ptr<ZstdDecompressStream>;

    static std::unique_ptr<BindingType> create(typename C::WireContext wire_context, BinderType binder, ContextPtr context) {
        auto r = ZstdDecompressStream::withContext(std::move(context));
        if (!r) {
            const auto error = errorFrom(std::forward<ZstdDecompressStreamError&&>(r.error()));
            binder.throwError(wire_context, error);
            return nullptr;
        }

        auto binding = new ZstdDecompressStreamBinding(std::move(binder), std::forward<StreamPtr>(*r));
        return std::unique_ptr<BindingType>(binding);
    }

    void decompress(typename C::WireContext wire_context, const typename B::WireType& data, const typename F::WireType& callback) {
        auto compressed = binding_.intoBytesVector(wire_context, data);
        auto callback_fn = binding_.intoBytesCallbackFn(wire_context, callback);
        auto result = stream_->decompress(compressed, callback_fn);
        if (!result) {
            throwError(wire_context, std::forward<ZstdDecompressStreamError>(result.error()));
            return;
        }
    }

    void close(typename C::WireContext wire_context) {
        auto result = stream_->close();
        if (!result) {
            throwError(wire_context, std::forward<ZstdDecompressStreamError>(result.error()));
            return;
        }
    }
};
