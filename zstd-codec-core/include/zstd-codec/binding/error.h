#pragma once

#include <cstdint>
#include <string>

#include "tl/optional.hpp"

#include "bindable.h"
#include "../compress.h"
#include "../decompress.h"

struct ZstdCodecBindingError {
    tl::optional<size_t> code;
    std::string message;

    static ZstdCodecBindingError from(ZstdCompressContextError&& error) {
        return ZstdCodecBindingError {
            error.code,
            std::move(error.message),
        };
    }

    static ZstdCodecBindingError from(ZstdCompressStreamError&& error) {
        return ZstdCodecBindingError {
                error.code,
                std::move(error.message),
        };
    }

    static ZstdCodecBindingError from(ZstdDecompressContextError&& error) {
        return ZstdCodecBindingError {
                error.code,
                std::move(error.message),
        };
    }

    static ZstdCodecBindingError from(ZstdDecompressStreamError&& error) {
        return ZstdCodecBindingError {
                error.code,
                std::move(error.message),
        };
    }
};

template <class T, class C>
concept ErrorThrowable = Contextual<C> && requires(T& t, const ZstdCodecBindingError& error) {
    requires requires(typename C::WireContext context) {
        t.throwError(context, error);
    };
};
