#pragma once

#include <cassert>
#include <functional>

#include "bindable.h"
#include "binder.h"
#include "error.h"

#include "../compress.h"

template <Contextual C, BytesBindable<C> B, BytesCallbackBindable<C> F, ErrorThrowable<C> E>
class ZstdCompressContextBinding {
private:
    ZstdCodecBinder<C, B, F, E> binding_;
    std::unique_ptr<ZstdCompressContext> context_;

    static ZstdCodecBindingError errorFrom(ZstdCompressContextError&& e) {
        return ZstdCodecBindingError {
                e.code,
                std::move(e.message),
        };
    }

    ZstdCompressContextBinding(ZstdCodecBinder<C, B, F, E>&& binder, std::unique_ptr<ZstdCompressContext>&& context)
            : binding_(binder)
            , context_(std::forward<std::unique_ptr<ZstdCompressContext>>(context))
    {
    }

    void throwError(typename C::WireContext wire_context, ZstdCompressContextError&& e) {
        const auto error = errorFrom(std::forward<ZstdCompressContextError&&>(e));
        binding_.throwError(wire_context, error);
    }

public:
    using BinderType = ZstdCodecBinder<C, B, F, E>;
    using BindingType = ZstdCompressContextBinding<C, B, F, E>;
    using ContextPtr = std::unique_ptr<ZstdCompressContext>;

    typedef ZstdCompressContextResult<void> (ZstdCompressContext::*ContextAction)();

    static std::unique_ptr<BindingType> create(typename C::WireContext wire_context, BinderType binder) {
        auto r = ZstdCompressContext::create();
        if (!r) {
            const auto error = errorFrom(std::forward<ZstdCompressContextError&&>(r.error()));
            binder.throwError(wire_context, error);
            return nullptr;
        }

        auto binding = new ZstdCompressContextBinding(std::move(binder), std::forward<ContextPtr>(*r));
        return std::unique_ptr<BindingType>(binding);
    }

    void updateContext(typename C::WireContext wire_context, ContextAction action) {
        assert(context_ != nullptr && "cannot update empty context.");
        auto result = ((*context_).*action)();
        if (!result) {
            throwError(wire_context, std::forward<ZstdCompressContextError>(result.error()));
        }
    }

    void updateContext(typename C::WireContext wire_context, std::function<ZstdCompressContextResult<void>(ZstdCompressContext&)>&& action) {
        assert(context_ != nullptr && "cannot update empty context.");
        auto result = action(*context_);
        if (!result) {
            throwError(wire_context, std::forward<ZstdCompressContextError>(result.error()));
        }
    }

    std::unique_ptr<ZstdCompressContext> takeContext() {
        assert(context_ != nullptr && "cannot update empty context.");
        return std::unique_ptr<ZstdCompressContext>(context_.release());
    }
};

template <Contextual C, BytesBindable<C> B, BytesCallbackBindable<C> F, ErrorThrowable<C> E>
class ZstdCompressStreamBinding {
private:
    ZstdCodecBinder<C, B, F, E> binding_;
    std::unique_ptr<ZstdCompressStream> stream_;

    static ZstdCodecBindingError errorFrom(ZstdCompressStreamError&& e) {
        return ZstdCodecBindingError {
          e.code,
          std::move(e.message),
        };
    }

    void throwError(typename C::WireContext wire_context, ZstdCompressStreamError&& e) {
        const auto error = errorFrom(std::forward<ZstdCompressStreamError&&>(e));
        binding_.throwError(wire_context, error);
    }

    ZstdCompressStreamBinding(ZstdCodecBinder<C, B, F, E>&& binder, std::unique_ptr<ZstdCompressStream>&& stream)
            : binding_(binder)
            , stream_(std::forward<std::unique_ptr<ZstdCompressStream>>(stream))
    {
    }

public:
    using BinderType = ZstdCodecBinder<C, B, F, E>;
    using BindingType = ZstdCompressStreamBinding<C, B, F, E>;
    using ContextPtr = std::unique_ptr<ZstdCompressContext>;
    using StreamPtr = std::unique_ptr<ZstdCompressStream>;

    static std::unique_ptr<BindingType> create(typename C::WireContext wire_context, BinderType binder, ContextPtr context) {
        auto r = ZstdCompressStream::withContext(std::move(context));
        if (!r) {
            const auto error = errorFrom(std::forward<ZstdCompressStreamError&&>(r.error()));
            binder.throwError(wire_context, error);
            return nullptr;
        }

        auto binding = new ZstdCompressStreamBinding(std::move(binder), std::forward<StreamPtr>(*r));
        return std::unique_ptr<BindingType>(binding);
    }

    void compress(typename C::WireContext wire_context, const typename B::WireType& data, const typename F::WireType& callback) {
        auto original = binding_.intoBytesVector(wire_context, data);
        auto callback_fn = binding_.intoBytesCallbackFn(wire_context, callback);
        auto result = stream_->compress(original, callback_fn);
        if (!result) {
            throwError(wire_context, std::forward<ZstdCompressStreamError>(result.error()));
            return;
        }
    }

    void flush(typename C::WireContext wire_context, const typename F::WireType& callback) {
        auto callback_fn = binding_.intoBytesCallbackFn(wire_context, callback);
        auto result = stream_->flush(callback_fn);
        if (!result) {
            throwError(wire_context, std::forward<ZstdCompressStreamError>(result.error()));
            return;
        }
    }

    void complete(typename C::WireContext wire_context, const typename F::WireType& callback) {
        auto callback_fn = binding_.intoBytesCallbackFn(wire_context, callback);
        auto result = stream_->complete(callback_fn);
        if (!result) {
            throwError(wire_context, std::forward<ZstdCompressStreamError>(result.error()));
            return;
        }
    }

    void close(typename C::WireContext wire_context) {
        auto result = stream_->close();
        if (!result) {
            throwError(wire_context, std::forward<ZstdCompressStreamError>(result.error()));
            return;
        }
    }
};
