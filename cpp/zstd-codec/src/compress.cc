#include <cassert>
#include <memory>
#include <vector>

#include "fmt/format.h"
#include "zstd.h"

#include "zstd-codec/compress.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// context for compression
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static ZstdCompressContextResult<void> updateContext(const char* op, std::function<size_t()>&& f) {
    const auto code = f();
    if (ZSTD_isError(code)) {
        const auto error = ZSTD_getErrorName(code);
        return tl::make_unexpected(ZstdCompressContextError {
                tl::make_optional(code),
                fmt::format("context error. op={}, error={}", op, error),
        });
    }

    return {};
}

static ZstdCompressContextResult<void> setParameter(ZSTD_CCtx_s *context, ZSTD_cParameter param, int value) {
    return updateContext("ZSTD_CCtx_setParameter", [=]() {
        return ZSTD_CCtx_setParameter(context, param, value);
    });
}

ZstdCompressContextResult<std::unique_ptr<ZstdCompressContext>> ZstdCompressContext::create() {
    std::unique_ptr<ZstdCompressContext> context(new ZstdCompressContext);
    context->context_ = ZSTD_createCCtx();
    if (context->context_ == nullptr) {
        return tl::make_unexpected(ZstdCompressContextError{
                tl::nullopt,
                "failed to allocate a ZSTD_CCtx object."
        });
    }

    auto r = context->resetSession();
    if (!r) {
        return tl::make_unexpected(r.error());
    }

    return context;
}

ZstdCompressContext::~ZstdCompressContext() {
    if (context_) {
        const auto r = close();

        // detect error on debug build
        assert(bool(r));
    }
}

ZSTD_CCtx_s *ZstdCompressContext::context() const {
    return context_;
}

ZstdCompressContextResult<void> ZstdCompressContext::close() {
    auto r = updateContext("ZSTD_freeCCtx", [this]() {
        return ZSTD_freeCCtx(context_);
    });
    if (r) {
        context_ = nullptr;
    }

    return r;
}

ZstdCompressContextResult<void> ZstdCompressContext::resetSession() {
    return updateContext("ZSTD_CCtx_reset(ZSTD_reset_session_only)", [this]() {
        return ZSTD_CCtx_reset(context_, ZSTD_reset_session_only);
    });
}

ZstdCompressContextResult<void> ZstdCompressContext::clearDictionary() {
    return updateContext("ZSTD_CCtx_refCDict(nullptr)", [this]() {
        return ZSTD_CCtx_refCDict(context_, nullptr);
    });
}

ZstdCompressContextResult<void> ZstdCompressContext::setCompressionLevel(int compression_level) {
    return setParameter(context_, ZSTD_c_compressionLevel, compression_level);
}

ZstdCompressContextResult<void> ZstdCompressContext::setChecksum(bool enable) {
    return setParameter(context_, ZSTD_c_checksumFlag, enable ? 1 : 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// compress stream
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static ZstdCompressStreamError compressStreamError(size_t code, const char* op) {
    const auto error = ZSTD_getErrorName(code);
    return ZstdCompressStreamError {
        code,
        fmt::format("compress stream error. op={}, error={}", op, error),
    };
}

static ZstdCompressStreamError compressStreamErrorFrom(const ZstdCompressContextError& e) {
    return ZstdCompressStreamError {
        e.code,
        e.message,
    };
}

class ZstdCompressStreamImpl {
public:
    std::unique_ptr<ZstdCompressContext> context_;
    std::vector<uint8_t> out_buffer_;

    explicit ZstdCompressStreamImpl(std::unique_ptr<ZstdCompressContext>&& context)
        : context_(std::forward<std::unique_ptr<ZstdCompressContext>>(context))
        , out_buffer_()
    {
    }

    ZSTD_outBuffer output() {
        out_buffer_.clear();
        out_buffer_.resize(ZSTD_CStreamOutSize());

        return ZSTD_outBuffer { &out_buffer_[0], out_buffer_.size(), 0 };
    }

    ZstdCompressStreamResult<void> setOriginalSize(uint64_t original_size) const {
        const auto code = ZSTD_CCtx_setPledgedSrcSize(context_->context(), original_size);
        if (ZSTD_isError(code)) {
            return tl::make_unexpected(compressStreamError(code, "ZSTD_CCtx_setPledgedSrcSize"));
        }

        return {};
    }

    ZstdCompressStreamResult<void> compress(ZSTD_inBuffer input, ZSTD_EndDirective end_op, const CompressStreamCallback& callback) {
        bool finished = false;
        while (!finished) {
            auto output = this->output();
            const auto remaining = ZSTD_compressStream2(context_->context(), &output, &input, end_op);
            finished = remaining == 0 || input.pos == input.size;
            if (output.pos != 0) {
                out_buffer_.resize(output.pos);
                callback(out_buffer_);
            }
        }

        return {};
    }
};

ZstdCompressStream::ZstdCompressStream(std::unique_ptr<ZstdCompressStreamImpl>&& pimpl)
        : pimpl_(std::forward<std::unique_ptr<ZstdCompressStreamImpl>>(pimpl))
{
}

ZstdCompressStream::~ZstdCompressStream() = default;

ZstdCompressStreamResult<std::unique_ptr<ZstdCompressStream>>
ZstdCompressStream::fromContext(std::unique_ptr<ZstdCompressContext>&& context) {
    const auto r = context->resetSession().map_error(compressStreamErrorFrom);
    if (!r) {
        return tl::make_unexpected(r.error());
    }

    auto pimpl = std::make_unique<ZstdCompressStreamImpl>(std::move(context));
    return std::unique_ptr<ZstdCompressStream>(new ZstdCompressStream(std::move(pimpl)));
}

ZstdCompressStreamResult<std::unique_ptr<ZstdCompressStream>>
ZstdCompressStream::withOriginalSize(std::unique_ptr<ZstdCompressStream> && s, uint64_t original_size) {
    return s->pimpl_->setOriginalSize(original_size)
            .map([&s](){
                return std::forward<std::unique_ptr<ZstdCompressStream>>(s);
            });
}

ZstdCompressStreamResult<std::unique_ptr<ZstdCompressStream>>
ZstdCompressStream::unbounded(std::unique_ptr<ZstdCompressContext> context) {
    return fromContext(std::move(context));
}

ZstdCompressStreamResult<std::unique_ptr<ZstdCompressStream>>
ZstdCompressStream::sized(std::unique_ptr<ZstdCompressContext> context, uint64_t original_size) {
    return ZstdCompressStream::fromContext(std::move(context))
            .and_then([original_size](std::unique_ptr<ZstdCompressStream> && s){
                return withOriginalSize(std::forward<std::unique_ptr<ZstdCompressStream>>(s), original_size);
            });
}

ZstdCompressStreamResult<void>
ZstdCompressStream::compress(const std::vector<uint8_t> &data, const CompressStreamCallback &callback) {
    if (data.empty()) {
        return {};
    }

    size_t i = 0;
    const auto max_input = ZSTD_CStreamInSize();
    while (i < data.size()) {
        const auto take = i + max_input < data.size() ? max_input : data.size() - i;
        ZSTD_inBuffer input = { &data[i], take, 0 };
        i += take;

        const auto r = pimpl_->compress(input, ZSTD_e_continue, callback);
        if (!r) {
            return tl::make_unexpected(r.error());
        }
    }

    return {};
}

ZstdCompressStreamResult<void> ZstdCompressStream::flush(const CompressStreamCallback& callback) {
    ZSTD_inBuffer input = { nullptr, 0, 0 };
    return pimpl_->compress(input, ZSTD_e_flush, callback);
}

ZstdCompressStreamResult<void> ZstdCompressStream::complete(const CompressStreamCallback& callback) {
    ZSTD_inBuffer input = { nullptr, 0, 0 };
    return pimpl_->compress(input, ZSTD_e_end, callback);
}

ZstdCompressStreamResult<void> ZstdCompressStream::close() {
    return pimpl_->context_->close()
            .map_error(compressStreamErrorFrom);
}
