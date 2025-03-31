/***************************************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/enums.h
/// \brief Basic enums.
///
/// See \ref mi_base_ilogger.

#ifndef MI_BASE_ENUMS_H
#define MI_BASE_ENUMS_H

#include <mi/base/assert.h>
#include <mi/base/types.h>
#include <mi/base/version.h> // for MI_BASE_DEPRECATED_ENUM_VALUE

namespace mi {

namespace base {

/// Namespace for details of the Base API.
/// \ingroup mi_base
namespace details {

/** \addtogroup mi_base_ilogger
@{
*/

/// Constants for possible message severities.
///
/// \see #mi::base::ILogger::message()
///
enum Message_severity : Uint32
{
    /// A fatal error has occurred.
    MESSAGE_SEVERITY_FATAL   = 0,
    /// An error has occurred.
    MESSAGE_SEVERITY_ERROR   = 1,
    /// A warning has occurred.
    MESSAGE_SEVERITY_WARNING = 2,
    /// This is a normal operational message.
    MESSAGE_SEVERITY_INFO    = 3,
    /// This is a more verbose message.
    MESSAGE_SEVERITY_VERBOSE = 4,
    /// This is debug message.
    MESSAGE_SEVERITY_DEBUG   = 5
#ifndef SWIG
    MI_BASE_DEPRECATED_ENUM_VALUE(MESSAGE_SEVERITY_FORCE_32_BIT, 0xffffffffU)
#endif
};

/*@}*/ // end group mi_base_ilogger

}

using namespace details;

} // namespace base

} // namespace mi

#endif // MI_BASE_ENUMS_H
