/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/ilogger.h
/// \brief Logger interface class that supports message logging
///
/// See \ref mi_base_ilogger.

#ifndef MI_BASE_ILOGGER_H
#define MI_BASE_ILOGGER_H

#include <cstdarg>
#include <cstdio>
#include <ostream>
#include <sstream>
#include <string>

#include <mi/base/config.h>
#include <mi/base/enums.h>
#include <mi/base/handle.h>
#include <mi/base/iinterface.h>
#include <mi/base/interface_declare.h>

namespace mi {

namespace base {

/** \defgroup mi_base_ilogger Logging
    \ingroup mi_base

    Logger interface and severity levels.

       \par Include File:
       <tt> \#include <mi/base/ilogger.h></tt>

    @{
*/

namespace details {

/// Tags which help categorize log messages.
///
// Tags may be combined.
enum Message_tag
{
    TAG_NONE                    = 0u,      ///< no tags
    TAG_COMPATIBILITY           = 1u << 0, ///< hardware or library compatibility
    TAG_UNRECOVERABLE           = 1u << 1, ///< unrecoverable issue, e.g. fatal CUDA errors
    TAG_API_INPUT               = 1u << 2, ///< e.g. invalid value
    TAG_API_USAGE               = 1u << 3, ///< e.g. wrong order of operations
    TAG_VERSIONING              = 1u << 4, ///< e.g. library version info, mismatch
    TAG_SYSTEM_RESOURCE         = 1u << 5, ///< non-memory, e.g. device assignment, disk space, ...
    TAG_MEMORY                  = 1u << 6, ///< memory resource
    TAG_FILE                    = 1u << 7, ///< file not found, etc.
    TAG_STATS                   = 1u << 8  ///< e.g. timing, memory usage
};

} // namespace details

using namespace details;

/// Structured details to log messages.
///
/// These details may be used to react to certain types of events or for more accurate log filtering
/// without having to parse the log message text.
/// Note that logging code may not always provide a full set of details.
///
/// This type uses named constructors (which can be chained) to allow for clearer construction code.
struct Message_details
{
    enum { HOST_ID_LOCAL = 0 };
    enum {
        DEVICE_ID_CPU = -1,
        DEVICE_ID_UNKNOWN_CUDA = -2
    };

    /// The cluster ID of the host on which the message originated.
    ///
    /// Callers will not need to set this field; it is populated automatically
    /// by the logging module. Note that a value of \c HOST_ID_LOCAL may be replaced
    /// by the host's ID.
    Uint32              host_id;

    /// \c DEVICE_ID_XXX or a CUDA device ID.
    Sint32              device_id;

    /// A set of tags.
    ///
    /// \see Message_tag.
    Uint32              tags;

    /// An additional message identifier.
    Uint32              message_id;

    Message_details()
    : host_id(HOST_ID_LOCAL)
    , device_id(DEVICE_ID_CPU)
    , tags(TAG_NONE)
    , message_id(0u)
    {}

    Message_details(
            const Uint32 host,
            const Sint32 device,
            const Uint32 tag,
            const Uint32 id)
    : host_id(host)
    , device_id(device)
    , tags(tag)
    , message_id(id)
    {}

    /// Checks if this event pertains to a CUDA device or the CPU.
    bool is_device() const
    {
        return device_id != DEVICE_ID_CPU;
    }

    /// Checks if all \p required_tags are present.
    bool is_tagged(const Uint32 required_tags) const
    {
        return (this->tags & required_tags) == required_tags;
    }

    /// Named constructor.
    Message_details& host(const Uint32 id)
    {
        host_id = id;
        return *this;
    }

    /// Named constructor.
    Message_details& device(const Sint32 id)
    {
        device_id = id;
        return *this;
    }

    /// Named constructor.
    Message_details& tag(const Uint32 t)
    {
        tags = t;
        return *this;
    }

    /// Named constructor.
    Message_details& tag(const Uint32 t, const Uint32 code)
    {
        tags = t;
        message_id = code;
        return *this;
    }

    /// Named constructor.
    Message_details& code(const Uint32 c)
    {
        message_id = c;
        return *this;
    }
};


/// The %ILogger interface class supports logging of messages.
///
/// Plugins based on #mi::base::Plugin are provided with a logger, such that plugins can be written
/// to use the same message log as the host application.
///
/// Different APIs allow to register your own implementation of a logger to override their internal
/// implementation. Note that in such case the object implementing this interface shall not be
/// created on the stack, since this might lead to premature destruction of such instances while
/// still being in use by the API.
///
/// The following message categories are used in some APIs. There might be other, undocumented
/// categories.
///
/// <table>
///   <tr><th>category   </th><th>purpose</th></tr>
///   <tr><td>"DATABASE" </td><td>database</td></tr>
///   <tr><td>"DISK"     </td><td>raw disk I/O, swapping</td></tr>
///   <tr><td>"GEOMETRY" </td><td>geometry processing, e.g., tessellation</td></tr>
///   <tr><td>"IMAGE"    </td><td>texture processing, image and video plugins</td></tr>
///   <tr><td>"IO"       </td><td>scene data import and export</td></tr>
///   <tr><td>"MAIN"     </td><td>reserved for the application itself</td></tr>
///   <tr><td>"MISC"     </td><td>other</td></tr>
///   <tr><td>"MEMORY"   </td><td>memory management</td></tr>
///   <tr><td>"NETWORK"  </td><td>networking</td></tr>
///   <tr><td>"PLUGIN"   </td><td>plugins (unless other categories fit better)</td></tr>
///   <tr><td>"RENDER"   </td><td>rendering</td></tr>
/// </table>
class ILogger : public
    Interface_declare<0x4afbf19a,0x5fb7,0x4422,0xae,0x4b,0x25,0x13,0x06,0x2c,0x30,0x5f>
{
public:
    /// Emits a message to the application's log.
    ///
    /// The application can decide to output the message to any channel or to drop it.
    ///
    /// This function can be called at any time from any thread, including concurrent calls from
    /// several threads at the same time.
    ///
    /// \note Severity #mi::base::details::MESSAGE_SEVERITY_FATAL indicates that the caller is unable to
    ///       recover from the error condition. Therefore, the process will be terminated after a
    ///       fatal log message has been delivered. To avoid the process termination logger
    ///       implementations might choose not to return from this method for fatal log messages.
    ///       This severity should only be used in exceptional cases.
    ///
    /// \param level             The log level which specifies the severity of the message.
    /// \param module_category   The module and the category which specify the origin and the
    ///                          functional area of this message. The format of string parameter
    ///                          is "module:category". Both names are optional. The module name must
    ///                          not contain any colons. See above for valid category names.
    /// \param message           The log message.
    virtual void message(
        Message_severity level, const char* module_category, const char* message)
#ifdef MI_NEURAYLIB_DEPRECATED_LOG
        = 0;
#else
    {
        this->message(level,module_category,Message_details(),message);
    }
#endif

    /// Emits a message to the application's log.
    ///
    /// This overload receives additional details which applications may use to react to
    /// certain events.
    virtual void message(
            Message_severity level,
            const char* module_category,
            const Message_details&,
            const char* message)
#ifdef MI_NEURAYLIB_DEPRECATED_LOG
    {
        this->message(level,module_category,message);
    }
#else
        = 0;
#endif
    
    /// Emits a message to the application's log.
    ///
    /// The application can decide to output the message to any channel or to drop it.
    ///
    /// This function can be called at any time from any thread, including concurrent calls from
    /// several threads at the same time.
    ///
    /// \note Severity #mi::base::details::MESSAGE_SEVERITY_FATAL indicates that the caller is unable to
    ///       recover from the error condition. Therefore, the process will be terminated after a
    ///       fatal log message has been delivered. To avoid the process termination logger
    ///       implementations might choose not to return from this method for fatal log messages.
    ///       This severity should only be used in exceptional cases.
    ///
    /// \param level             The log level which specifies the severity of the message.
    /// \param module_category   The module and the category which specify the origin and the
    ///                          functional area of this message. The format of string parameter
    ///                          is "module:category". Both names are optional. The module name must
    ///                          not contain any colons. See above for valid category names.
    /// \param message           The log message using printf()-like format specifiers, followed
    ///                          by matching arguments. The formatted message is limited to 1023
    ///                          characters.
    inline void printf(
        Message_severity level, const char* module_category, const char* message, ...)
#ifdef MI_COMPILER_GCC
        __attribute__((format(printf, 4, 5)))
#endif
    {
        va_list args;
        va_start( args, message);
        char buffer[1024];
#ifdef MI_COMPILER_MSC
        vsnprintf_s( &buffer[0], sizeof( buffer), sizeof( buffer)-1, message, args);
#else
        vsnprintf( buffer, sizeof( buffer), message, args);
#endif
        this->message( level, module_category, Message_details(), buffer);
        va_end( args);
    }

    /// Emits a message to the application's log.
    ///
    /// This overload receives additional details which applications may use to react to
    /// certain events.
    inline void printf(
        Message_severity level, const char* module_category, const Message_details& details, const char* message, ...)
#ifdef MI_COMPILER_GCC
        __attribute__((format(printf, 5, 6)))
#endif
    {
        va_list args;
        va_start( args, message);
        char buffer[1024];
#ifdef MI_COMPILER_MSC
        vsnprintf_s( buffer, sizeof( buffer), sizeof( buffer)-1, message, args);
#else
        vsnprintf( buffer, sizeof( buffer), message, args);
#endif
        this->message( level, module_category, details, buffer);
        va_end( args);
    }
};

// A specialization of std::stringbuf to be used together with #mi::base::Log_stream.
//
// Its sync() method is overridden to send the contents of the string buffer to the logger, and an
// additional method #set_log_level() allows to specify the log level of the next message.
class Log_streambuf : public std::stringbuf
{
public:
    // Constructor.
    //
    // \param stream             The stream used with this string buffer. Used to flush the stream
    //                           if the log level is changed.
    // \param logger             The logger that finally receives the contents of this string
    //                           buffer.
    // \param module_category    The module and the category which specify the origin and the
    //                           functional area of this message. See #mi::base::ILogger::message()
    //                           for details.
    // \param default_level      The default log level. Used if no other log level is selected by
    //                           one of the manipulators.
    // \param default_details    The default message details. Used if no other details are set.
    Log_streambuf(
        std::ostream& stream,
        ILogger* logger,
        const std::string& module_category,
        Message_severity default_level = MESSAGE_SEVERITY_INFO,
        const Message_details& default_details = Message_details())
      : std::stringbuf( std::ios::out),
        m_stream( stream),
        m_default_level( default_level),
        m_default_details( default_details),
        m_logger( logger, DUP_INTERFACE),
        m_module_category( module_category),
        m_details( default_details)
    {
        set_log_level( m_default_level);
    }

    // Destructor.
    ~Log_streambuf() throw()
    {
    }
    
    // Flushes the string buffer if not empty, and sets the log level of the next message to the
    // given log level.
    void set_log_level( Message_severity level)
    {
        m_stream.flush();
        m_level = level;
    }

    // Flushes the string buffer if not empty, and sets the log level of the next message to the
    // given log level.
    void set_details( const Message_details& details)
    {
        m_stream.flush();
        m_details = details;
    }

protected:

    // Sends the contents of the string buffer to the logger, clears the string buffer, and resets
    // the log level and details to their defaults.
    int sync()
    {
        std::stringbuf::sync();
        const std::string& s = str();
        if( !s.empty()) {
            m_logger->message( m_level, m_module_category.c_str(), m_details, s.c_str());
            str( "");
            m_level = m_default_level;
            m_details = m_default_details;
        }
        return 0;
    }

private:

    std::ostream& m_stream;
    Message_severity m_default_level;
    Message_details m_default_details;
    mi::base::Handle<ILogger> m_logger;
    std::string m_module_category;
    Message_severity m_level;
    Message_details m_details;
};

/// Adapts #mi::base::ILogger to a standard streaming interface.
///
/// Messages are forwarded to the logger whenever the stream is flushed. The log level for the next
/// message can be changed by using one of the manipulators #fatal, #error, #warning, #info,
/// #verbose, or #debug, which correspond to the values of #mi::base::details::Message_severity.
/// The log details can be changed by pushing an instance of #mi::base::details;:Message_details
/// into the stream. Changing the log level or the details also triggers flushing.
///
/// Example:
/// \code
/// mi::base::Handle<mi::base::Logger> logger( ...);
/// mi::base::Log_stream stream( logger.get(), "APP:MAIN");
/// stream << "An info message" << std::flush;
/// stream << error << "And an error message" << std::flush;
/// stream << "And another info message" << std::flush;
/// \endcode
class Log_stream : public std::ostream
{
public:
    /// Constructor.
    ///
    /// \param logger             The logger object used by this stream.
    /// \param module_category    The module and the category which specify the origin and the
    ///                           functional area of this message. See #mi::base::ILogger::message()
    ///                           for details.
    /// \param default_level      The default log level. Used if no other log level is selected by
    ///                           one of the manipulators.
    /// \param default_details    The default message details.
    Log_stream(
        ILogger* logger,
        const char* module_category,
        Message_severity default_level = MESSAGE_SEVERITY_INFO,
        const Message_details& default_details = Message_details())
      : std::ostream( 0),
        m_buffer( *this, logger, module_category ? module_category : "APP:MAIN",
                default_level, default_details)
    {
        rdbuf( &m_buffer);
#if (__cplusplus >= 201402L)
        this->pword( get_index()) = this;
#endif
    }

    /// Constructor.
    ///
    /// \param logger             The logger object used by this stream.
    /// \param module_category    The module and the category which specify the origin and the
    ///                           functional area of this message. See #mi::base::ILogger::message()
    ///                           for details.
    /// \param default_level      The default log level. Used if no other log level is selected by
    ///                           one of the manipulators.
    /// \param default_details    The default message details.
    Log_stream(
        ILogger* logger,
        const std::string& module_category,
        Message_severity default_level = MESSAGE_SEVERITY_INFO,
        const Message_details& default_details = Message_details())
      : std::ostream( 0),
        m_buffer( *this, logger, module_category, default_level, default_details)
    {
        rdbuf( &m_buffer);
#if (__cplusplus >= 201402L)
        this->pword( get_index()) = this;
#endif
    }

    /// Destructor.
    ///
    /// Flushes the buffer.
    ~Log_stream() throw()
    {
        flush();
    }

    /// Flushes the buffer if not empty, and sets the log level of the next message to the given
    /// log level.
    void set_log_level( Message_severity level) { m_buffer.set_log_level( level); }
    
    /// Flushes the buffer if not empty, and sets the message details of the next message.
    void set_details( const Message_details& details) { m_buffer.set_details( details); }

#if (__cplusplus >= 201402L)
    //  Returns the unique index into the private storage of std::ios_base.
    static int get_index()
    {
        // Static initialization is guaranteed to be thread-safe with C++11 and later. The method
        // std::ios_base::xalloc() is guaranteed to be thread-safe with C++14 and later.
        static int s_index = std::ios_base::xalloc();
        return s_index;
    }
#endif

private:
    Log_streambuf m_buffer;
};

/// Manipulator for #mi::base::Log_stream.
///
/// Flushes the buffer if not empty, and sets the log level of the next message to
/// #mi::base::details::MESSAGE_SEVERITY_FATAL.
///
/// \note If you are using a C++ standard older than C++14, make sure to use the manipulator only if
/// the stream is an instance of #mi::base::Log_stream, otherwise your program will crash. With
/// C++14 and later, these manipulators will be ignored if the stream is not an instance of
/// #mi::base::Log_stream.
template <typename C, typename T>
std::basic_ostream<C, T>& fatal( std::basic_ostream<C, T>& ostream)
{
#if (__cplusplus >= 201402L)
    if( ostream.pword( Log_stream::get_index()) == &ostream)
#endif
        static_cast<Log_stream&>( ostream).set_log_level( base::MESSAGE_SEVERITY_FATAL);
    return ostream;
}

/// Manipulator for #mi::base::Log_stream.
///
/// Flushes the buffer if not empty, and sets the log level of the next message to
/// #mi::base::details::MESSAGE_SEVERITY_ERROR.
///
/// \note If you are using a C++ standard older than C++14, make sure to use the manipulator only if
/// the stream is an instance of #mi::base::Log_stream, otherwise your program will crash. With
/// C++14 and later, these manipulators will be ignored if the stream is not an instance of
/// #mi::base::Log_stream.
template <typename C, typename T>
std::basic_ostream<C, T>& error( std::basic_ostream<C, T>& ostream)
{
#if (__cplusplus >= 201402L)
    if( ostream.pword( Log_stream::get_index()) == &ostream)
#endif
        static_cast<Log_stream&>( ostream).set_log_level( base::MESSAGE_SEVERITY_ERROR);
    return ostream;
}

/// Manipulator for #mi::base::Log_stream.
///
/// Flushes the buffer if not empty, and sets the log level of the next message to
/// #mi::base::details::MESSAGE_SEVERITY_WARNING.
///
/// \note If you are using a C++ standard older than C++14, make sure to use the manipulator only if
/// the stream is an instance of #mi::base::Log_stream, otherwise your program will crash. With
/// C++14 and later, these manipulators will be ignored if the stream is not an instance of
/// #mi::base::Log_stream.
template <typename C, typename T>
std::basic_ostream<C, T>& warning( std::basic_ostream<C, T>& ostream)
{
#if (__cplusplus >= 201402L)
    if( ostream.pword( Log_stream::get_index()) == &ostream)
#endif
        static_cast<Log_stream&>( ostream).set_log_level( base::MESSAGE_SEVERITY_WARNING);
    return ostream;
}

/// Manipulator for #mi::base::Log_stream.
///
/// Flushes the buffer if not empty, and sets the log level of the next message to
/// #mi::base::details::MESSAGE_SEVERITY_INFO.
///
/// \note If you are using a C++ standard older than C++14, make sure to use the manipulator only if
/// the stream is an instance of #mi::base::Log_stream, otherwise your program will crash. With
/// C++14 and later, these manipulators will be ignored if the stream is not an instance of
/// #mi::base::Log_stream.
template <typename C, typename T>
std::basic_ostream<C, T>& info( std::basic_ostream<C, T>& ostream)
{
#if (__cplusplus >= 201402L)
    if( ostream.pword( Log_stream::get_index()) == &ostream)
#endif
        static_cast<Log_stream&>( ostream).set_log_level( base::MESSAGE_SEVERITY_INFO);
    return ostream;
}

/// Manipulator for #mi::base::Log_stream.
///
/// Flushes the buffer if not empty, and sets the log level of the next message to
/// #mi::base::details::MESSAGE_SEVERITY_VERBOSE.
///
/// \note If you are using a C++ standard older than C++14, make sure to use the manipulator only if
/// the stream is an instance of #mi::base::Log_stream, otherwise your program will crash. With
/// C++14 and later, these manipulators will be ignored if the stream is not an instance of
/// #mi::base::Log_stream.
template <typename C, typename T>
std::basic_ostream<C, T>& verbose( std::basic_ostream<C, T>& ostream)
{
#if (__cplusplus >= 201402L)
    if( ostream.pword( Log_stream::get_index()) == &ostream)
#endif
        static_cast<Log_stream&>( ostream).set_log_level( base::MESSAGE_SEVERITY_VERBOSE);
    return ostream;
}

/// Manipulator for #mi::base::Log_stream.
///
/// Flushes the buffer if not empty, and sets the log level of the next message to
/// #mi::base::details::MESSAGE_SEVERITY_DEBUG.
///
/// \note If you are using a C++ standard older than C++14, make sure to use the manipulator only if
/// the stream is an instance of #mi::base::Log_stream, otherwise your program will crash. With
/// C++14 and later, these manipulators will be ignored if the stream is not an instance of
/// #mi::base::Log_stream.
template <typename C, typename T>
std::basic_ostream<C, T>& debug( std::basic_ostream<C, T>& ostream)
{
#if (__cplusplus >= 201402L)
    if( ostream.pword( Log_stream::get_index()) == &ostream)
#endif
        static_cast<Log_stream&>( ostream).set_log_level( base::MESSAGE_SEVERITY_DEBUG);
    return ostream;
}

/// Manipulator for #mi::base::Log_stream.
///
/// Flushes the buffer if not empty, and sets the log details of the next message.
///
/// \note If you are using a C++ standard older than C++14, make sure to use the manipulator only if
/// the stream is an instance of #mi::base::Log_stream, otherwise your program will crash. With
/// C++14 and later, these manipulators will be ignored if the stream is not an instance of
/// #mi::base::Log_stream.
template <typename C, typename T>
std::basic_ostream<C, T>& operator<<( std::basic_ostream<C, T>& ostream, const Message_details& md)
{
#if (__cplusplus >= 201402L)
    if( ostream.pword( Log_stream::get_index()) == &ostream)
#endif
        static_cast<Log_stream&>( ostream).set_details( md);
    return ostream;
}


namespace msg {

using namespace details;
typedef Message_tag Tag;
typedef Message_details Details;

inline Details tag_details(const Uint32 tags)
{
    return Details().tag(tags);
}

inline Details device_details(
        const Sint32 device=Details::DEVICE_ID_UNKNOWN_CUDA,
        const Uint32 tags=TAG_NONE)
{
    return Details().tag(tags).device(device);
}

}

/*@}*/ // end group mi_base_ilogger

} // namespace base

} // namespace mi

#endif // MI_BASE_ILOGGER_H
