/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/math/assert.h
/// \brief Assertions and compile-time assertions.
///
/// See \ref mi_math_assert.

#ifndef MI_MATH_ASSERT_H
#define MI_MATH_ASSERT_H

#include <mi/base/config.h>
#include <mi/base/assert.h>

namespace mi {

namespace math {

/** \defgroup mi_math_assert \MathApiName Assertions
    \ingroup mi_math

    Assertions.

    \par Include File:
    <tt> \#include <mi/math/assert.h></tt>

    \MathApiName supports quality software development with assertions. They are contained in
    various places in the %math API include files.

    These tests are mapped to corresponding %base API assertions by default, which in turn are
    switched off by default to have the performance of a release build. To activate the tests, you
    need to define the two macros #mi_math_assert and #mi_math_assert_msg, or correspondingly the
    #mi_base_assert and #mi_base_assert_msg macros, before including the relevant include files.
    Defining only one of the two macros is considered an error.

    See also \ref mi_math_intro_config and \ref mi_base_intro_config in
    %general.

    @{
*/

#if       defined( mi_math_assert) && ! defined( mi_math_assert_msg) \
     || ! defined( mi_math_assert) &&   defined( mi_math_assert_msg)
error "Only one of mi_math_assert and mi_math_assert_msg has been defined. Please define both."
#else
#ifndef mi_math_assert

/// Math API assertion macro (without message).
///
/// If \c expr evaluates to \c true this macro shall have no effect. If \c expr evaluates to \c
/// false this macro may print a diagnostic message and change the control flow of the program, such
/// as aborting the program or throwing an exception. But it may also have no effect at all, for
/// example if assertions are configured to be disabled.
///
/// By default, this macro maps to #mi_base_assert, which in turn does nothing by default. You can
/// (re-)define this macro to perform possible checks and diagnostics within the specification given
/// in the previous paragraph.
///
/// \see \ref mi_math_intro_assert and
///      \ref mi_base_intro_assert
#define mi_math_assert(expr) mi_base_assert(expr)

/// Math API assertion macro (with message).
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
/// By default, this macro maps to #mi_base_assert_msg, which in turn does nothing by default. You
/// can (re-)define this macro to perform possible checks and diagnostics within the specification
/// given in the previous paragraph.
///
/// \see \ref mi_math_intro_assert and
///      \ref mi_base_intro_assert
#define mi_math_assert_msg(expr, msg) mi_base_assert_msg(expr, msg)

// Just for doxygen, begin

/// Indicates whether assertions are actually enabled. This symbol gets defined if and only if you
/// (re-)defined #mi_math_assert and #mi_math_assert_msg (or #mi_base_assert and
/// #mi_base_assert_msg). Note that you can not simply test for #mi_math_assert or
/// #mi_math_assert_msg, since these macros get defined in any case (if you do not (re-)define them,
/// they evaluate to a dummy statement that has no effect).
#define mi_math_assert_enabled
#undef mi_math_assert_enabled

// Just for doxygen, end

#ifdef mi_base_assert_enabled
#define mi_math_assert_enabled
#endif // mi_math_assert_enabled

#else // mi_math_assert

#define mi_math_assert_enabled

#endif // mi_math_assert
#endif // mi_math_assert xor mi_math_assert_msg


/*@}*/ // end group mi_math_assert

} // namespace math

} // namespace mi

#endif // MI_MATH_ASSERT_H
