#pragma once

#include "bindable.h"
#include "error.h"

template <BytesBindable B, BytesCallbackBindable C, ErrorThrowable E>
class ZstdCodecBinder {
private:
    B bytes_;
    C callback_;
    E error_;

public:
    explicit ZstdCodecBinder(B&& b, C&& c, E&& e) : bytes_(b), callback_(c), error_(e)
    {
    }

    std::vector<uint8_t> intoBytesVector(const typename B::WireType& wire) {
        return bytes_.fromWireType(wire);
    }

    typename B::WireType intoBytesWire(const std::vector<uint8_t>& data) {
        return bytes_.intoWireType(data);
    }

    std::function<void(const std::vector<uint8_t>&)> intoBytesCallbackFn(const typename C::WireType& wire) {
        return callback_.fromWireType(wire);
    }

    void throwError(const ZstdCodecBindingError& error) {
        error_.throwError(error);
    }
};
