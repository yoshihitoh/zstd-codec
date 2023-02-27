#pragma once

#include <cassert>
#include <functional>

#include "bindable.h"
#include "binder.h"
#include "error.h"

#include "../compress.h"

template <BytesBindable B, BytesCallbackBindable C, ErrorThrowable E>
class ZstdCompressContextBinding {
private:
    ZstdCodecBinder<B, C, E> binding_;
    std::unique_ptr<ZstdCompressContext> context_;

    static ZstdCodecBindingError errorFrom(ZstdCompressContextError&& e) {
        return ZstdCodecBindingError {
                e.code,
                std::move(e.message),
        };
    }

    void throwError(ZstdCompressContextError&& e) {
        const auto error = errorFrom(std::forward<ZstdCompressContextError&&>(e));
        binding_.throwError(error);
    }

    ZstdCompressContextBinding(ZstdCodecBinder<B, C, E>&& binder, std::unique_ptr<ZstdCompressContext>&& context)
            : binding_(binder)
            , context_(std::forward<std::unique_ptr<ZstdCompressContext>>(context))
    {
    }

    typedef ZstdCompressContextResult<void> (ZstdCompressContext::*ContextAction)();

    void updateContext(ContextAction action) {
        assert(context_ != nullptr && "cannot update empty context.");
        auto result = ((*context_).*action)();
        if (!result) {
            throwError(std::forward<ZstdCompressContextError>(result.error()));
        }
    }

    void updateContext(std::function<ZstdCompressContextResult<void>()>&& action) {
        assert(context_ != nullptr && "cannot update empty context.");
        auto result = action();
        if (!result) {
            throwError(std::forward<ZstdCompressContextError>(result.error()));
        }
    }

public:
    using BinderType = ZstdCodecBinder<B, C, E>;
    using BindingType = ZstdCompressContextBinding<B, C, E>;
    using ContextPtr = std::unique_ptr<ZstdCompressContext>;

    static std::unique_ptr<BindingType> create(BinderType binder) {
        auto r = ZstdCompressContext::create();
        if (!r) {
            const auto error = errorFrom(std::forward<ZstdCompressContextError&&>(r.error()));
            binder.throwError(error);
            return nullptr;
        }

        auto binding = new ZstdCompressContextBinding(std::move(binder), std::forward<ContextPtr>(*r));
        return std::unique_ptr<BindingType>(binding);
    }

    std::unique_ptr<ZstdCompressContext> takeContext() {
        assert(context_ != nullptr && "cannot update empty context.");
        return std::unique_ptr<ZstdCompressContext>(context_.release());
    }

    void resetSession() {
        updateContext(&ZstdCompressContext::resetSession);
    }

    void clearDictionary() {
        updateContext(&ZstdCompressContext::clearDictionary);
    }

    void setCompressionLevel(int compression_level) {
        updateContext([this, compression_level]() {
            return context_->setCompressionLevel(compression_level);
        });
    }

    void setChecksum(bool enable) {
        updateContext([this, enable]() {
            return context_->setChecksum(enable);
        });
    }

    void setOriginalSize(uint64_t original_size) {
        updateContext([this, original_size]() {
            return context_->setOriginalSize(original_size);
        });
    }
};

template <BytesBindable B, BytesCallbackBindable C, ErrorThrowable E>
class ZstdCompressStreamBinding {
private:
    ZstdCodecBinder<B, C, E> binding_;
    std::unique_ptr<ZstdCompressStream> stream_;

    static ZstdCodecBindingError errorFrom(ZstdCompressStreamError&& e) {
        return ZstdCodecBindingError {
          e.code,
          std::move(e.message),
        };
    }

    void throwError(ZstdCompressStreamError&& e) {
        const auto error = errorFrom(std::forward<ZstdCompressStreamError&&>(e));
        binding_.throwError(error);
    }

    ZstdCompressStreamBinding(ZstdCodecBinder<B, C, E>&& binder, std::unique_ptr<ZstdCompressStream>&& stream)
            : binding_(binder)
            , stream_(std::forward<std::unique_ptr<ZstdCompressStream>>(stream))
    {
    }

public:
    using BinderType = ZstdCodecBinder<B, C, E>;
    using BindingType = ZstdCompressStreamBinding<B, C, E>;
    using ContextPtr = std::unique_ptr<ZstdCompressContext>;
    using StreamPtr = std::unique_ptr<ZstdCompressStream>;

    static std::unique_ptr<BindingType> create(BinderType binder, ContextPtr context) {
        auto r = ZstdCompressStream::withContext(std::move(context));
        if (!r) {
            const auto error = errorFrom(std::forward<ZstdCompressStreamError&&>(r.error()));
            binder.throwError(error);
            return nullptr;
        }

        auto binding = new ZstdCompressStreamBinding(std::move(binder), std::forward<StreamPtr>(*r));
        return std::unique_ptr<BindingType>(binding);
    }

    void compress(const typename B::WireType& data, const typename C::WireType& callback) {
        auto original = binding_.intoBytesVector(data);
        auto callback_fn = binding_.intoBytesCallbackFn(callback);
        auto result = stream_->compress(original, callback_fn);
        if (!result) {
            throwError(std::forward<ZstdCompressStreamError>(result.error()));
            return;
        }
    }

    void flush(const typename C::WireType& callback) {
        auto callback_fn = binding_.intoBytesCallbackFn(callback);
        auto result = stream_->flush(callback_fn);
        if (!result) {
            throwError(std::forward<ZstdCompressStreamError>(result.error()));
            return;
        }
    }

    void complete(const typename C::WireType& callback) {
        auto callback_fn = binding_.intoBytesCallbackFn(callback);
        auto result = stream_->complete(callback_fn);
        if (!result) {
            throwError(std::forward<ZstdCompressStreamError>(result.error()));
            return;
        }
    }

    void close() {
        auto result = stream_->close();
        if (!result) {
            throwError(std::forward<ZstdCompressStreamError>(result.error()));
            return;
        }
    }
};
