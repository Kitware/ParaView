/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for logging related settings.

#ifndef MI_NEURAYLIB_ILOGGING_CONFIGURATION_H
#define MI_NEURAYLIB_ILOGGING_CONFIGURATION_H

#include <mi/base/enums.h>
#include <mi/base/interface_declare.h>

namespace mi
{

namespace base
{
class ILogger;
}

namespace neuraylib
{

/** \addtogroup mi_neuray_configuration
@{
*/

/// Components of the log message prefix.
enum Log_prefix
{
  LOG_PREFIX_TIME = 0x0001,         ///< human-readable timestamp
  LOG_PREFIX_TIME_SECONDS = 0x0002, ///< timestamp in seconds with milliseconds resolution
  LOG_PREFIX_HOST_THREAD = 0x0004,  ///< ID of the host and thread that generate the log message
  LOG_PREFIX_HOST_NAME = 0x0008,    ///< name of the host that generates the log message
  LOG_PREFIX_MODULE = 0x0010,       ///< module that generates the log message
  LOG_PREFIX_CATEGORY = 0x0020,     ///< category of the log message
  LOG_PREFIX_SEVERITY = 0x0040,     ///< severity of the log message
  LOG_PREFIX_FORCE_32_BIT = 0xffffffffU
};

mi_static_assert(sizeof(Log_prefix) == sizeof(Uint32));

/// This interface is used for configuring the logging for the \neurayLibraryName.
///
/// The \neurayApiName allows to configure the logging in several ways. You can control the logging
/// by providing an own logger object that implements the #mi::base::ILogger interface. By default,
/// all messages are written to stderr.
///
/// There are two types of loggers, forwarding and receiving loggers. From an application point of
/// view the flow of a log message is as follows: The application sends the message to the
/// forwarding logger which forwards it to the \neurayLibraryName. The message is processed, and if
/// not discarded, sent to the logging host (if distinct from the local host), and passed to the
/// receiving logger. Both types of loggers use the same interface
/// #mi::base::ILogger. The receiving logger can be set by the application, the forwarding logger
/// cannot be controlled and is provided by the \neurayLibraryName.
///
/// The amount of messages can be controlled via log levels. There are two different kinds of log
/// levels, one overall log level, and log levels for individual message categories. These log
/// levels act as independent filters; only messages passing both log level settings are reported.
///
/// Note that setting a particular log level implies all higher log levels, e.g.,
/// #mi::base::MESSAGE_SEVERITY_WARNING includes #mi::base::MESSAGE_SEVERITY_ERROR and
/// #mi::base::MESSAGE_SEVERITY_FATAL.
///
/// In a cluster setup only one host, the logging host, receives the log messages of all hosts. The
/// selection of the logging host is controlled via log priorities (see #set_log_priority()).
/// Forwarding of log messages to the logging host can be disabled (see #set_log_locally()).
///
/// \see #mi::base::ILogger for a list of supported message categories
class ILogging_configuration : public mi::base::Interface_declare<0xaf42fbf7, 0xa7da, 0x4f35, 0xa7,
                                 0xcb, 0xbe, 0xb5, 0xcc, 0x11, 0x3d, 0x7c>
{
public:
  /// Sets the receiving logger.
  ///
  /// Note that in a cluster setup only the logging host will receive messages from the
  /// \neurayLibraryName. An exception are fatal log messages because due to kind of nature of
  /// these messages there is no guarantee that they will reach the logging host. In addition,
  /// very few log messages might be received during startup before the host has joined the
  /// cluster.
  ///
  /// \param logger      The receiving logger. It is valid to pass \c NULL in which case logging
  ///                    is reset to be done to stderr.
  virtual void set_receiving_logger(base::ILogger* logger) = 0;

  /// Returns the receiving logger.
  ///
  /// Note that if no receiving logger has been set, this method returns \c NULL and logging is
  /// done using a default logger that is internal to this library. This default logger simply
  /// writes to stderr.
  ///
  /// Note that log messages directly passed to the \em receiving logger will not be processed by
  /// the \neurayLibraryName. Use the \em forwarding logger instead.
  virtual base::ILogger* get_receiving_logger() const = 0;

  /// Returns the forwarding logger.
  ///
  /// The forwarding logger should be used to pass log messages to the \neurayLibraryName.
  virtual base::ILogger* get_forwarding_logger() const = 0;

  /// Sets the overall logging level of this host.
  ///
  /// Any messages below the given level will be discarded.
  ///
  /// \see set_log_level_by_category()
  ///
  /// \param level           The logging level.
  /// \return                0, in case of success, -1 in case of failure.
  virtual Sint32 set_log_level(base::Message_severity level) = 0;

  /// Returns the overall logging level of this host.
  ///
  /// \see get_log_level_by_category()
  ///
  /// \return                The configured logging level.
  virtual base::Message_severity get_log_level() const = 0;

  /// Sets the logging level of this host for a particular message category.
  ///
  /// Any messages of that category below the given level will be discarded.
  ///
  /// \see #mi::base::ILogger for supported categories
  /// \see set_log_level() for a filter level independent of categories
  ///
  /// \param category        The message category. The special value "ALL" is supported to set
  ///                        the logging level of all categories.
  /// \param level           The logging level.
  /// \return                0, in case of success, -1 in case of failure.
  virtual Sint32 set_log_level_by_category(const char* category, base::Message_severity level) = 0;

  /// Returns the logging level of this host for a particular message category.
  ///
  /// \see #mi::base::ILogger for supported categories
  /// \see get_log_level() for a filter level independent of categories
  ///
  /// \param category        The message category.
  /// \return                The logging level, in case of success, -1 in case of failure.
  virtual base::Message_severity get_log_level_by_category(const char* category) const = 0;

  /// Sets the logging priority of this host.
  ///
  /// The log priority determines which host becomes the logging host. The preferred logging host
  /// should have the highest number, and fallbacks in case of problems should have lower numbers,
  /// in the preferred order of fallbacks. Hosts with the highest logging priority will take over
  /// from designated logging hosts that have failed. The default logging priority is 0.
  ///
  /// This can only be configured before \NeurayProductName has been started.
  ///
  /// \param priority        The logging priority
  /// \return                0, in case of success, -1 in case of failure.
  virtual Sint32 set_log_priority(Sint32 priority) = 0;

  /// Returns the logging priority of this host.
  virtual Sint32 get_log_priority() const = 0;

  /// Sets the log message prefix.
  ///
  /// The log message prefix consists of several optional components that provide context for the
  /// log message. Each of these components can be enabled or disabled.
  ///
  /// \param prefix          A bitmask of the to-be-enabled log message components (see
  ///                        #mi::neuraylib::Log_prefix).
  virtual void set_log_prefix(Uint32 prefix) = 0;

  /// Returns the log message prefix.
  ///
  /// \return                A bitmask of the enabled log message components (see
  ///                        #mi::neuraylib::Log_prefix).
  virtual Uint32 get_log_prefix() const = 0;

  /// Enables or disables local logging.
  ///
  /// If local logging is enabled, all log messages from this host are sent to the local logger.
  /// Otherwise, they are sent to the logger of the logging host (see #set_log_priority()).
  ///
  /// This can only be configured before \NeurayProductName has been started.
  ///
  /// \param value           \c true to enable the local logging, \c false to disable it (the
  ///                        default).
  /// \return                0, in case of success, -1 in case of failure.
  virtual Sint32 set_log_locally(bool value) = 0;

  /// Indicates whether local logging is enabled or not.
  virtual bool get_log_locally() const = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ILOGGING_CONFIGURATION_H
