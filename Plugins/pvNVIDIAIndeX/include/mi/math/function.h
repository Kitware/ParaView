/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/math/function.h
/// \brief Math functions and function templates on simple types or generic container and vector
///        concepts.
///
/// See \ref mi_math_function.

#ifndef MI_MATH_FUNCTION_H
#define MI_MATH_FUNCTION_H

#include <mi/base/assert.h>
#include <mi/base/types.h>

#ifdef MI_PLATFORM_WINDOWS
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#ifdef MI_ARCH_64BIT
#pragma intrinsic(_BitScanReverse64)
#endif
#endif

namespace mi
{

namespace math
{

/** \defgroup mi_math_function Math Functions
    \ingroup mi_math

    The %math API provides functions and function templates that act on simple
    types or generic container and vector concepts.

    Examples are trigonometric functions or #lexicographically_compare().

    Functions exist typically as a family of overloaded functions for all applicable argument types,
    such as trigonometric functions, or as a single function template for a vector-like concept,
    such as #lexicographically_compare().

    Generic function templates on vector-like value require the vector-like type to have a
    compile-time constant \c SIZE as local value, defining the number of elements in the value,
    and \c operator[] style access to these elements.

    Overloaded functions may have additional overloads for various non-simple types, such as
    #mi::math::Vector or #mi::math::Color.

    Functions in this group are intended for unqualified naming, such that argument-dependent name
    lookup (ADL, a.k.a. extended Koenig lookup) can be used with them.

    The basic function templates for \c min() and \c max(), as well as the overloaded \c %abs()
    functions are taken from the #mi::base namespace.

    \par Include File:
    <tt> \#include <mi/math/function.h></tt>

    @{
*/

/// Namespace for basic %math functors in the Math API.
namespace functor
{
/** \defgroup mi_math_functor Basic Math Functors in the Math API
    \ingroup mi_math_function

    Basic %math functors in the Math API.

    In particular, the different math operators are simple scalar functors are provided. They are
    useful for the \ref mi_math_general.

    \par Include File:
    <tt> \#include <mi/math/function.h></tt>

    @{
*/

/// Functor for the equality comparison operator, \c ==.
struct Operator_equal_equal
{
  /// Functor call.
  template <typename T1, typename T2>
  inline bool operator()(const T1& t1, const T2& t2) const
  {
    return t1 == t2;
  }
};

/// Functor for the inequality comparison operator, \c !=.
struct Operator_not_equal
{
  /// Functor call.
  template <typename T1, typename T2>
  inline bool operator()(const T1& t1, const T2& t2) const
  {
    return t1 != t2;
  }
};

/// Functor for the less-than comparison operator, \c <.
struct Operator_less
{
  /// Functor call.
  template <typename T1, typename T2>
  inline bool operator()(const T1& t1, const T2& t2) const
  {
    return t1 < t2;
  }
};

/// Functor for the less-than-or-equal comparison operator, \c <=.
struct Operator_less_equal
{
  /// Functor call.
  template <typename T1, typename T2>
  inline bool operator()(const T1& t1, const T2& t2) const
  {
    return t1 <= t2;
  }
};

/// Functor for the greater-than comparison operator, \c >.
struct Operator_greater
{
  /// Functor call.
  template <typename T1, typename T2>
  inline bool operator()(const T1& t1, const T2& t2) const
  {
    return t1 > t2;
  }
};

/// Functor for the greater-than-or-equal comparison operator, \c >=.
struct Operator_greater_equal
{
  /// Functor call.
  template <typename T1, typename T2>
  inline bool operator()(const T1& t1, const T2& t2) const
  {
    return t1 >= t2;
  }
};

/// Functor for the plus operator, \c +.
struct Operator_plus
{
  /// Functor call.
  template <typename T>
  inline T operator()(const T& t1, const T& t2) const
  {
    return t1 + t2;
  }
};

/// Functor for the minus operator, \c -, unary and binary.
struct Operator_minus
{
  /// Unary %functor call.
  template <typename T>
  inline T operator()(const T& t) const
  {
    return -t;
  }
  /// Binary %functor call.
  template <typename T>
  inline T operator()(const T& t1, const T& t2) const
  {
    return t1 - t2;
  }
};

/// Functor for the multiplication operator, \c *.
struct Operator_multiply
{
  /// Functor call.
  template <typename T>
  inline T operator()(const T& t1, const T& t2) const
  {
    return t1 * t2;
  }
};

/// Functor for the division operator, \c /.
struct Operator_divide
{
  /// Functor call.
  template <typename T>
  inline T operator()(const T& t1, const T& t2) const
  {
    return t1 / t2;
  }
};

/// Functor for the logical and operator, \c &&.
struct Operator_and_and
{
  /// Functor call.
  template <typename T>
  inline bool operator()(const T& t1, const T& t2) const
  {
    return t1 && t2;
  }
};

/// Functor for the logical or operator, \c ||.
struct Operator_or_or
{
  /// Functor call.
  template <typename T>
  inline bool operator()(const T& t1, const T& t2) const
  {
    return t1 || t2;
  }
};

/// Functor for the xor operator, \c ^.
struct Operator_xor
{
  /// Functor call.
  template <typename T>
  inline bool operator()(const T& t1, const T& t2) const
  {
    return t1 ^ t2;
  }
};

/// Functor for the logical not operator, \c !.
struct Operator_not
{
  /// Functor call.
  template <typename T>
  inline bool operator()(const T& t) const
  {
    return !t;
  }
};

/// Functor for the pre-increment operator, \c ++.
struct Operator_pre_incr
{
  /// Functor call.
  template <typename T>
  inline T operator()(T& t) const
  {
    return ++t;
  }
};

/// Functor for the post-increment operator, \c ++.
struct Operator_post_incr
{
  /// Functor call.
  template <typename T>
  inline T operator()(T& t) const
  {
    return t++;
  }
};

/// Functor for the pre-decrement operator, \c --.
struct Operator_pre_decr
{
  /// Functor call.
  template <typename T>
  inline T operator()(T& t) const
  {
    return --t;
  }
};

/// Functor for the post-decrement operator, \c --.
struct Operator_post_decr
{
  /// Functor call.
  template <typename T>
  inline T operator()(T& t) const
  {
    return t--;
  }
};

/*@}*/ // end group mi_math_functor

} // namespace functor

/// Namespace for generic functions in the Math API.
namespace general
{
/** \defgroup mi_math_general Generic Functions in the Math API
    \ingroup mi_math_function

    Generic functions in the Math API targeted to static vector-like sequences.

    They help, together with the \ref mi_math_functor to implement the operators and functions on
    vector-like data types, whose number of elements are known at compile time. Thus, optimizations
    like loop unrolling are possible. These generic functions and the functors are in principle
    inline.

    \par Include File:
    <tt> \#include <mi/math/function.h></tt>

    @{
*/

/// Generic transform function that applies a unary %functor (return value).
///
/// The function applies the unary %functor \c f to each element of the vector-like value \c vec and
/// assigns the result to the same coordinate in the vector-like value \c result.
///
/// Both vector values have to be of the same size, available as compile-time constant
/// \c Vector::SIZE.
template <class Vector, class ResultVector, class UnaryFunctor>
inline void transform(const Vector& vec, ResultVector& result, UnaryFunctor f)
{
  mi_static_assert(Vector::SIZE == ResultVector::SIZE);
  for (Size i = 0; i != Vector::SIZE; ++i)
    result.set(i, f(vec.get(i)));
}

/// Generic transform function that applies a binary %functor (return value).
///
/// The function applies the binary %functor \c f to each pair of matching elements of the
/// vector-like values \c vec1 and \c vec2 and assigns the result to the same coordinate in the
/// vector-like value \c result.
///
/// All vector values have to be of the same size, available as compile-time constant
/// \c Vector1::SIZE.
template <class Vector1, class Vector2, class ResultVector, class BinaryFunctor>
inline void transform(
  const Vector1& vec1, const Vector2& vec2, ResultVector& result, BinaryFunctor f)
{
  mi_static_assert(Vector1::SIZE == Vector2::SIZE);
  mi_static_assert(Vector1::SIZE == ResultVector::SIZE);
  for (Size i = 0; i != Vector1::SIZE; ++i)
    result.set(i, f(vec1.get(i), vec2.get(i)));
}

/// Generic transform function that applies a binary %functor (return value, LHS scalar).
///
/// The function applies the binary %functor \c f to each element of the vector-like values \c vec
/// and assigns the result to the same coordinate in the vector-like value \c result. The scalar
/// value \c s is passed as a left argument to \c f and the vector element is passed as a right
/// argument.
///
/// All vector values have to be of the same size, available as compile-time constant
/// \c Vector::SIZE.
template <class Scalar, class Vector, class ResultVector, class BinaryFunctor>
inline void transform_left_scalar(
  const Scalar& s, const Vector& vec, ResultVector& result, BinaryFunctor f)
{
  mi_static_assert(Vector::SIZE == ResultVector::SIZE);
  for (Size i = 0; i != Vector::SIZE; ++i)
    result.set(i, f(s, vec.get(i)));
}

/// Generic transform function that applies a binary %functor (return value, RHS scalar).
///
/// The function applies the binary %functor \c f to each element of the vector-like values \c vec
/// and assigns the result to the same coordinate in the vector-like value \c result. The scalar
/// value \c s is passed as a left argument to \c f and the vector element is passed as a right
/// argument.
///
/// All vector values have to be of the same size, available as compile-time constant
/// \c Vector::SIZE.
template <class Scalar, class Vector, class ResultVector, class BinaryFunctor>
inline void transform_right_scalar(
  const Vector& vec, const Scalar& s, ResultVector& result, BinaryFunctor f)
{
  mi_static_assert(Vector::SIZE == ResultVector::SIZE);
  for (Size i = 0; i != Vector::SIZE; ++i)
    result.set(i, f(vec.get(i), s));
}

/// Generic transform function that applies a unary %functor (in-place).
///
/// The transform function applies the unary %functor \c f to each element of the vector-like
/// value \c vec, which is modifiable.
///
/// The size of \c vec is available as compile-time constant \c Vector::SIZE.
template <class Vector, class UnaryFunctor>
inline void for_each(Vector& vec, UnaryFunctor f)
{
  for (Size i = 0; i != Vector::SIZE; ++i)
    f(vec.begin()[i]);
}

/// Generic transform function that applies a binary %functor (in-place).
///
/// The transform function applies the binary %functor \c f to each pair of matching elements of the
/// vector-like values \c vec1 and \c vec2, where \c vec1 is modifiable and \c vec2 not.
///
/// The size of \c vec1 is available as compile-time constant \c Vector::SIZE and has to be the same
/// as for \c vec2.
template <class Vector1, class Vector2, class BinaryFunctor>
inline void for_each(Vector1& vec1, const Vector2& vec2, BinaryFunctor f)
{
  mi_static_assert(Vector1::SIZE == Vector2::SIZE);
  for (Size i = 0; i != Vector1::SIZE; ++i)
    f(vec1.begin()[i], vec2.begin()[i]);
}

/*@}*/ // end group mi_math_functor

} // namespace general

/// Returns the constant \c e to the power of \p s (exponential function).
inline Float32 exp(Float32 s)
{
  return std::exp(s);
}
/// Returns the constant \c e to the power of \p s (exponential function).
inline Float64 exp(Float64 s)
{
  return std::exp(s);
}

/// Returns the natural logarithm of \p s.
inline Float32 log(Float32 s)
{
  return std::log(s);
}
/// Returns the natural logarithm of \p s.
inline Float64 log(Float64 s)
{
  return std::log(s);
}

using mi::base::min;
using mi::base::max;
using mi::base::abs;

/** \defgroup mi_math_approx_function Fast Approximations for float Math Functions
    \ingroup mi_math_function

    Fast approximations for %math functions on limited precision floats.

    \par Include File:
    <tt> \#include <mi/math/function.h></tt>

    @{
*/

/// A fast implementation of sqrt(x) for floats.
inline Float32 fast_sqrt(Float32 i)
{
  int tmp = base::binary_cast<int>(i);
  tmp -= 1 << 23; // Remove last bit to not let it go to mantissa
  // tmp is now an approximation to logbase2(i)
  tmp = tmp >> 1; // Divide by 2
  tmp += 1 << 29; // Add 64 to the exponent: (e+127)/2 = (e/2)+63
                  // that represents (e/2)-64 but we want e/2
  return base::binary_cast<Float32>(tmp);
}

/// A fast implementation of exp for floats.
inline Float32 fast_exp(Float32 x)
{
  const Float32 EXP_C = 8388608.0f;                           // 2^23
  const Float32 LOG_2_E = 1.4426950408889634073599246810019f; // 1 / log(2)

  x *= LOG_2_E;
  Float32 y = x - std::floor(x);
  y = (y - y * y) * 0.33971f;
  const Float32 z = max MI_PREVENT_MACRO_EXPAND((x + 127.0f - y) * EXP_C, 0.f);
  return base::binary_cast<Float32>(static_cast<int>(z));
}

/// A fast implementation of pow(2,x) for floats.
inline Float32 fast_pow2(Float32 x)
{
  const Float32 EXP_C = 8388608.0f; // 2^23

  Float32 y = x - std::floor(x);
  y = (y - y * y) * 0.33971f;
  const Float32 z = max MI_PREVENT_MACRO_EXPAND((x + 127.0f - y) * EXP_C, 0.f);
  return base::binary_cast<Float32>(static_cast<int>(z));
}

/// A fast implementation of log2(x) for floats.
inline Float32 fast_log2(Float32 i)
{
  const Float32 LOG_C = 0.00000011920928955078125f; // 2^(-23)

  const Float32 x = static_cast<Float32>(base::binary_cast<int>(i)) * LOG_C - 127.f;
  const Float32 y = x - std::floor(x);
  return x + (y - y * y) * 0.346607f;
}

/// A fast implementation of pow(x,y) for floats.
inline Float32 fast_pow(Float32 b, ///< %base
  Float32 e)                       ///< exponent
{
  if (b == 0.0f)
    return 0.0f;

  const Float32 LOG_C = 0.00000011920928955078125f; // 2^(-23)
  const Float32 EXP_C = 8388608.0f;                 // 2^23

  const Float32 x = static_cast<Float32>(base::binary_cast<int>(b)) * LOG_C - 127.f;
  const Float32 y = x - std::floor(x);
  const Float32 fl = e * (x + (y - y * y) * 0.346607f);
  const Float32 y2 = fl - std::floor(fl);
  const Float32 z = max(fl * EXP_C + static_cast<Float32>(127.0 * EXP_C) -
      (y2 - y2 * y2) * static_cast<Float32>(0.33971 * EXP_C),
    0.f);

  return base::binary_cast<Float32>(static_cast<int>(z));
}

/*@}*/ // end group mi_math_approx_function

/// Returns the arc cosine of \p s in radians.
inline Float32 acos(Float32 s)
{
  return std::acos(s);
}
/// Returns the arc cosine of \p s in radians.
inline Float64 acos(Float64 s)
{
  return std::acos(s);
}

/// Returns \c true if \c v is not equal to zero.
inline bool all(Uint8 v)
{
  return Uint8(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Uint16 v)
{
  return Uint16(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Uint32 v)
{
  return Uint32(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Uint64 v)
{
  return Uint64(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Sint8 v)
{
  return Sint8(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Sint16 v)
{
  return Sint16(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Sint32 v)
{
  return Sint32(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Sint64 v)
{
  return Sint64(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Float32 v)
{
  return Float32(0) != v;
}
/// Returns \c true if \c v is not equal to zero.
inline bool all(Float64 v)
{
  return Float64(0) != v;
}

/// Returns \c true if \c v is not equal to zero.
inline bool any(Uint8 v)
{
  return Uint8(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Uint16 v)
{
  return Uint16(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Uint32 v)
{
  return Uint32(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Uint64 v)
{
  return Uint64(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Sint8 v)
{
  return Sint8(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Sint16 v)
{
  return Sint16(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Sint32 v)
{
  return Sint32(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Sint64 v)
{
  return Sint64(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Float32 v)
{
  return Float32(0) != v;
} //-V524 PVS
/// Returns \c true if \c v is not equal to zero.
inline bool any(Float64 v)
{
  return Float64(0) != v;
} //-V524 PVS

/// Returns the arc sine of \p s in radians.
inline Float32 asin(Float32 s)
{
  return std::asin(s);
}
/// Returns the arc sine of \p s in radians.
inline Float64 asin(Float64 s)
{
  return std::asin(s);
}

/// Returns the arc tangent of \p s.
inline Float32 atan(Float32 s)
{
  return std::atan(s);
}
/// Returns the arc tangent of \p s.
inline Float64 atan(Float64 s)
{
  return std::atan(s);
}

/// Returns the arc tangent of \p s / \p t.
///
/// The signs of \p s and \p t are used to determine the quadrant of the results.
inline Float32 atan2(Float32 s, Float32 t)
{
  return std::atan2(s, t);
}
/// Returns the arc tangent of \p s / \p t.
///
/// The signs of \p s and \p t are used to determine the quadrant of the results.
inline Float64 atan2(Float64 s, Float64 t)
{
  return std::atan2(s, t);
}

/// Returns the smallest integral value that is not less than \p s.
inline Float32 ceil(Float32 s)
{
  return std::ceil(s);
}
/// Returns the smallest integral value that is not less than \p s.
inline Float64 ceil(Float64 s)
{
  return std::ceil(s);
}

/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Uint8 clamp(Uint8 s, Uint8 low, Uint8 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Uint16 clamp(Uint16 s, Uint16 low, Uint16 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Uint32 clamp(Uint32 s, Uint32 low, Uint32 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Uint64 clamp(Uint64 s, Uint64 low, Uint64 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Sint8 clamp(Sint8 s, Sint8 low, Sint8 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Sint16 clamp(Sint16 s, Sint16 low, Sint16 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Sint32 clamp(Sint32 s, Sint32 low, Sint32 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Sint64 clamp(Sint64 s, Sint64 low, Sint64 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Float32 clamp(Float32 s, Float32 low, Float32 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}
/// Returns the value \p s if it is in the range [\p low, \p high], the
/// value \p low if \p s < \p low, or the value \p high if \p s > \p high.
inline Float64 clamp(Float64 s, Float64 low, Float64 high)
{
  return (s < low) ? low : (s > high) ? high : s;
}

/// Returns the cosine of \p a. The angle \p a is specified in radians.
inline Float32 cos(Float32 a)
{
  return std::cos(a);
}
/// Returns the cosine of \p a. The angle \p a is specified in radians.
inline Float64 cos(Float64 a)
{
  return std::cos(a);
}

/// Converts radians \p r to degrees.
inline Float32 degrees(Float32 r)
{
  return r * Float32(180.0 / MI_PI);
}
/// Converts radians \p r to degrees.
inline Float64 degrees(Float64 r)
{
  return r * Float64(180.0 / MI_PI);
}

/// Returns the constant \c 2 to the power of \p s (exponential function).
inline Float32 exp2(Float32 s)
{
  return fast_pow2(s);
}
/// Returns the constant \c 2 to the power of \p s (exponential function).
inline Float64 exp2(Float64 s)
{
  return std::exp(s * 0.69314718055994530941723212145818 /* log(2) */);
}

/// Returns the largest integral value that is not greater than \p s.
inline Float32 floor(Float32 s)
{
  return std::floor(s);
}
/// Returns the largest integral value that is not greater than \p s.
inline Float64 floor(Float64 s)
{
  return std::floor(s);
}

/// Returns \p a modulo \p b, in other words, the remainder of a/b.
///
/// The result has the same sign as \p a.
inline Float32 fmod(Float32 a, Float32 b)
{
  return std::fmod(a, b);
}
/// Returns \p a modulo \p b, in other words, the remainder of a/b.
///
/// The result has the same sign as \p a.
inline Float64 fmod(Float64 a, Float64 b)
{
  return std::fmod(a, b);
}

/// Returns the positive fractional part of \p s.
inline Float32 frac(Float32 s)
{
  Float32 dummy;
  if (s >= 0.0f)
    return std::modf(s, &dummy);
  else
    return 1.0f + std::modf(s, &dummy);
}
/// Returns the positive fractional part of \p s.
inline Float64 frac(Float64 s)
{
  Float64 dummy;
  if (s >= 0.0f)
    return std::modf(s, &dummy);
  else
    return 1.0f + std::modf(s, &dummy);
}

/// Compares the two given values for equality within the given epsilon.
inline bool is_approx_equal(Float32 left, Float32 right, Float32 e)
{
  return abs(left - right) <= e;
}

/// Compares the two given values for equality within the given epsilon.
inline bool is_approx_equal(Float64 left, Float64 right, Float64 e)
{
  return abs(left - right) <= e;
}

/// Returns the number of leading zeros of \p v, 32-bit version.
inline Uint32 leading_zeros(Uint32 v)
{
// This implementation tries to use built-in functions if available. For the fallback
// method, see Henry Warren: "Hacker's Delight" for reference.
#if defined(MI_COMPILER_MSC)
  unsigned long index;
  const unsigned char valid = _BitScanReverse(&index, v);
  return (valid != 0) ? 31 - index : 32;
#elif defined(MI_COMPILER_ICC) || defined(MI_COMPILER_GCC)
  return (v != 0) ? __builtin_clz(v) : 32;
#else
  // use fallback
  if (v == 0)
    return 32;
  Uint32 n = 1;
  if ((v >> 16) == 0)
  {
    n += 16;
    v <<= 16;
  };
  if ((v >> 24) == 0)
  {
    n += 8;
    v <<= 8;
  };
  if ((v >> 28) == 0)
  {
    n += 4;
    v <<= 4;
  };
  if ((v >> 30) == 0)
  {
    n += 2;
    v <<= 2;
  };
  n -= Uint32(v >> 31);
  return n;
#endif
}

/// Returns the number of leading zeros of \p v, 64-bit version.
inline Uint32 leading_zeros(Uint64 v)
{
// This implementation tries to use built-in functions if available. For the fallback
// method, see Henry Warren: "Hacker's Delight" for reference.
#if defined(MI_COMPILER_MSC)
#if defined(MI_ARCH_64BIT)
  unsigned long index;
  const unsigned char valid = _BitScanReverse64(&index, v);
  return (valid != 0) ? 63 - index : 64;
#else
  unsigned long index_h, index_l;
  const unsigned char valid_h = _BitScanReverse(&index_h, (Uint32)(v >> 32));
  const unsigned char valid_l = _BitScanReverse(&index_l, (Uint32)(v & 0xFFFFFFFF));
  if (valid_h == 0)
    return (valid_l != 0) ? 63 - index_l : 64;
  return 63 - index_h + 32;
#endif
#elif defined(MI_COMPILER_ICC) || defined(MI_COMPILER_GCC)
  return (v != 0) ? __builtin_clzll(v) : 64;
#else
  // use fallback, e.g. on Solaris
  if (v == 0)
    return 64;
  Uint32 n = 1;
  if ((v >> 32) == 0)
  {
    n += 32;
    v <<= 32;
  };
  if ((v >> 48) == 0)
  {
    n += 16;
    v <<= 16;
  };
  if ((v >> 56) == 0)
  {
    n += 8;
    v <<= 8;
  };
  if ((v >> 60) == 0)
  {
    n += 4;
    v <<= 4;
  };
  if ((v >> 62) == 0)
  {
    n += 2;
    v <<= 2;
  };
  n -= Uint32(v >> 63);
  return n;
#endif
}

/// Returns the linear interpolation between \p s1 and \c s2, i.e., it returns
/// <tt>(1-t) * s1 + t * s2</tt>.
inline Float32 lerp(Float32 s1, ///< one scalar
  Float32 s2,                   ///< second scalar
  Float32 t)                    ///< interpolation parameter in [0,1]
{
  return s1 * (Float32(1) - t) + s2 * t;
}

/// Returns the linear interpolation between \p s1 and \c s2, i.e., it returns
/// <tt>(1-t) * s1 + t * s2</tt>.
inline Float64 lerp(Float64 s1, ///< one scalar
  Float64 s2,                   ///< second scalar
  Float64 t)                    ///< interpolation parameter in [0,1]
{
  return s1 * (Float64(1) - t) + s2 * t;
}

/// Returns the %base 2 logarithm of \p s.
inline Float32 log2 MI_PREVENT_MACRO_EXPAND(Float32 s)
{
  return std::log(s) * 1.4426950408889634073599246810019f /* log(2) */;
}
/// Returns the %base 2 logarithm of \p s.
inline Float64 log2 MI_PREVENT_MACRO_EXPAND(Float64 s)
{
  return std::log(s) * 1.4426950408889634073599246810019 /* log(2) */;
}

/// Returns the integer log2 of \p v.
inline Sint32 log2_int(const Uint32 v)
{
  return (v != 0) ? 31 - leading_zeros(v) : 0;
}

/// Returns the integer log2 of \p v.
inline Sint32 log2_int(const Uint64 v)
{
  return (v != 0) ? 63 - leading_zeros(v) : 0;
}

/// Returns the integer log2 of \p v.
inline Sint32 log2_int(const Float32 v)
{
  return (mi::base::binary_cast<Uint32>(v) >> 23) - 127;
}

/// Returns the integer log2 of \p v.
inline Sint32 log2_int(const Float64 v)
{
  return static_cast<Sint32>(mi::base::binary_cast<Uint64>(v) >> 52) - 1023;
}

/// Returns the integer log2 of \p v, i.e., rounded up to the next integer.
template <typename Integer>
inline Sint32 log2_int_ceil(const Integer v)
{
  // See Henry Warren: "Hacker's Delight" for reference.
  return (v > 1) ? log2_int(v - 1) + 1 : 0;
}

/// Returns the %base 10 logarithm of \p s.
inline Float32 log10(Float32 s)
{
  return std::log10(s);
}
/// Returns the %base 10 logarithm of \p s.
inline Float64 log10(Float64 s)
{
  return std::log10(s);
}

/// Returns the fractional part of \p s and stores the integral part of \p s in \p i.
///
/// Both parts have the same sign as \p s.
inline Float32 modf(Float32 s, Float32& i)
{
  return std::modf(s, &i);
}
/// Returns the fractional part of \p s and stores the integral part of \p s in \p i.
///
/// Both parts have the same sign as \p s.
inline Float64 modf(Float64 s, Float64& i)
{
  return std::modf(s, &i);
}

/// Returns \p a to the power of  \p b.
inline Uint32 pow(Uint32 a, Uint32 b)
{
  return Uint32(std::pow(double(a), int(b)));
}
/// Returns \p a to the power of  \p b.
inline Uint64 pow(Uint64 a, Uint64 b)
{
  return Uint64(std::pow(double(a), int(b)));
}
/// Returns \p a to the power of  \p b.
inline Sint32 pow(Sint32 a, Sint32 b)
{
  return Sint32(std::pow(double(a), int(b)));
}
/// Returns \p a to the power of  \p b.
inline Sint64 pow(Sint64 a, Sint64 b)
{
  return Sint64(std::pow(double(a), int(b)));
}
/// Returns \p a to the power of  \p b.
inline Float32 pow(Float32 a, Float32 b)
{
  return std::pow(a, b);
}
/// Returns \p a to the power of  \p b.
inline Float64 pow(Float64 a, Float64 b)
{
  return std::pow(a, b);
}

/// Converts degrees \p d to radians.
inline Float32 radians(Float32 d)
{
  return d * Float32(MI_PI / 180.0);
}
/// Converts degrees \p d to radians.
inline Float64 radians(Float64 d)
{
  return d * Float64(MI_PI / 180.0);
}

/// Returns \p s rounded to the nearest integer value.
inline Float32 round(Float32 s)
{
  return std::floor(s + 0.5f);
}
/// Returns \p s rounded to the nearest integer value.
inline Float64 round(Float64 s)
{
  return std::floor(s + 0.5);
}

/// Returns the reciprocal of the square root of \p s.
inline Float32 rsqrt(Float32 s)
{
  return 1.0f / std::sqrt(s);
}
/// Returns the reciprocal of the square root of \p s.
inline Float64 rsqrt(Float64 s)
{
  return 1.0 / std::sqrt(s);
}

/// Returns the value \p s clamped to the range [0,1].
inline Float32 saturate(Float32 s)
{
  return (s < 0.f) ? 0.f : (s > 1.f) ? 1.f : s;
}
/// Returns the value \p s clamped to the range [0,1].
inline Float64 saturate(Float64 s)
{
  return (s < 0.) ? 0. : (s > 1.) ? 1. : s;
}

/// Returns -1 if \c s<0, 0 if \c s==0, and +1 if \c s>0.
inline Sint8 sign(Sint8 s)
{
  int r = (s < 0) ? -1 : (s > 0) ? 1 : 0;
  return static_cast<Sint8>(r);
}
/// Returns -1 if \c s<0, 0 if \c s==0, and +1 if \c s>0.
inline Sint16 sign(Sint16 s)
{
  int r = (s < 0) ? -1 : (s > 0) ? 1 : 0;
  return static_cast<Sint16>(r);
}
/// Returns -1 if \c s<0, 0 if \c s==0, and +1 if \c s>0.
inline Sint32 sign(Sint32 s)
{
  return (s < 0) ? -1 : (s > 0) ? 1 : 0;
}
/// Returns -1 if \c s<0, 0 if \c s==0, and +1 if \c s>0.
inline Sint64 sign(Sint64 s)
{
  return (s < 0) ? -1 : (s > 0) ? 1 : 0;
}
/// Returns -1 if \c s<0, 0 if \c s==0, and +1 if \c s>0.
inline Float32 sign(Float32 s)
{
  return (s < 0.f) ? -1.f : (s > 0.f) ? 1.f : 0.f;
}
/// Returns -1 if \c s<0, 0 if \c s==0, and +1 if \c s>0.
inline Float64 sign(Float64 s)
{
  return (s < 0.) ? -1. : (s > 0.) ? 1. : 0.;
}

/// Returns \c true if \c s<0 and \c false if \c s>= 0.
inline bool sign_bit(Sint8 s)
{
  return s < 0;
}
/// Returns \c true if \c s<0 and \c false if \c s>= 0.
inline bool sign_bit(Sint16 s)
{
  return s < 0;
}
/// Returns \c true if \c s<0 and \c false if \c s>= 0.
inline bool sign_bit(Sint32 s)
{
  return s < 0;
}
/// Returns \c true if \c s<0 and \c false if \c s>= 0.
inline bool sign_bit(Sint64 s)
{
  return s < 0;
}

/// Extracts the sign bit of a single-precision floating point number.
///
/// The methods relies on the IEEE 754 floating-point standard. Note that the sign bit is set for
/// the special floating-point value -0.0f, so this function returns \c true for this value.
inline bool sign_bit(Float32 s)
{
  return (base::binary_cast<Uint32>(s) & (1U << 31)) != 0U;
}

/// Extracts the sign bit of a double-precision floating point number.
///
/// The methods relies on the IEEE 754 floating-point standard. Note that the sign bit is set for
/// the special floating-point value -0.0f, so this function returns \c true for this value.
inline bool sign_bit(Float64 s)
{
  return (base::binary_cast<Uint64>(s) & (1ULL << 63)) != 0ULL;
}

#if (__cplusplus < 201103L)
/// Checks a single-precision floating point number for "not a number".
///
/// The methods relies on the IEEE 754 floating-point standard.
inline bool isnan MI_PREVENT_MACRO_EXPAND(const Float32 x)
{
  // interpret as Uint32 value
  const Uint32 f = base::binary_cast<Uint32>(x);

  // check bit pattern
  return (f << 1) > 0xFF000000U; // shift sign bit, 8bit exp == 2^8-1, fraction != 0
}

/// Checks a double-precision floating point number for "not a number".
///
/// The methods relies on the IEEE 754 floating-point standard.
inline bool isnan MI_PREVENT_MACRO_EXPAND(const Float64 x)
{
  // interpret as Uint64 value
  const Uint64 f = base::binary_cast<Uint64>(x);

  return (f << 1) > 0xFFE0000000000000ULL; // shift sign bit, 11bit exp == 2^11-1, fraction != 0
}
#else
using std::isnan;
#endif

/// Checks a single-precision floating point number for "infinity".
///
/// The methods relies on the IEEE 754 floating-point standard.
inline bool isinfinite MI_PREVENT_MACRO_EXPAND(const Float32 x)
{
  const Uint32 exponent_mask = 0x7F800000; // 8 bit exponent
  const Uint32 fraction_mask = 0x7FFFFF;   // 23 bit fraction

  // interpret as Uint32 value
  const Uint32 f = base::binary_cast<Uint32>(x);

  // check bit pattern
  return ((f & exponent_mask) == exponent_mask) && // exp == 2^8 - 1
    ((f & fraction_mask) == 0);                    // fraction == 0
}

/// Checks a double-precision floating point number for "infinity".
///
/// The methods relies on the IEEE 754 floating-point standard.
inline bool isinfinite MI_PREVENT_MACRO_EXPAND(const Float64 x)
{
  const Uint64 exponent_mask = 0x7FF0000000000000ULL; // 11 bit exponent
  const Uint64 fraction_mask = 0xFFFFFFFFFFFFFULL;    // 52 bit fraction

  // interpret as Uint64 value
  const Uint64 f = base::binary_cast<Uint64>(x);

  // check bit pattern
  return ((f & exponent_mask) == exponent_mask) && // exp == 2^11 - 1
    ((f & fraction_mask) == 0);                    // fraction == 0
}

#if (__cplusplus < 201103L)
/// Checks a single-precision floating point number for neither "not a number" nor "infinity".
///
/// The methods relies on the IEEE 754 floating-point standard. Note that the result of this
/// function might differ from negating the result value of #isinfinite() because of the "not a
/// number" check.
inline bool isfinite MI_PREVENT_MACRO_EXPAND(const Float32 x)
{
  const Uint32 exponent_mask = 0x7F800000; // 8 bit exponent

  // interpret as Uint32 value
  const Uint32 f = base::binary_cast<Uint32>(x);

  // check exponent bits
  return ((f & exponent_mask) != exponent_mask); // exp != 2^8 - 1
}

/// Checks a double-precision floating point number for neither "not a number" nor "infinity".
///
/// The methods relies on the IEEE 754 floating-point standard. Note that the result of this
/// function might differ from negating the result value of #isinfinite() because of the "not a
/// number" check.
inline bool isfinite MI_PREVENT_MACRO_EXPAND(const Float64 x)
{
  const Uint64 exponent_mask = 0x7FF0000000000000ULL; // 11 bit exponent

  // interpret as Uint64 value
  const Uint64 f = base::binary_cast<Uint64>(x);

  // check exponent bits
  return ((f & exponent_mask) != exponent_mask); // exp != 2^11 - 1
}
#else
using std::isfinite;
#endif

/// Returns the sine of \p a. The angle \p a is specified in radians.
inline Float32 sin(Float32 a)
{
  return std::sin(a);
}
/// Returns the sine of \p a. The angle \p a is specified in radians.
inline Float64 sin(Float64 a)
{
  return std::sin(a);
}

/// Computes the sine \p s and cosine \p c of angle \p a simultaneously.
///
/// The angle \p a  is specified in radians.
inline void sincos(Float32 a, Float32& s, Float32& c)
{
  s = std::sin(a);
  c = std::cos(a);
}
/// Computes the sine \p s and cosine \p c of angle \p a simultaneously.
///
/// The angle \p a  is specified in radians.
inline void sincos(Float64 a, Float64& s, Float64& c)
{
  s = std::sin(a);
  c = std::cos(a);
}

/// Returns 0 if \p x is less than \p a and 1 if \p x is greater than \p b.
///
/// A smooth curve is applied in-between so that the return value varies continuously from 0 to 1 as
/// \p x varies from \p a to \p b.
inline Float32 smoothstep(Float32 a, Float32 b, Float32 x)
{
  if (x < a)
    return 0.0f;
  if (b < x)
    return 1.0f;
  Float32 t = (x - a) / (b - a);
  return t * t * (3.0f - 2.0f * t);
}
/// Returns 0 if \p x is less than \p a and 1 if \p x is greater than \p b.
///
/// A smooth curve is applied in-between so that the return value varies continuously from 0 to 1 as
/// \p x varies from \p a to \p b.
inline Float64 smoothstep(Float64 a, Float64 b, Float64 x)
{
  if (x < a)
    return 0.0;
  if (b < x)
    return 1.0;
  Float64 t = (x - a) / (b - a);
  return t * t * (3.0 - 2.0 * t);
}

/// Returns the square root of \p s.
inline Float32 sqrt(Float32 s)
{
  return std::sqrt(s);
}
/// Returns the square root of \p s.
inline Float64 sqrt(Float64 s)
{
  return std::sqrt(s);
}

/// Returns 0 if \p x is less than \p a and 1 otherwise.
inline Float32 step(Float32 a, Float32 x)
{
  return (x < a) ? 0.0f : 1.0f;
}
/// Returns 0 if \p x is less than \p a and 1 otherwise.
inline Float64 step(Float64 a, Float64 x)
{
  return (x < a) ? 0.0 : 1.0;
}

/// Returns the tangent of \p a. The angle \p a is specified in radians.
inline Float32 tan(Float32 a)
{
  return std::tan(a);
}
/// Returns the tangent of \p a. The angle \p a is specified in radians.
inline Float64 tan(Float64 a)
{
  return std::tan(a);
}

/// Encodes a color into RGBE representation.
inline void to_rgbe(const Float32 color[3], Uint32& rgbe)
{
  Float32 c[3];
  c[0] = mi::base::max(color[0], 0.0f);
  c[1] = mi::base::max(color[1], 0.0f);
  c[2] = mi::base::max(color[2], 0.0f);

  const Float32 max = mi::base::max(mi::base::max(c[0], c[1]), c[2]);

  // should actually be -126 or even -128, but avoid precision problems / denormalized numbers
  if (max <= 7.5231631727e-37f) // ~2^(-120)
    rgbe = 0;
  else if (max >= 1.7014118346046923173168730371588e+38f) // 2^127
    rgbe = 0xFFFFFFFFu;
  else
  {
    const Uint32 e = base::binary_cast<Uint32>(max) & 0x7F800000u;
    const Float32 v = base::binary_cast<Float32>(0x82800000u - e);

    rgbe =
      Uint32(c[0] * v) | (Uint32(c[1] * v) << 8) | (Uint32(c[2] * v) << 16) | (e * 2 + (2 << 24));
  }
}

/// Encodes a color into RGBE representation.
inline void to_rgbe(const Float32 color[3], Uint8 rgbe[4])
{
  Float32 c[3];
  c[0] = mi::base::max(color[0], 0.0f);
  c[1] = mi::base::max(color[1], 0.0f);
  c[2] = mi::base::max(color[2], 0.0f);

  const Float32 max = mi::base::max(mi::base::max(c[0], c[1]), c[2]);

  // should actually be -126 or even -128, but avoid precision problems / denormalized numbers
  if (max <= 7.5231631727e-37f) // ~2^(-120)
    rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
  else if (max >= 1.7014118346046923173168730371588e+38f) // 2^127
    rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 255;
  else
  {
    const Uint32 e = base::binary_cast<Uint32>(max) & 0x7F800000u;
    const Float32 v = base::binary_cast<Float32>(0x82800000u - e);

    rgbe[0] = Uint8(c[0] * v);
    rgbe[1] = Uint8(c[1] * v);
    rgbe[2] = Uint8(c[2] * v);
    rgbe[3] = Uint8((e >> 23) + 2);
  }
}

/// Decodes a color from RGBE representation.
inline void from_rgbe(const Uint8 rgbe[4], Float32 color[3])
{
  if (rgbe[3] == 0)
  {
    color[0] = color[1] = color[2] = 0.0f;
    return;
  }

  const Uint32 e = (static_cast<Uint32>(rgbe[3]) << 23) - 0x800000u;
  const Float32 v = base::binary_cast<Float32>(e);
  const Float32 c = static_cast<Float32>(1.0 - 0.5 / 256.0) * v;

  color[0] = base::binary_cast<Float32>(e | (static_cast<Uint32>(rgbe[0]) << 15)) - c;
  color[1] = base::binary_cast<Float32>(e | (static_cast<Uint32>(rgbe[1]) << 15)) - c;
  color[2] = base::binary_cast<Float32>(e | (static_cast<Uint32>(rgbe[2]) << 15)) - c;
}

/// Decodes a color from RGBE representation.
inline void from_rgbe(const Uint32 rgbe, Float32 color[3])
{
  const Uint32 rgbe3 = rgbe & 0xFF000000u;
  if (rgbe3 == 0)
  {
    color[0] = color[1] = color[2] = 0.0f;
    return;
  }

  const Uint32 e = (rgbe3 >> 1) - 0x800000u;
  const Float32 v = base::binary_cast<Float32>(e);
  const Float32 c = static_cast<Float32>(1.0 - 0.5 / 256.0) * v;

  color[0] = base::binary_cast<Float32>(e | ((rgbe << 15) & 0x7F8000u)) - c;
  color[1] = base::binary_cast<Float32>(e | ((rgbe << 7) & 0x7F8000u)) - c;
  color[2] = base::binary_cast<Float32>(e | ((rgbe >> 1) & 0x7F8000u)) - c;
}

//------ Generic Vector Algorithms --------------------------------------------

// overloads for 1D vectors (scalars)

/// Returns the inner product (a.k.a. dot or scalar product) of two integers.
inline Sint32 dot(Sint32 a, Sint32 b)
{
  return a * b;
}
/// Returns the inner product (a.k.a. dot or scalar product) of two scalars.
inline Float32 dot(Float32 a, Float32 b)
{
  return a * b;
}
/// Returns the inner product (a.k.a. dot or scalar product) of two scalars.
inline Float64 dot(Float64 a, Float64 b)
{
  return a * b;
}

/// Returns the inner product (a.k.a. dot or scalar product) of two vectors.
template <class V>
inline typename V::value_type dot(const V& lhs, const V& rhs)
{
  typename V::value_type v(0);
  for (Size i(0u); i < V::SIZE; ++i)
    v += lhs.get(i) * rhs.get(i);
  return v;
}

/// Returns the squared Euclidean norm of the vector \p v.
template <class V>
inline typename V::value_type square_length(const V& v)
{
  return dot(v, v);
}

// base case for scalars

/// Returns the Euclidean norm of the scalar \p a (its absolute value).
inline Float32 length(Float32 a)
{
  return abs(a);
}
/// Returns the Euclidean norm of the scalar \p a (its absolute value).
inline Float64 length(Float64 a)
{
  return abs(a);
}

/// Returns the Euclidean norm of the vector \p v.
///
/// Uses an unqualified call to \c sqrt(...) on the vector element type.
template <class V>
inline typename V::value_type length(const V& v)
{
  return sqrt(square_length(v));
}

/// Returns the squared Euclidean distance from the vector \p lhs to the vector \p rhs.
template <class V>
inline typename V::value_type square_euclidean_distance(const V& lhs, const V& rhs)
{
  return square_length(lhs - rhs);
}

/// Returns the Euclidean distance from the vector \p lhs to the vector \p rhs.
///
/// Uses an unqualified call to \c sqrt(...) on the vector element type.
template <class V>
inline typename V::value_type euclidean_distance(const V& lhs, const V& rhs)
{
  return length(lhs - rhs);
}

/// Bounds the value of vector \p v elementwise to the given \p low and \p high vector values.
template <class V>
inline void set_bounds(V& v, const V& low, const V& high)
{
  for (Size i(0u); i < V::SIZE; ++i)
    v[i] =
      min MI_PREVENT_MACRO_EXPAND(max MI_PREVENT_MACRO_EXPAND(v.get(i), low.get(i)), high.get(i));
}

/// Returns \c true if vector \p lhs is elementwise equal to vector \p rhs,
/// and \c false otherwise.
template <class V>
inline bool is_equal(const V& lhs, const V& rhs)
{
  for (Size i(0u); i < V::SIZE; ++i)
    if (!(lhs.get(i) == rhs.get(i)))
      return false;
  return true;
}

/// Returns \c true if vector \p lhs is elementwise not equal to vector \p rhs,
/// and \c false otherwise.
template <class V>
inline bool is_not_equal(const V& lhs, const V& rhs)
{
  for (Size i(0u); i < V::SIZE; ++i)
    if (lhs.get(i) != rhs.get(i))
      return true;
  return false;
}

/// Returns \c true if vector \p lhs is lexicographically less than vector \p rhs,
/// and \c false otherwise.
///
/// \see   \ref mi_def_lexicographic_order
template <class V>
inline bool lexicographically_less(const V& lhs, const V& rhs)
{
  for (Size i(0u); i < V::SIZE - 1; ++i)
  {
    if (lhs.get(i) < rhs.get(i))
      return true;
    if (lhs.get(i) > rhs.get(i))
      return false;
  }
  return lhs.get(V::SIZE - 1) < rhs.get(V::SIZE - 1);
}

/// Returns \c true if vector \p lhs is lexicographically less than or equal to vector \p rhs,
/// and \c false otherwise.
///
/// \see   \ref mi_def_lexicographic_order
template <class V>
inline bool lexicographically_less_or_equal(const V& lhs, const V& rhs)
{
  for (Size i(0u); i < V::SIZE - 1; ++i)
  {
    if (lhs.get(i) < rhs.get(i))
      return true;
    if (lhs.get(i) > rhs.get(i))
      return false;
  }
  return lhs.get(V::SIZE - 1) <= rhs.get(V::SIZE - 1);
}

/// Returns \c true if vector \p lhs is lexicographically greater than vector \p rhs,
/// and \c false otherwise.
///
/// \see   \ref mi_def_lexicographic_order
template <class V>
inline bool lexicographically_greater(const V& lhs, const V& rhs)
{
  for (Size i(0u); i < V::SIZE - 1; ++i)
  {
    if (lhs.get(i) > rhs.get(i))
      return true;
    if (lhs.get(i) < rhs.get(i))
      return false;
  }
  return lhs.get(V::SIZE - 1) > rhs.get(V::SIZE - 1);
}

/// Returns \c true if vector \p lhs is lexicographically greater than or equal to vector \p rhs,
/// and \c false otherwise.
///
/// \see   \ref mi_def_lexicographic_order
template <class V>
inline bool lexicographically_greater_or_equal(const V& lhs, const V& rhs)
{
  for (Size i(0u); i < V::SIZE - 1; ++i)
  {
    if (lhs.get(i) > rhs.get(i))
      return true;
    if (lhs.get(i) < rhs.get(i))
      return false;
  }
  return lhs.get(V::SIZE - 1) >= rhs.get(V::SIZE - 1);
}

/// Compares two vectors lexicographically.
///
/// Returns \c LESS if \p lhs is less than \p rhs, and correspondingly \c EQUAL or \c GREATER for
/// the other cases.
///
/// \note The result of this function is undefined if \p lhs or \p rhs contains NaNs.
///
/// \see #mi::Comparison_result,
///      \ref mi_def_lexicographic_order
template <class V>
inline Comparison_result lexicographically_compare(const V& lhs, const V& rhs)
{
  for (Size i(0u); i < V::SIZE; ++i)
  {
    Comparison_result result = three_valued_compare(lhs.get(i), rhs.get(i));
    if (result != EQUAL)
      return result;
  }
  return EQUAL;
}

/*@}*/ // end group mi_math_function

} // namespace math

} // namespace mi

#endif // MI_MATH_FUNCTION_H
