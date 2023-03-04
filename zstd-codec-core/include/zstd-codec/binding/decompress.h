#pragma once

#include <cassert>
#include <functional>

#include "bindable.h"
#include "binder.h"
#include "error.h"

#include "../decompress.h"

template <BytesBindable B, BytesCallbackBindable C, ErrorThrowable E>
class ZstdDecompressContextBinding {
private:
    ZstdCodecBinder<B, C, E> binding_;
    std::unique_ptr<ZstdDecompressContext> context_;

    static ZstdCodecBindingError errorFrom(ZstdDecompressContextError&& e) {
        return ZstdCodecBindingError {
                e.code,
                std::move(e.message),
        };
    }

    void throwError(ZstdDecompressContextError&& e) {
        const auto error = errorFrom(std::forward<ZstdDecompressContextError&&>(e));
        binding_.throwError(error);
    }

    ZstdDecompressContextBinding(ZstdCodecBinder<B, C, E>&& binder, std::unique_ptr<ZstdDecompressContext>&& context)
            : binding_(binder)
            , context_(std::forward<std::unique_ptr<ZstdDecompressContext>>(context))
    {
    }

    typedef ZstdDecompressContextResult<void> (ZstdDecompressContext::*ContextAction)();

    void updateContext(ContextAction action) {
        assert(context_ != nullptr && "cannot update empty context.");
        auto result = ((*context_).*action)();
        if (!result) {
            throwError(std::forward<ZstdDecompressContextError>(result.error()));
        }
    }

    void updateContext(std::function<ZstdDecompressContextResult<void>()>&& action) {
        assert(context_ != nullptr && "cannot update empty context.");
        auto result = action();
        if (!result) {
            throwError(std::forward<ZstdDecompressContextError>(result.error()));
        }
    }

public:
    using BinderType = ZstdCodecBinder<B, C, E>;
    using BindingType = ZstdDecompressContextBinding<B, C, E>;
    using ContextPtr = std::unique_ptr<ZstdDecompressContext>;

    static std::unique_ptr<BindingType> create(BinderType binder) {
        auto r = ZstdDecompressContext::create();
        if (!r) {
            const auto error = errorFrom(std::forward<ZstdDecompressContextError&&>(r.error()));
            binder.throwError(error);
            return nullptr;
        }

        auto binding = new ZstdDecompressContextBinding(std::move(binder), std::forward<ContextPtr>(*r));
        return std::unique_ptr<BindingType>(binding);
    }

    std::unique_ptr<ZstdDecompressContext> takeContext() {
        assert(context_ != nullptr && "cannot update empty context.");
        return std::unique_ptr<ZstdDecompressContext>(context_.release());
    }

    void resetSession() {
        updateContext(&ZstdDecompressContext::resetSession);
    }

    void clearDictionary() {
        updateContext(&ZstdDecompressContext::clearDictionary);
    }
};

template <BytesBindable B, BytesCallbackBindable C, ErrorThrowable E>
class ZstdDecompressStreamBinding {
private:
    ZstdCodecBinder<B, C, E> binding_;
    std::unique_ptr<ZstdDecompressStream> stream_;

    static ZstdCodecBindingError errorFrom(ZstdDecompressStreamError&& e) {
        return ZstdCodecBindingError {
                e.code,
                std::move(e.message),
        };
    }

    void throwError(ZstdDecompressStreamError&& e) {
        const auto error = errorFrom(std::forward<ZstdDecompressStreamError&&>(e));
        binding_.throwError(error);
    }

    ZstdDecompressStreamBinding(ZstdCodecBinder<B, C, E>&& binder, std::unique_ptr<ZstdDecompressStream>&& stream)
            : binding_(binder)
            , stream_(std::forward<std::unique_ptr<ZstdDecompressStream>>(stream))
    {
    }

public:
    using BinderType = ZstdCodecBinder<B, C, E>;
    using BindingType = ZstdDecompressStreamBinding<B, C, E>;
    using ContextPtr = std::unique_ptr<ZstdDecompressContext>;
    using StreamPtr = std::unique_ptr<ZstdDecompressStream>;

    static std::unique_ptr<BindingType> create(BinderType binder, ContextPtr context) {
        auto r = ZstdDecompressStream::withContext(std::move(context));
        if (!r) {
            const auto error = errorFrom(std::forward<ZstdDecompressStreamError&&>(r.error()));
            binder.throwError(error);
            return nullptr;
        }

        auto binding = new ZstdDecompressStreamBinding(std::move(binder), std::forward<StreamPtr>(*r));
        return std::unique_ptr<BindingType>(binding);
    }

    void decompress(const typename B::WireType& data, const typename C::WireType& callback) {
        auto compressed = binding_.intoBytesVector(data);
        auto callback_fn = binding_.intoBytesCallbackFn(callback);
        auto result = stream_->decompress(compressed, callback_fn);
        if (!result) {
            throwError(std::forward<ZstdDecompressStreamError>(result.error()));
            return;
        }
    }

    void close() {
        auto result = stream_->close();
        if (!result) {
            throwError(std::forward<ZstdDecompressStreamError>(result.error()));
            return;
        }
    }
};
