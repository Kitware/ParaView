/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/enums.h
/// \brief Basic enums.
///
/// See \ref mi_base_ilogger.

#ifndef MI_BASE_ENUMS_H
#define MI_BASE_ENUMS_H

#include <mi/base/assert.h>

namespace mi
{

namespace base
{

/** \addtogroup mi_base_ilogger
@{
*/

/// Constants for possible message severities.
///
/// \see #mi::base::ILogger::message()
///
enum Message_severity
{
  /// A fatal error has occurred.
  MESSAGE_SEVERITY_FATAL = 0,
  /// An error has occurred.
  MESSAGE_SEVERITY_ERROR = 1,
  /// A warning has occurred.
  MESSAGE_SEVERITY_WARNING = 2,
  /// This is a normal operational message.
  MESSAGE_SEVERITY_INFO = 3,
  /// This is a more verbose message.
  MESSAGE_SEVERITY_VERBOSE = 4,
  /// This is debug message.
  MESSAGE_SEVERITY_DEBUG = 5,
  //  Undocumented, for alignment only
  MESSAGE_SEVERITY_FORCE_32_BIT = 0xffffffffU
};

mi_static_assert(sizeof(Message_severity) == 4);

/*@}*/ // end group mi_base_ilogger

} // namespace base
} // namespace mi

#endif // MI_BASE_ENUMS_H
