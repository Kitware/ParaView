/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/types.h
/// \brief Basic types.
///
/// See \ref mi_base_types.

#ifndef MI_BASE_TYPES_H
#define MI_BASE_TYPES_H

#include <cmath>   // declare abs overloads
#include <cstddef> // define  size_t, ptrdiff_t
#include <cstdlib> // declare abs
#include <mi/base/assert.h>
#include <mi/base/config.h>

#ifndef MI_BASE_NO_STL
#include <algorithm> // define  min, max
#endif               // MI_BASE_NO_STL

namespace mi
{

/** \defgroup mi_base_types Basic Types
    \ingroup mi_base
    \brief Platform-independent types.

       \par Include File:
       <tt> \#include <mi/base/types.h></tt>

    @{
 */

// primitive types of known bit size

typedef signed char Sint8;     ///<  8-bit   signed integer.
typedef signed short Sint16;   ///< 16-bit   signed integer.
typedef signed int Sint32;     ///< 32-bit   signed integer.
typedef unsigned char Uint8;   ///<  8-bit unsigned integer.
typedef unsigned short Uint16; ///< 16-bit unsigned integer.
typedef unsigned int Uint32;   ///< 32-bit unsigned integer.

typedef float Float32;  ///< 32-bit float.
typedef double Float64; ///< 64-bit float.

#if defined(MI_COMPILER_MSC)

typedef signed __int64 Sint64;   ///< 64-bit   signed integer.
typedef unsigned __int64 Uint64; ///< 64-bit unsigned integer.

#elif defined(MI_COMPILER_GCC) // defined(MI_COMPILER_MSC)

typedef long long Sint64;          ///< 64-bit   signed integer.
typedef unsigned long long Uint64; ///< 64-bit unsigned integer.

#else // defined(MI_COMPILER_GCC)

typedef signed long long Sint64;   ///< 64-bit   signed integer.
typedef unsigned long long Uint64; ///< 64-bit unsigned integer.

#endif // defined(MI_COMPILER_GCC)

mi_static_assert(sizeof(Sint8) == 1);
mi_static_assert(sizeof(Sint16) == 2);
mi_static_assert(sizeof(Sint32) == 4);
mi_static_assert(sizeof(Sint64) == 8);
mi_static_assert(sizeof(Uint8) == 1);
mi_static_assert(sizeof(Uint16) == 2);
mi_static_assert(sizeof(Uint32) == 4);
mi_static_assert(sizeof(Uint64) == 8);
mi_static_assert(sizeof(Float32) == 4);
mi_static_assert(sizeof(Float64) == 8);

/// The \c printf format specifier for #mi::Sint64.
///
/// \code
///     mi::Sint64 x = ...;
///     printf( "x equals %" MI_BASE_FMT_MI_SINT64 " \n", x);
/// \endcode
#ifdef MI_ARCH_64BIT
#define MI_BASE_FMT_MI_SINT64 "lld"
#else // MI_ARCH_64BIT
#define MI_BASE_FMT_MI_SINT64 "d"
#endif // MI_ARCH_64BIT

/// The \c printf format specifier for #mi::Uint64.
///
/// \code
///     mi::Uint64 x = ...;
///     printf( "x equals %" MI_BASE_FMT_MI_UINT64 " \n", x);
/// \endcode
#ifdef MI_ARCH_64BIT
#define MI_BASE_FMT_MI_UINT64 "llu"
#else // MI_ARCH_64BIT
#define MI_BASE_FMT_MI_UINT64 "u"
#endif // MI_ARCH_64BIT

/// Unsigned integral type that is large enough to hold the size of all types. This type is for
/// example used for dimensions and indices of vectors.
///
/// It corresponds to a 32-bit unsigned integer on 32-bit architectures and a 64-bit unsigned
/// integer on 64-bit architectures.
#ifdef MI_ARCH_64BIT
typedef Uint64 Size;
#else  // MI_ARCH_64BIT
typedef Uint32 Size;
#endif // MI_ARCH_64BIT

/// Signed integral type that is large enough to hold the difference of two pointers.
///
/// It corresponds to a 32-bit signed integer on 32-bit architectures and a 64-bit signed integer on
/// 64-bit architectures.
#ifdef MI_ARCH_64BIT
typedef Sint64 Difference;
#else  // MI_ARCH_64BIT
typedef Sint32 Difference;
#endif // MI_ARCH_64BIT

#ifdef MI_ARCH_64BIT
mi_static_assert(sizeof(Size) == 8);
mi_static_assert(sizeof(Difference) == 8);
#else // MI_ARCH_64BIT
mi_static_assert(sizeof(Size) == 4);
mi_static_assert(sizeof(Difference) == 4);
#endif // MI_ARCH_64BIT

/// The maximum value for \c %Size.
///
/// \note The actual value of this constant depends on the architecture. Possible values are
///       18446744073709551615 on 64-bit architectures and 4294967295 on 32-bit architectures.
#ifdef MI_ARCH_64BIT
static const Size SIZE_MAX_VALUE = 18446744073709551615ULL;
#else  // MI_ARCH_64BIT
static const Size SIZE_MAX_VALUE = 4294967295U;
#endif // MI_ARCH_64BIT

/// The minimum value for \c %Difference.
///
/// \note The actual value of this constant depends on the architecture. Possible values are
///       -9223372036854775808 on 64-bit architectures and -2147483648 on 32-bit architectures.
#ifdef MI_ARCH_64BIT
static const Difference DIFFERENCE_MIN_VALUE = -9223372036854775807LL - 1LL;
#else  // MI_ARCH_64BIT
static const Difference DIFFERENCE_MIN_VALUE = -2147483647 - 1;
#endif // MI_ARCH_64BIT

/// The maximum value for \c %Difference.
///
/// \note The actual value of this constant depends on the architecture. Possible values are
///       9223372036854775807 on 64-bit architectures and 2147483647 on 32-bit architectures.
#ifdef MI_ARCH_64BIT
static const Difference DIFFERENCE_MAX_VALUE = 9223372036854775807LL;
#else  // MI_ARCH_64BIT
static const Difference DIFFERENCE_MAX_VALUE = 2147483647;
#endif // MI_ARCH_64BIT

/// The \c printf format specifier for #mi::Size.
///
/// \code
///     mi::Size x = ...;
///     printf( "x equals %" MI_BASE_FMT_MI_SIZE " \n", x);
/// \endcode
#ifdef MI_ARCH_64BIT
#define MI_BASE_FMT_MI_SIZE "llu"
#else // MI_ARCH_64BIT
#define MI_BASE_FMT_MI_SIZE "u"
#endif // MI_ARCH_64BIT

/// The \c printf format specifier for #mi::Difference.
///
/// \code
///     mi::Difference x = ...;
///     printf( "x equals %" MI_BASE_FMT_MI_DIFFERENCE " \n", x);
/// \endcode
#ifdef MI_ARCH_64BIT
#define MI_BASE_FMT_MI_DIFFERENCE "lld"
#else // MI_ARCH_64BIT
#define MI_BASE_FMT_MI_DIFFERENCE "d"
#endif // MI_ARCH_64BIT

// primitive types related constants

/// Value of Pi.
#define MI_PI 3.14159265358979323846
/// Value of Pi / 2.
#define MI_PI_2 1.57079632679489661923
/// Value of Pi / 4.
#define MI_PI_4 0.78539816339744830962

/// An enum for a three-valued comparison result.
///
/// The three values, -1, 0, and 1, have several symbolic names that can be used interchangeable
/// depending on the context. The symbolic names group together as indicated in their order.
enum Comparison_result
{
  NEGATIVE = -1, ///< = -1. Sign of a value is negative.
  ZERO = 0,      ///< =  0. Value is zero.
  POSITIVE = 1,  ///< = +1. Sign of a value is positive.

  LESS = -1,  ///< = -1. First value is less than second value.
  EQUAL = 0,  ///< =  0. First value is equal to second value.
  GREATER = 1 ///< = +1. First value is greater than second value.
};

/// Reverses the sign of a three valued enum.
inline Comparison_result operator-(Comparison_result sign)
{
  return Comparison_result(-static_cast<int>(sign));
}

/// Returns the three valued sign for a numerical type \c T.
///
/// \pre The value of type \c T must be comparable against 0.
///
/// \note The result of this function is undefined if \p t is NaN.
template <typename T>
inline Comparison_result three_valued_sign(T t)
{
  if (t < 0)
    return NEGATIVE;
  else if (t > 0)
    return POSITIVE;
  else
    return ZERO;
}

/// Returns the three valued comparison result between two values of a numerical type \c T.
///
/// \pre Values of type \c T must be comparable.
///
/// \note The result of this function is undefined if \p lhs or \p rhs is NaN.
template <typename T>
inline Comparison_result three_valued_compare(T lhs, T rhs)
{
  if (lhs < rhs)
    return LESS;
  else if (rhs < lhs)
    return GREATER;
  else
    return EQUAL;
}

namespace base
{

// Define min/max within mi::base if and only if no STL usage is selected by defining the
// MI_BASE_NO_STL macro. Use std definitions otherwise.
#ifdef MI_BASE_NO_STL

template <class T>
inline const T& min MI_PREVENT_MACRO_EXPAND(const T& a, const T& b)
{
  return a < b ? a : b;
}

template <class T>
inline const T& max MI_PREVENT_MACRO_EXPAND(const T& a, const T& b)
{
  return a > b ? a : b;
}

#else // MI_BASE_NO_STL

using std::min;
using std::max;

#endif // MI_BASE_NO_STL

// For simple types, we add variants that pass the arguments by value. This can result in better
// performance on some compilers.
inline Sint8 min MI_PREVENT_MACRO_EXPAND(const Sint8 a, const Sint8 b)
{
  return a < b ? a : b;
}
inline Sint16 min MI_PREVENT_MACRO_EXPAND(const Sint16 a, const Sint16 b)
{
  return a < b ? a : b;
}
inline Sint32 min MI_PREVENT_MACRO_EXPAND(const Sint32 a, const Sint32 b)
{
  return a < b ? a : b;
}
inline Sint64 min MI_PREVENT_MACRO_EXPAND(const Sint64 a, const Sint64 b)
{
  return a < b ? a : b;
}
inline Uint8 min MI_PREVENT_MACRO_EXPAND(const Uint8 a, const Uint8 b)
{
  return a < b ? a : b;
}
inline Uint16 min MI_PREVENT_MACRO_EXPAND(const Uint16 a, const Uint16 b)
{
  return a < b ? a : b;
}
inline Uint32 min MI_PREVENT_MACRO_EXPAND(const Uint32 a, const Uint32 b)
{
  return a < b ? a : b;
}
inline Uint64 min MI_PREVENT_MACRO_EXPAND(const Uint64 a, const Uint64 b)
{
  return a < b ? a : b;
}
inline Float32 min MI_PREVENT_MACRO_EXPAND(const Float32 a, const Float32 b)
{
  return a < b ? a : b;
}
inline Float64 min MI_PREVENT_MACRO_EXPAND(const Float64 a, const Float64 b)
{
  return a < b ? a : b;
}

inline Sint8 max MI_PREVENT_MACRO_EXPAND(const Sint8 a, const Sint8 b)
{
  return a > b ? a : b;
}
inline Sint16 max MI_PREVENT_MACRO_EXPAND(const Sint16 a, const Sint16 b)
{
  return a > b ? a : b;
}
inline Sint32 max MI_PREVENT_MACRO_EXPAND(const Sint32 a, const Sint32 b)
{
  return a > b ? a : b;
}
inline Sint64 max MI_PREVENT_MACRO_EXPAND(const Sint64 a, const Sint64 b)
{
  return a > b ? a : b;
}
inline Uint8 max MI_PREVENT_MACRO_EXPAND(const Uint8 a, const Uint8 b)
{
  return a > b ? a : b;
}
inline Uint16 max MI_PREVENT_MACRO_EXPAND(const Uint16 a, const Uint16 b)
{
  return a > b ? a : b;
}
inline Uint32 max MI_PREVENT_MACRO_EXPAND(const Uint32 a, const Uint32 b)
{
  return a > b ? a : b;
}
inline Uint64 max MI_PREVENT_MACRO_EXPAND(const Uint64 a, const Uint64 b)
{
  return a > b ? a : b;
}
inline Float32 max MI_PREVENT_MACRO_EXPAND(const Float32 a, const Float32 b)
{
  return a > b ? a : b;
}
inline Float64 max MI_PREVENT_MACRO_EXPAND(const Float64 a, const Float64 b)
{
  return a > b ? a : b;
}

// Take the abs function overloads from the Standard Library header cmath.

/// Returns the absolute value of argument \p val.
///
/// For integer types, trying to take the absolute value of the most negative integer is not
/// defined. Type \c T must support construction from 0, less than comparison, and negation.
///
/// \param val   Argument of which the absolute value will be returned.
/// \return      The absolute value of \p val.
using std::abs;

namespace
{
// helper class for the \c binary_cast function defined below
template <class Target, class Source>
union Binary_cast {
  Source source;
  Target target;
};
}

/// Cast an immutable 'Source' value to an immutable 'Target' value. Use this function as follows:
/// \code
///     float  fval( 0.0f );
///     Uint32 uval( binary_cast<Uint32>(fval) );
/// \endcode
template <class Target, class Source>
inline Target binary_cast(Source const& val)
{
  mi_static_assert(sizeof(Source) == sizeof(Target));
  Binary_cast<Target, Source> val_;
  val_.source = val;
  return val_.target;
}

/// %Base class for the helper class to deduce properties of numeric types defined in this API.
///
/// This class provides the common default implementations of all member of \c numeric_traits, for
/// its default template as well as all specialization. Thus, specializations just have to redefine
/// what changes.
///
/// The default uses default constructed values. It therefore requires that a value of \c T can be
/// default constructed.
template <typename T>
struct numeric_traits_base
{
  /// Constant that is \c true if and only if this traits is specialized for \c T.
  static const bool is_specialized = false;

  /// Constant that is \c true if and only if \c T has an infinity representation.
  ///
  /// You can get the infinity representation through the \c infinity() method.
  static const bool has_infinity = false;

  /// Constant that is \c true if and only if \c T has a quiet NaN (not-a-number) representation.
  ///
  /// You can get the quiet NaN representation through the \c quiet_NaN() method.
  static const bool has_quiet_NaN = false;

  /// Constant that is \c true if and only if \c T has a signaling NaN (not-a-number)
  /// representation.
  ///
  /// You can get the signaling NaN representation through the \c signaling_NaN() method.
  static const bool has_signaling_NaN = false;

  /// Returns the minimum finite value for \c T (and for floating point types the minimum positive
  /// value).
  ///
  /// The value is equivalent to standard C constants \c CHAR_MIN, \c SHRT_MIN, \c FLT_MIN, \c
  /// DBL_MIN, etc.
  ///
  /// \return the minimum finite value for \c T.
  static T(min)() throw() { return T(); }

  /// Returns the maximum finite value for \c T.
  ///
  /// The value is equivalent to standard C constants \c CHAR_MAX, \c SHRT_MAX, \c FLT_MAX, \c
  /// DBL_MAX, etc.
  ///
  /// \return the maximum finite value for \c T.
  static T(max)() throw() { return T(); }

  /// Returns the smallest finite negative value for \c T.
  ///
  /// For signed integer types this value is typically \c (-max()-1). For unsigned integer types
  /// it is 0. For floating point types it is typically \c (-max()).
  ///
  /// \return the smallest finite negative value for \c T.
  static T negative_max() throw() { return T(); }

  /// Returns an infinity value for \c T, if one exists, and \c T() otherwise.
  static T infinity() throw() { return T(); }

  /// Returns a quiet NaN value for \c T, if one exists, and \c T() otherwise.
  static T quiet_NaN() throw() { return T(); }

  /// Returns a signaling NaN value for \c T, if one exists, and \c T() otherwise.
  static T signaling_NaN() throw() { return T(); }
};

/// Helper class to deduce properties of numeric types defined in this API.
///
/// This class can be used in generic code to deduce the limits and special values, such as a
/// signaling NaN, for number types defined in this API, such as #mi::Uint32 and #mi::Float64.
///
/// The default template defines all members, see #mi::base::numeric_traits_base, but with default
/// constructed or \c false values. It requires that a value of \c T can be default constructed.
///
/// This traits class is similar to \c std::numeric_limits<T>, and in case of overlap behaves in the
/// same way.
template <typename T>
struct numeric_traits : public numeric_traits_base<T>
{
};

/** \defgroup mi_base_number_traits_specialization Specializations for numeric_traits
    \ingroup mi_base_types
    \brief The number traits class is specialized for number types defined in this API, such as
    #mi::Uint32 and #mi::Float64.

       \par Include File:
       <tt> \#include <mi/base/types.h></tt>

    @{
 */

/// Numeric traits specialization for #mi::Sint8.
template <>
struct numeric_traits<Sint8> : public numeric_traits_base<Sint8>
{
  static const bool is_specialized = true;             ///< \c true for specialization.
  static Sint8(min)() throw() { return -128; }         ///< SCHAR_MIN
  static Sint8(max)() throw() { return 127; }          ///< SCHAR_MAX
  static Sint8 negative_max() throw() { return -128; } ///< SCHAR_MIN
};

/// Numeric traits specialization for #mi::Sint16.
template <>
struct numeric_traits<Sint16> : public numeric_traits_base<Sint16>
{
  static const bool is_specialized = true;                ///< \c true for specialization.
  static Sint16(min)() throw() { return -32768; }         ///< SHRT_MIN
  static Sint16(max)() throw() { return 32767; }          ///< SHRT_MAX
  static Sint16 negative_max() throw() { return -32768; } ///< SHRT_MIN
};

/// Numeric traits specialization for #mi::Sint32.
template <>
struct numeric_traits<Sint32> : public numeric_traits_base<Sint32>
{
  static const bool is_specialized = true;                         ///< \c true for specialization.
  static Sint32(min)() throw() { return -2147483647 - 1; }         ///< INT_MIN
  static Sint32(max)() throw() { return 2147483647; }              ///< INT_MAX
  static Sint32 negative_max() throw() { return -2147483647 - 1; } ///< INT_MIN
};

/// Numeric traits specialization for #mi::Sint64.
template <>
struct numeric_traits<Sint64> : public numeric_traits_base<Sint64>
{
  static const bool is_specialized = true; ///< \c true for specialization.
  static Sint64(min)() throw() { return -9223372036854775807LL - 1LL; }         ///< LLONG_MIN
  static Sint64(max)() throw() { return 9223372036854775807LL; }                ///< LLONG_MAX
  static Sint64 negative_max() throw() { return -9223372036854775807LL - 1LL; } ///< LLONG_MIN
};

/// Numeric traits specialization for #mi::Uint8.
template <>
struct numeric_traits<Uint8> : public numeric_traits_base<Uint8>
{
  static const bool is_specialized = true;    ///< \c true for specialization.
  static Uint8(max)() throw() { return 255; } ///< UCHAR_MAX
};

/// Numeric traits specialization for #mi::Uint16.
template <>
struct numeric_traits<Uint16> : public numeric_traits_base<Uint16>
{
  static const bool is_specialized = true;       ///< \c true for specialization.
  static Uint16(max)() throw() { return 65535; } ///< USHRT_MAX
};

/// Numeric traits specialization for #mi::Uint32.
template <>
struct numeric_traits<Uint32> : public numeric_traits_base<Uint32>
{
  static const bool is_specialized = true;             ///< \c true for specialization.
  static Uint32(max)() throw() { return 4294967295U; } ///< UINT_MAX
};

/// Numeric traits specialization for #mi::Uint64.
template <>
struct numeric_traits<Uint64> : public numeric_traits_base<Uint64>
{
  static const bool is_specialized = true;                         ///< \c true for specialization.
  static Uint64(max)() throw() { return 18446744073709551615ULL; } ///< ULLONG_MAX
};

// Architecture dependent definition of quiet NaN
#ifdef MI_ARCH_BIG_ENDIAN
#define MI__FLOAT32_INF_REP                                                                        \
  {                                                                                                \
    0x7f80, 0                                                                                      \
  }
#define MI__FLOAT32_QNAN_REP                                                                       \
  {                                                                                                \
    0x7fc1, 0                                                                                      \
  }
#define MI__FLOAT32_SNAN_REP                                                                       \
  {                                                                                                \
    0x7f81, 0                                                                                      \
  }
#define MI__FLOAT64_INF_REP                                                                        \
  {                                                                                                \
    0x7ff0, 0, 0, 0                                                                                \
  }
#define MI__FLOAT64_QNAN_REP                                                                       \
  {                                                                                                \
    0x7ff9, 0, 0, 0                                                                                \
  }
#define MI__FLOAT64_SNAN_REP                                                                       \
  {                                                                                                \
    0x7ff1, 0, 0, 0                                                                                \
  }
#endif // MI_ARCH_BIG_ENDIAN
#ifdef MI_ARCH_LITTLE_ENDIAN
#define MI__FLOAT32_INF_REP                                                                        \
  {                                                                                                \
    0, 0x7f80                                                                                      \
  }
#define MI__FLOAT32_QNAN_REP                                                                       \
  {                                                                                                \
    0, 0x7fc0                                                                                      \
  }
#define MI__FLOAT32_SNAN_REP                                                                       \
  {                                                                                                \
    0, 0x7fa0                                                                                      \
  }
#define MI__FLOAT64_INF_REP                                                                        \
  {                                                                                                \
    0, 0, 0, 0x7ff0                                                                                \
  }
#define MI__FLOAT64_QNAN_REP                                                                       \
  {                                                                                                \
    0, 0, 0, 0x7ff8                                                                                \
  }
#define MI__FLOAT64_SNAN_REP                                                                       \
  {                                                                                                \
    0, 0, 0, 0x7ff4                                                                                \
  }
#endif // MI_ARCH_LITTLE_ENDIAN

namespace
{
// Float number type representation for bitwise inits with NaNs
union Float32_rep {
  Uint16 rep[2];
  Float32 val;
};
union Float64_rep {
  Uint16 rep[4];
  Float64 val;
};
}

/// Numeric traits specialization for #mi::Float32.
template <>
struct numeric_traits<Float32> : public numeric_traits_base<Float32>
{
  static const bool is_specialized = true;                   ///< \c true for specialization.
  static const bool has_infinity = true;                     ///< Has infinity value.
  static const bool has_quiet_NaN = true;                    ///< Has a quiet NaN value.
  static const bool has_signaling_NaN = true;                ///< Has a signaling NaN value.
  static Float32(min)() throw() { return 1.17549435e-38F; }  ///<  FLT_MIN
  static Float32(max)() throw() { return 3.402823466e+38F; } ///<  FLT_MAX
  static Float32 negative_max() throw() { return -3.402823466e+38F; } ///< -FLT_MAX
  static Float32 infinity() throw()
  { ///< Infinity.
    Float32_rep rep = { MI__FLOAT32_INF_REP };
    return rep.val;
  }
  static Float32 quiet_NaN() throw()
  { ///< Quiet NaN.
    Float32_rep rep = { MI__FLOAT32_QNAN_REP };
    return rep.val;
  }
  static Float32 signaling_NaN() throw()
  { ///< Signaling NaN.
    Float32_rep rep = { MI__FLOAT32_SNAN_REP };
    return rep.val;
  }
};

/// Numeric traits specialization for #mi::Float64.
template <>
struct numeric_traits<Float64> : public numeric_traits_base<Float64>
{
  static const bool is_specialized = true;                          ///< \c true for specialization.
  static const bool has_infinity = true;                            ///< Has infinity value.
  static const bool has_quiet_NaN = true;                           ///< Has a quiet NaN value.
  static const bool has_signaling_NaN = true;                       ///< Has a signaling NaN value.
  static Float64(min)() throw() { return 2.2250738585072014e-308; } ///<  DBL_MIN
  static Float64(max)() throw() { return 1.7976931348623158e+308; } ///<  DBL_MAX
  static Float64 negative_max() throw() { return -1.7976931348623158e+308; } ///< -DBL_MAX
  static Float64 infinity() throw()
  { ///< Infinity.
    Float64_rep rep = { MI__FLOAT64_INF_REP };
    return rep.val;
  }
  static Float64 quiet_NaN() throw()
  { ///< Quiet NaN.
    Float64_rep rep = { MI__FLOAT64_QNAN_REP };
    return rep.val;
  }
  static Float64 signaling_NaN() throw()
  { ///< Signaling NaN.
    Float64_rep rep = { MI__FLOAT64_SNAN_REP };
    return rep.val;
  }
};

/*@}*/ // end group mi_base_number_traits_specialization

} // namespace base

/*@}*/ // end group mi_base_types

} // namespace mi

#endif // MI_BASE_TYPES_H
