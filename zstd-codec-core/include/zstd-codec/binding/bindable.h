#pragma once

#include <cstdint>
#include <concepts>
#include <functional>
#include <vector>

template <class T>
concept Contextual = requires {
    typename T::WireContext;
};

template <class T>
concept Bindable = requires {
    typename T::WireType;
    typename T::ValueType;
};

template <class T, class C>
concept FromWireBindable = Contextual<C> && Bindable<T> && requires (T& t) {
    requires requires(typename C::WireContext context, const typename T::WireType& wire) {
        {t.fromWireType(context, wire)} -> std::convertible_to<typename T::ValueType>;
    };
};

template <class T, class C>
concept IntoWireBindable = Contextual<C> && Bindable<T> && requires (T& t) {
    requires requires(typename C::WireContext context, const typename T::ValueType& value) {
        {t.intoWireType(context, value)} -> std::convertible_to<typename T::WireType>;
    };
};

template <class T, class C>
concept BytesBindable = FromWireBindable<T, C>
        && IntoWireBindable<T, C>
        && std::is_same_v<typename T::ValueType, std::vector<uint8_t>>;

// from js to cpp only. no need to convert from cpp to js.
template <class T, class C>
concept BytesCallbackBindable = FromWireBindable<T, C>
        && std::is_same_v<typename T::ValueType, std::function<void(const std::vector<uint8_t>&)>>;
