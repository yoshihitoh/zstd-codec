#pragma once

#include <cstdint>
#include <string>

#include "tl/optional.hpp"

struct ZstdCodecBindingError {
    tl::optional<size_t> code;
    std::string message;
};

template <class T>
concept ErrorThrowable = requires(T& t, const ZstdCodecBindingError& error) {
    t.throwError(error);
};
