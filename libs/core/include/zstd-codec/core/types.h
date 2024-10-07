#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zstd_codec::core {
    using int8_t = std::int8_t;
    using int16_t = std::int16_t;
    using int32_t = std::int32_t;
    using int64_t = std::int64_t;

    using uint8_t = std::uint8_t;
    using uint16_t = std::uint16_t;
    using uint32_t = std::uint32_t;
    using uint64_t = std::uint64_t;

    using string = std::string;

    template <typename T>
    using vector = std::vector<T>;
}