/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Assertions and compile-time assertions.
///
/// See \ref mi_neuray_assert.

#ifndef MI_NEURAYLIB_ASSERT_H
#define MI_NEURAYLIB_ASSERT_H

#include <mi/base/assert.h>
#include <mi/base/config.h>

namespace mi
{

namespace neuraylib
{

// --------------------------------------------------------------------
//
//  Assertions
//
// --------------------------------------------------------------------
/** \defgroup mi_neuray_assert \NeurayApiName Assertions
    \ingroup mi_neuray
    \brief Assertions.

       \par Include File:
       <tt> \#include <mi/neuraylib/assert.h></tt>

    \NeurayApiName supports quality software development with assertions.
    They are contained in various places in the \neurayApiName include files.

    These tests are mapped to corresponding %base API assertions by
    default, which in turn are switched off by default to have the
    performance of a release build. To activate the tests, you need to
    define the two macros #mi_neuray_assert and #mi_neuray_assert_msg,
    or correspondingly the #mi_base_assert and #mi_base_assert_msg
    macros, before including the relevant include files. Defining only
    one of the two macros is considered an error.

    See also \ref mi_math_intro_config and \ref mi_base_intro_config in general.
    @{
 */

#if defined(mi_neuray_assert) && !defined(mi_neuray_assert_msg) ||                                 \
  !defined(mi_neuray_assert) && defined(mi_neuray_assert_msg)
error "Only one of mi_neuray_assert and mi_neuray_assert_msg has been defined. Please define both."
#else
#ifndef mi_neuray_assert

/// If \c expr evaluates to \c true this macro shall have no effect.
/// If \c expr evaluates to \c false this macro may print a diagnostic
/// message and change the control flow of the program, such as aborting
/// the program or throwing an exception. But it may also have no
/// effect at all, for example if assertions are configured to
/// be disabled.
///
/// By default, this macro maps to #mi_base_assert, which in turn
/// does nothing by default. You can (re-)define this macro
/// to perform possible checks and diagnostics within the specification
/// given in the previous paragraph.
///
/// \see \ref mi_math_intro_config, \ref mi_base_intro_config

#define mi_neuray_assert(expr) mi_base_assert(expr)

/// If \c expr evaluates to \c true this macro shall have no effect.
/// If \c expr evaluates to \c false this macro may print a diagnostic
/// message and change the control flow of the program, such as aborting
/// the program or throwing an exception. But it may also have no
/// effect at all, for example if assertions are configured to
/// be disabled.
///
/// the \c msg text string contains additional diagnostic information
/// that may be shown with a diagnostic message. Typical usages would
/// contain \c "precondition" or  \c "postcondition" as clarifying
/// context information in the \c msg parameter.
///
/// By default, this macro maps to #mi_base_assert_msg, which in turn
/// does nothing by default. You can (re-)define this macro
/// to perform possible checks and diagnostics within the specification
/// given in the previous paragraph.
///
/// \see \ref mi_math_intro_config, \ref mi_base_intro_config
#define mi_neuray_assert_msg(expr, msg) mi_base_assert_msg(expr, msg)

#endif // mi_neuray_assert
#endif // mi_neuray_assert xor mi_neuray_assert_msg

/*@}*/ // end group mi_neuray_assert

} // namespace neuraylib
} // namespace mi

#endif // MI_NEURAYLIB_ASSERT_H
