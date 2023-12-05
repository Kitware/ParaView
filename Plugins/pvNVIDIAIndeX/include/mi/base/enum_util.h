/***************************************************************************************************
 * Copyright 2023 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/

#ifndef MI_BASE_ENUM_UTIL_H
#define MI_BASE_ENUM_UTIL_H

#if (__cplusplus >= 201103L)
#include "config.h"

#include <type_traits>

// internal utility for MI_MAKE_ENUM_BITOPS
#define MI_MAKE_ENUM_BITOPS_PAIR(Enum,OP) \
MI_HOST_DEVICE_INLINE constexpr Enum operator OP(const Enum l, const Enum r) { \
    using Basic = std::underlying_type_t<Enum>; \
    return static_cast<Enum>(static_cast<Basic>(l) OP static_cast<Basic>(r)); } \
MI_HOST_DEVICE_INLINE Enum& operator OP##=(Enum& l, const Enum r) { \
    using Basic = std::underlying_type_t<Enum>; \
    return reinterpret_cast<Enum&>(reinterpret_cast<Basic&>(l) OP##= static_cast<Basic>(r)); }

// internal utility for MI_MAKE_ENUM_BITOPS
#define MI_MAKE_ENUM_SHIFTOPS_PAIR(Enum,OP) \
template <typename T, std::enable_if_t<std::is_integral<T>::value,bool> = true> \
MI_HOST_DEVICE_INLINE constexpr Enum operator OP(const Enum e, const T s) { \
    return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(e) OP s); } \
template <typename T, std::enable_if_t<std::is_integral<T>::value,bool> = true> \
MI_HOST_DEVICE_INLINE constexpr Enum& operator OP##=(Enum& e, const T s) { \
    return reinterpret_cast<Enum&>(reinterpret_cast<std::underlying_type_t<Enum>&>(e) OP##= s); }


/// Utility to define binary operations on enum types.
///
/// Note that the resulting values may not have names in the given enum type.
#define MI_MAKE_ENUM_BITOPS(Enum) \
MI_MAKE_ENUM_BITOPS_PAIR(Enum,|) \
MI_MAKE_ENUM_BITOPS_PAIR(Enum,&) \
MI_MAKE_ENUM_BITOPS_PAIR(Enum,^) \
MI_MAKE_ENUM_BITOPS_PAIR(Enum,+) \
MI_MAKE_ENUM_BITOPS_PAIR(Enum,-) \
MI_MAKE_ENUM_SHIFTOPS_PAIR(Enum,<<) \
MI_MAKE_ENUM_SHIFTOPS_PAIR(Enum,>>) \
MI_HOST_DEVICE_INLINE constexpr Enum operator ~(const Enum e) { \
    return static_cast<Enum>(~static_cast<std::underlying_type_t<Enum>>(e)); }

namespace mi {

/** \brief Converts an enumerator to its underlying integral value. */
template <typename T, std::enable_if_t<std::is_enum<T>::value,bool> = true>
constexpr auto to_underlying(const T val) { return std::underlying_type_t<T>(val); }

template <typename T, std::enable_if_t<std::is_integral<T>::value,bool> = true>
constexpr auto to_underlying(const T val) { return val; }

}

#else
#define MI_MAKE_ENUM_BITOPS(Enum)
#endif

#endif //MI_BASE_ENUM_UTIL_H
