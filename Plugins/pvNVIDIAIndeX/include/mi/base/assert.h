/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/assert.h
/// \brief Assertions and compile-time assertions.
///
/// See \ref mi_base_assert.

#ifndef MI_BASE_ASSERT_H
#define MI_BASE_ASSERT_H

#include <mi/base/config.h>

namespace mi {

namespace base {

/** \defgroup mi_base_assert \BaseApiName Assertions
    \ingroup mi_base
    \brief Assertions and static assertions.

       \par Include File:
       <tt> \#include <mi/base/assert.h></tt>

    The \BaseApiName supports quality software development with assertions. They are contained in
    various places in the \BaseApiName include files.

    These tests are switched off by default to have the performance of a release build. To activate
    the tests, you need to define the two macros #mi_base_assert and #mi_base_assert_msg before
    including the relevant include files. Defining only one of the two macros is considered an
    error.

    See also \ref mi_base_intro_config in %general.

    @{
 */

/// \def mi_static_assert(expr)
///
/// Compile time assertion that raises a compilation error if the constant expression \p expr
/// evaluates to \c false.
///
/// Example usage: <tt> mi_static_assert(sizeof(char) == 1);</tt>
///
/// This compile-time assertion can be used inside as well as outside of functions.
///
/// If this assertion fails the compiler will complain about applying the \c sizeof operator to an
/// undefined or incomplete type on the line of the assertion failure.
///
/// You may define the macro #mi_static_assert(expr) yourself to customize its behavior, for
/// example, to disable it.
#ifndef mi_static_assert
#ifdef _MSC_VER
// Special case for Visual Studio 7.1, since __LINE__ would not work.
#define mi_static_assert(expr)                                               \
    typedef mi::base::static_assert_test<static_cast<int>(                   \
        sizeof(mi::base::static_assert_failure<static_cast<bool>((expr))>))> \
            MI_BASE_JOIN(static_assert_instance, __COUNTER__)
#else // _MSC_VER
#ifdef MI_COMPILER_GCC
#define mi_static_assert_attribute __attribute__((unused))
#else
#define mi_static_assert_attribute
#endif
#define mi_static_assert(expr)                                               \
    typedef mi::base::static_assert_test<static_cast<int>(                   \
        sizeof(mi::base::static_assert_failure<static_cast<bool>((expr))>))> \
            MI_BASE_JOIN(static_assert_instance, __LINE__) mi_static_assert_attribute
#endif // _MSC_VER
#endif // mi_static_assert

// helper definitions for the mi_static_assert above.
template <bool> struct static_assert_failure;
template <>     struct static_assert_failure<true> {};
template <int>  struct static_assert_test {};

#if       defined( mi_base_assert) && ! defined( mi_base_assert_msg) \
     || ! defined( mi_base_assert) &&   defined( mi_base_assert_msg)
error "Only one of mi_base_assert and mi_base_assert_msg has been defined. Please define both."
#else
#ifndef mi_base_assert

/// Base API assertion macro (without message).
///
/// If \c expr evaluates to \c true this macro shall have no effect. If \c expr evaluates to \c
/// false this macro may print a diagnostic message and change the control flow of the program, such
/// as aborting the program or throwing an exception. But it may also have no effect at all, for
/// example if assertions are configured to be disabled.
///
/// By default, this macro does nothing. You can (re-)define this macro to perform possible checks
/// and diagnostics within the specification given in the previous paragraph.
///
/// \see \ref mi_base_intro_assert
#define mi_base_assert(expr) (static_cast<void>(0)) // valid but void null stmt

/// Base API assertion macro (with message).
///
/// If \c expr evaluates to \c true this macro shall have no effect. If \c expr evaluates to \c
/// false this macro may print a diagnostic message and change the control flow of the program, such
/// as aborting the program or throwing an exception. But it may also have no effect at all, for
/// example if assertions are configured to be disabled.
///
/// The \c msg text string contains additional diagnostic information that may be shown with a
/// diagnostic message. Typical usages would contain \c "precondition" or \c "postcondition" as
/// clarifying context information in the \c msg parameter.
///
/// By default, this macro does nothing. You can (re-)define this macro to perform possible checks
/// and diagnostics within the specification given in the previous paragraph.
///
/// \see \ref mi_base_intro_assert
#define mi_base_assert_msg(expr, msg) (static_cast<void>(0)) // valid but void null stmt

// Just for doxygen, begin

/// Indicates whether assertions are actually enabled. This symbol gets defined if and only if you
/// (re-)defined #mi_base_assert and #mi_base_assert_msg. Note that you can not simply test for
/// #mi_base_assert or #mi_base_assert_msg, since these macros get defined in any case (if you do
/// not (re-)define them, they evaluate to a dummy statement that has no effect).
#define mi_base_assert_enabled
#undef mi_base_assert_enabled

// Just for doxygen, end

#else // mi_base_assert

#define mi_base_assert_enabled

#endif // mi_base_assert
#endif // mi_base_assert xor mi_base_assert_msg

/// \def MI_BASE_ASSERT_FUNCTION
///
/// Expands to a string constant that describes the function in which the macro has been expanded.
///
/// This macro can be used as diagnostic in addition to the standard \c __LINE__ and \c __FILE__
/// values. For compilers that do not support such function name diagnostic the string \c "unknown"
/// will be used.
///
/// If possible, lets the asserts support function names in their message.
#if defined(__FUNCSIG__)
#  define MI_BASE_ASSERT_FUNCTION __FUNCSIG__
#elif defined( __cplusplus) && defined(__GNUC__) && defined(__GNUC_MINOR__) \
        && ((__GNUC__ << 16) + __GNUC_MINOR__ >= (2 << 16) + 6)
#  define MI_BASE_ASSERT_FUNCTION    __PRETTY_FUNCTION__
#else
#  if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#    define MI_BASE_ASSERT_FUNCTION    __func__
#  else
#    define MI_BASE_ASSERT_FUNCTION    ("unknown")
#  endif
#endif

/*@}*/ // end group mi_base_assert

} // namespace base

} // namespace mi

#endif // MI_BASE_ASSERT_H
