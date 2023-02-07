#include <cassert>
#include <cstdint>
#include <vector>

#include "fmt/format.h"
#include "zstd.h"

#include "zstd-codec/decompress.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// context for decompression
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static ZstdDecompressContextResult<void> updateContext(const char* op, std::function<size_t()>&& f) {
    const auto code = f();
    if (ZSTD_isError(code)) {
        const auto error = ZSTD_getErrorName(code);
        return tl::make_unexpected(ZstdDecompressContextError {
                tl::make_optional(code),
                fmt::format("context error. op={}, error={}", op, error),
        });
    }

    return {};
}

tl::expected<std::unique_ptr<ZstdDecompressContext>, ZstdDecompressContextError> ZstdDecompressContext::create() {
    std::unique_ptr<ZstdDecompressContext> context(new ZstdDecompressContext);
    context->context_ = ZSTD_createDCtx();
    if (context->context_ == nullptr) {
        return tl::make_unexpected(ZstdDecompressContextError {
            tl::nullopt,
            "failed to allocate a ZSTD_DCtx object."
        });
    }

    auto r = context->resetSession();
    if (!r) {
        return tl::make_unexpected(r.error());
    }

    return context;
}

ZstdDecompressContext::~ZstdDecompressContext() {
    if (context_) {
        const auto r = close();

        // detect error on debug build
        assert(bool(r));
    }
}

ZSTD_DCtx_s *ZstdDecompressContext::context() const {
    return context_;
}

ZstdDecompressContextResult<void> ZstdDecompressContext::close() {
    auto r = updateContext("ZSTD_freeDCtx", [this]() {
       return ZSTD_freeDCtx(context_);
    });

    if (r) {
        context_ = nullptr;
    }

    return r;
}

ZstdDecompressContextResult<void> ZstdDecompressContext::resetSession() {
    return updateContext("ZSTD_DCtx_reset(ZSTD_reset_session_only)", [this]() {
        return ZSTD_DCtx_reset(context_, ZSTD_reset_session_only);
    });
}

ZstdDecompressContextResult<void> ZstdDecompressContext::clearDictionary() {
    return updateContext("ZSTD_DCtx_refDDict(ZSTD_reset_session_only)", [this]() {
        return ZSTD_DCtx_refDDict(context_, nullptr);
    });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// decompress stream
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static ZstdDecompressStreamError decompressStreamError(size_t code, const char* op) {
    const auto error = ZSTD_getErrorName(code);
    return ZstdDecompressStreamError {
            code,
            fmt::format("decompress stream error. op={}, error={}", op, error),
    };
}

static ZstdDecompressStreamError decompressStreamErrorFrom(const ZstdDecompressContextError& e) {
    return ZstdDecompressStreamError {
            e.code,
            e.message,
    };
}

static ZstdDecompressStreamResult<tl::optional<uint64_t>> getFrameContentSize(const std::vector<uint8_t>& data) {
    const auto code = ZSTD_getFrameContentSize(&data[0], data.size());
    if (code == ZSTD_CONTENTSIZE_UNKNOWN) {
        return tl::nullopt;
    }
    else if (ZSTD_isError(code)) {
        return tl::make_unexpected(decompressStreamError(code, "ZSTD_getFrameContentSize"));
    }

    return code;
}

class ZstdDecompressStreamImpl {
public:
    std::unique_ptr<ZstdDecompressContext> context_;
    std::vector<uint8_t> out_buffer_;

    explicit ZstdDecompressStreamImpl(std::unique_ptr<ZstdDecompressContext>&& context)
        : context_(std::forward<std::unique_ptr<ZstdDecompressContext>>(context))
        , out_buffer_()
    {
    }

    ZSTD_outBuffer output() {
        out_buffer_.clear();
        out_buffer_.resize(ZSTD_DStreamOutSize());

        return ZSTD_outBuffer { &out_buffer_[0], out_buffer_.size(), 0 };
    }

    ZstdDecompressStreamResult<void> decompress(ZSTD_inBuffer input, const DecompressStreamCallback& callback) {
        auto finished = false;
        while (!finished) {
            auto output = this->output();
            const auto code = ZSTD_decompressStream(context_->context(), &output, &input);
            if (ZSTD_isError(code)) {
                return tl::make_unexpected(decompressStreamError(code, "ZSTD_decompressStream"));
            }

            finished = code == 0 || input.pos >= input.size;
            if (output.pos != 0) {
                out_buffer_.resize(output.pos);
                callback(out_buffer_);
            }
        }

        return {};
    }
};

ZstdDecompressStream::ZstdDecompressStream(std::unique_ptr<ZstdDecompressStreamImpl> &&pimpl)
        : pimpl_(std::forward<std::unique_ptr<ZstdDecompressStreamImpl>>(pimpl))
{
}

ZstdDecompressStream::~ZstdDecompressStream() = default;

ZstdDecompressStreamResult<std::unique_ptr<ZstdDecompressStream>>
ZstdDecompressStream::fromContext(std::unique_ptr<ZstdDecompressContext> context) {
    const auto r = context->resetSession().map_error(decompressStreamErrorFrom);
    if (!r) {
        return tl::make_unexpected(r.error());
    }

    auto pimpl = std::make_unique<ZstdDecompressStreamImpl>(std::move(context));
    return std::unique_ptr<ZstdDecompressStream>(new ZstdDecompressStream(std::move(pimpl)));
}

ZstdDecompressStreamResult<tl::optional<uint64_t>> ZstdDecompressStream::getFrameContentSize(const std::vector<uint8_t> &data) {
    return ::getFrameContentSize(data);
}

ZstdDecompressStreamResult<void>
ZstdDecompressStream::decompress(const std::vector<uint8_t> &data, const DecompressStreamCallback &callback) {
    if (data.empty()) {
        return {};
    }

    size_t i = 0;
    const auto max_input = ZSTD_DStreamInSize();
    while (i < data.size()) {
        const auto take = i + max_input < data.size() ? max_input : data.size() - i;
        ZSTD_inBuffer input = {&data[i], take, 0};
        i += take;

        const auto r = pimpl_->decompress(input, callback);
        if (!r) {
            return tl::make_unexpected(r.error());
        }
    }

    return {};
}

ZstdDecompressStreamResult<void> ZstdDecompressStream::close() {
    return pimpl_->context_->close()
            .map_error(decompressStreamErrorFrom);
}
