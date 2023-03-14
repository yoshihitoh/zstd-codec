#pragma once

#include "bindable.h"
#include "error.h"

template <Contextual C, BytesBindable<C> B, BytesCallbackBindable<C> F, ErrorThrowable<C> E>
class ZstdCodecBinder {
private:
    B bytes_;
    F callback_;
    E error_;

public:
    using Bytes = std::vector<uint8_t>;
    using BytesCallbak = std::function<void(const Bytes&)>;

    explicit ZstdCodecBinder(B&& b, F&& f, E&& e) : bytes_(b), callback_(f), error_(e)
    {
    }

    Bytes intoBytesVector(typename C::WireContext wire_context, const typename B::WireType& wire) {
        return bytes_.fromWireType(wire_context, wire);
    }

    typename B::WireType intoBytesWire(typename C::WireContext wire_context, const Bytes& data) {
        return bytes_.intoWireType(wire_context, data);
    }

    BytesCallbak intoBytesCallbackFn(typename C::WireContext wire_context, const typename F::WireType& wire) {
        return callback_.fromWireType(wire_context, wire);
    }

    void throwError(typename C::WireContext wire_context, const ZstdCodecBindingError& error) {
        error_.throwError(wire_context, error);
    }
};
