#pragma once

#include <cstdint>
#include <concepts>
#include <functional>
#include <vector>

template <class T>
concept Bindable = requires {
    typename T::WireType;
    typename T::ValueType;
};

template <class T>
concept FromWireBindable = Bindable<T> && requires (T& t) {
    requires requires(const typename T::WireType& wire) {
        {t.fromWireType(wire)} -> std::convertible_to<typename T::ValueType>;
    };
};

template <class T>
concept IntoWireBindable = Bindable<T> && requires (T& t) {
    requires requires(const typename T::ValueType& value) {
        {t.intoWireType(value)} -> std::convertible_to<typename T::WireType>;
    };
};

template <class T>
concept BytesBindable = FromWireBindable<T>
        && IntoWireBindable<T>
        && std::is_same_v<typename T::ValueType, std::vector<uint8_t>>;

// from js to cpp only. no need to convert from cpp to js.
template <class T>
concept BytesCallbackBindable = FromWireBindable<T>
        && std::is_same_v<typename T::ValueType, std::function<void(const std::vector<uint8_t>&)>>;
