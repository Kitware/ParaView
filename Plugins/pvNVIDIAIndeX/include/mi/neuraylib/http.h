/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief A lightweight HTTP server.

#ifndef MI_NEURAYLIB_HTTP_H
#define MI_NEURAYLIB_HTTP_H

#include <cstdarg>
#include <cstdio>

#include <mi/base/interface_declare.h>
#include <mi/neuraylib/idata.h>

namespace mi
{

namespace http
{
class IConnection;
class IWeb_socket;
}
namespace neuraylib
{
class IBuffer;
}

/** \defgroup mi_neuray_http HTTP server
    \ingroup mi_neuray

    The HTTP server module implements a lightweight HTTP server. The HTTP server itself is only a
    framework for handler classes. It will accept incoming connections and extract various
    parameters of the request, like URL, query parameters, headers etc. Then it will call all
    request handlers in sequence until one of the handlers signals that the request was completely
    handled by the handler.

    The server is multithreaded and will start one thread for each connection.

    Any number of request handler classes may be installed to the HTTP server. Each of those handler
    classes has to implement a handle() function. This function gets the requesting connection. From
    the connection the handle() function may get the various parameters of the request.

    Each handler may alter the various request and response parameters. This is useful for
    preprocessing the request for subsequent handlers. In that case it can return \c false which
    will pass the request to subsequent handlers. Note that if a handler changes the URL of a
    request and then returns \c false then the iteration over the handlers will start again from the
    first handler.

    If a request handler is responsible for handling a request it may first alter the response and
    then use the IConnection::print() or IConnection::enqueue() function to output data. Then it has
    to return \c true to indicate that no subsequent request handlers have to be called.

    In addition to the request handlers, response handlers may be installed to the server. A
    response handler is called just before the response is sent out to the client. It may alter the
    response header fields and it may cause side effects (like installing data handlers). Each
    request or response handler may install a data handler to the connection. All payload data sent
    to the client will be passed through all data handlers installed to the connection. The data
    handlers may or may not alter the data passed through it. This may be used to do things like SSI
    (server-side include) expansion, server side script execution, implementing logging etc.

    To create a server the #mi::http::IFactory needs to be obtained from the \neurayApiName using
    #mi::neuraylib::INeuray::get_api_component(). The factory can then be used to create as many
    HTTP servers as needed and also to create built-in request and response handlers.
*/

/// Namespace for the HTTP server of the \neurayApiName.
/// \ingroup mi_neuray
namespace http
{

/** \addtogroup mi_neuray_http
@{
*/

/// This interface holds all the parameters of a request.
///
/// \note
///   A pointer to an #mi::http::IRequest interface is only valid as long as the
///   #mi::http::IConnection interface from which it was obtained is valid. The same is true for all
///   strings obtained from the request. Overwriting such a string using the supplied functions will
///   invalidate the obtained string.
///
/// \note
///   This interface is not derived from #mi::base::IInterface. As one consequence, there are no
///   retain() and release() methods, and the interface cannot be used together with the
///   #mi::base::Handle class.
class IRequest
{
public:
  /// Returns the request line of the request.
  virtual const char* get_request_line() const = 0;

  /// Returns the command of the request.
  virtual const char* get_command() const = 0;

  /// Returns the protocol of the request.
  virtual const char* get_protocol() const = 0;

  /// Sets the URL of the request.
  ///
  /// \param url     The new URL to be set.
  virtual void set_url(const char* url) = 0;

  /// Returns the URL of the request.
  virtual const char* get_url() const = 0;

  /// Changes an existing header of the request or adds a new header to the request.
  ///
  /// \param key     The key of the header.
  /// \param value   The value of the header.
  virtual void set_header(const char* key, const char* value) = 0;

  /// Removes a header from the request.
  ///
  /// \param key     The key of the header.
  virtual void remove_header(const char* key) = 0;

  /// Returns a header from the request.
  ///
  /// \param key     The key of the header.
  /// \return        The value of the header, or \c NULL if the header was not found.
  virtual const char* get_header(const char* key) const = 0;

  /// Returns the header with the given index.
  ///
  /// Can be used for iterating over all headers by starting with \p index equal to 0 and then
  /// incrementing \p index until \c false is returned. When \c false is returned, \p key_pointer
  /// and \p value_pointer will not be written.
  ///
  /// \param index                The index of the header.
  /// \param[out] key_pointer     A pointer to the header's key.
  /// \param[out] value_pointer   A pointer to the header's value.
  /// \return                     \c true, if there is a header for given index, \c false
  ///                             otherwise.
  virtual bool get_header(
    Size index, const char** key_pointer, const char** value_pointer) const = 0;

  /// Changes an argument of the request or adds an argument to the request.
  ///
  /// If one or more arguments with the same key are present the first argument with this key will
  /// be changed.
  ///
  /// \param key     The key of the argument.
  /// \param value   The value of the argument.
  virtual void set_argument(const char* key, const char* value) = 0;

  /// Adds an argument to the request.
  ///
  /// This method does not overwrite an argument with the same key but always adds a new one.
  ///
  /// \param key     The key of the argument.
  /// \param value   The value of the argument.
  virtual void add_argument(const char* key, const char* value) = 0;

  /// Removes an argument from the request.
  ///
  /// This method removes all arguments with the given key.
  ///
  /// \param key The key of the argument.
  virtual void remove_argument(const char* key) = 0;

  /// Returns an argument from the request.
  ///
  /// Note that multiple arguments with the same key can occur in a request and this method will
  /// only return the first of them. Consider using
  /// \c get_argument(Size,const char**, const char**) if all arguments matching a particular key
  /// are desired.
  ///
  /// \param key     The key of the argument.
  virtual const char* get_argument(const char* key) const = 0;

  /// Returns the argument with the given index.
  ///
  /// Can be used for iterating over all arguments by starting with \p index equal to 0 and then
  /// incrementing until \c false is returned. When \c false is returned, \p key_pointer
  /// and \p value_pointer will not be written.
  ///
  /// \param index                The index of the argument.
  /// \param[out] key_pointer     A pointer to the argument's key.
  /// \param[out] value_pointer   A pointer to the argument's value.
  /// \return                     \c true, if there is a header for given index, \c false
  ///                             otherwise.
  virtual bool get_argument(
    Size index, const char** key_pointer, const char** value_pointer) const = 0;

  /// Returns the body string.
  ///
  /// \return The body, or \c NULL if there is no body.
  virtual const char* get_body() const = 0;
};

/// This interface holds all the parameters of a response.
///
/// \note
///   A pointer to an #mi::http::IResponse interface is only valid as long as the
///   #mi::http::IConnection interface from which it was obtained is valid. The same is true for all
///   strings obtained from the response. Overwriting such a string using the supplied functions
///   will invalidate the obtained string.
///
/// \note
///   This interface is not derived from #mi::base::IInterface. As one consequence, there are no
///   retain() and release() methods, and the interface cannot be used together with the
///   #mi::base::Handle class.
class IResponse
{
public:
  /// Sets the result code of the response.
  ///
  /// \param code    The new result code.
  /// \param message The corresponding string for \p code.
  virtual void set_result_code(Sint32 code, const char* message) = 0;

  /// Returns the result code of the response.
  virtual Sint32 get_result_code() const = 0;

  /// Changes a header of the response or adds a header to the response.
  ///
  /// \param key     The key of the header.
  /// \param value   The value of the header.
  virtual void set_header(const char* key, const char* value) = 0;

  /// Removes a header from the response.
  ///
  /// \param key     The key of the header.
  virtual void remove_header(const char* key) = 0;

  /// Returns a header from the response.
  ///
  /// \param key     The key of the header.
  /// \return        The value of the header, or \c NULL if the header was not found.
  virtual const char* get_header(const char* key) const = 0;

  /// Returns the header with the given index.
  ///
  /// Can be used for iterating over all headers by starting with \p index equal to 0 and then
  /// incrementing \p index until \c false is returned. When \c false is returned, \p key_pointer
  /// and \p value_pointer will not be written.
  ///
  /// \param index                The index of the header.
  /// \param[out] key_pointer     A pointer to the header's key.
  /// \param[out] value_pointer   A pointer to the header's value.
  /// \return                     \c true, if there is a header for given index, \c false
  ///                             otherwise.
  virtual bool get_header(
    Size index, const char** key_pointer, const char** value_pointer) const = 0;

  /// Indicates whether the response was already sent.
  virtual bool was_sent() const = 0;
};

/// A data handler may be added to a connection.
///
/// It is called every time data is enqueued or printed to the connection. The buffer is passed
/// through all installed data handlers which might replace the buffer by a different buffer. If a
/// data handler returns \c NULL no data is printed or enqueued to the connection and the handler
/// is uninstalled and released.
class IData_handler : public mi::base::Interface_declare<0x723054d8, 0xdfa7, 0x4475, 0xbc, 0xb4,
                        0x44, 0x23, 0x25, 0xea, 0x52, 0x50>
{
public:
  /// Handles data.
  ///
  /// The method might change the buffer (by returning a new buffer instance).
  ///
  /// \param connection   The connection to which the data handler belongs.
  /// \param buffer       The original buffer.
  /// \return             The new, modified buffer (or \p buffer if there was no modification).
  virtual neuraylib::IBuffer* handle(IConnection* connection, neuraylib::IBuffer* buffer) = 0;
};

/// The connection class represents a connection from a client to the server.
///
/// An instance of this interface is passed into the various registered callbacks.
///
/// \note
///   A pointer to an #mi::http::IConnection interface is only valid inside
///   #mi::http::IRequest_handler::handle(), #mi::http::ICGIRequest_handler::handle(),
///   #mi::http::IResponse_handler::handle(), or #mi::http::IData_handler::handle().
///
/// \note
///   This interface is not derived from #mi::base::IInterface. As one consequence, there are no
///   retain() and release() methods, and the interface cannot be used together with the
///   #mi::base::Handle class.
class IConnection
{
public:
  /// Returns the address of the peer of the connection.
  virtual const char* get_peer_address() = 0;

  /// Returns the address of the local side of the connection.
  virtual const char* get_local_address() = 0;

  /// Prints a string to the connection.
  ///
  /// \note
  ///   The string might be modified or discarded by data handlers before being sent.
  ///   The string might be queued before it is actually sent. See also #flush().
  ///
  /// \param string   The string to be written.
  /// \return         \c true, in case of success, and \c false in case of failure.
  virtual bool print(const char* string) = 0;

  /// Prints a string to the connection.
  ///
  /// \note
  ///   The string might be modified or discarded by data handlers before being sent.
  ///   The string might be queued before it is actually sent. See also #flush().
  ///
  /// \param string   The string to be written using \c printf()-like format specifiers, followed
  ///                 by matching arguments. The formatted message is limited to 16383 characters.
  /// \return         \c true, in case of success, and \c false in case of failure.
  bool printf(const char* string, ...)
#ifdef MI_COMPILER_GCC
    __attribute__((format(printf, 2, 3)))
#endif
  {
    va_list args;
    va_start(args, string);
    char buffer[16384];
#ifdef MI_COMPILER_MSC
    vsnprintf_s(&buffer[0], sizeof(buffer), sizeof(buffer) - 1, string, args);
#else
    vsnprintf(&buffer[0], sizeof(buffer), string, args);
#endif
    bool result = this->print(&buffer[0]);
    va_end(args);
    return result;
  }

  /// Enqueues a buffer to be sent on the connection.
  ///
  /// \note
  ///   The buffer might be modified or discarded by data handlers before being sent.
  ///   The string might be queued before it is actually sent. See also #flush().
  ///
  /// \param buffer   The buffer to be written. Ownership of the buffer passes from the caller
  ///                 to the connection.
  /// \return         \c true, in case of success, and \c false in case of failure.
  virtual bool enqueue(neuraylib::IBuffer* buffer) = 0;

  /// Waits until all enqueued data has been written.
  ///
  /// \return   0, in case of success, and -1 in case of failure.
  virtual Sint32 flush() = 0;

  /// Indicates whether an error occurred on the connection.
  ///
  /// \return   \c true, if there was an error, \c false otherwise.
  virtual bool check_error() = 0;

  /// Returns the request of the connection.
  virtual IRequest* get_request() = 0;

  /// Returns the response associated with the connection.
  virtual IResponse* get_response() = 0;

  /// Adds a data handler to the connection.
  ///
  /// The handler is removed when the connection is closed or when the handler's \c handle()
  /// method returns \c NULL.
  ///
  /// \param handler   The data handler.
  virtual void add_data_handler(IData_handler* handler) = 0;

  /// Attaches an attachment to the connection.
  ///
  /// Attachments can be used for communication between different handlers. This methods deals
  /// with arbitrary attachment types. The method #set_attachment() is more convenient for string
  /// attachments.
  ///
  /// \param key     Key of the attachment.
  /// \param value   Value of the attachment.
  virtual void set_data_attachment(const char* key, IData* value) = 0;

  /// Returns an attachment from the connection.
  ///
  /// Attachments can be used for communication between different handlers. This methods deals
  /// with arbitrary attachment types. The method #get_attachment() is more convenient for string
  /// attachments.
  ///
  /// \param key     The key of the attachment.
  /// \return        The value of the attachment, or \c NULL if there is no such attachment.
  virtual IData* get_data_attachment(const char* key) = 0;

  /// Returns an attachment from the connection.
  ///
  /// Attachments can be used for communication between different handlers. This methods deals
  /// with arbitrary attachment types. The method #get_attachment() is more convenient for string
  /// attachments.
  ///
  /// This templated member function is a wrapper of the non-template variant for the user's
  /// convenience. It eliminates the need to call
  /// #mi::base::IInterface::get_interface(const Uuid &)
  /// on the returned pointer, since the return type already is a pointer to the type \p T
  /// specified as template parameter.
  ///
  /// \tparam T      The interface type of the requested element.
  /// \param key     The key of the attachment.
  /// \return        The value of the attachment, or \c NULL if there is no such attachment.
  template <class T>
  T* get_data_attachment(const char* key)
  {
    IData* ptr = get_data_attachment(key);
    if (!ptr)
      return 0;
    T* ptr_T = static_cast<T*>(ptr->get_interface(typename T::IID()));
    ptr->release();
    return ptr_T;
  }
  /// Attaches a string attachment to the connection.
  ///
  /// Attachments can be used for communication between different handlers. This convenience
  /// methods deals with string attachments. Use #set_data_attachment() for all other data types.
  ///
  /// \param key     Key of the attachment.
  /// \param value   Value of the attachment.
  virtual void set_attachment(const char* key, const char* value) = 0;

  /// Returns a string attachment from the connection.
  ///
  /// Attachments can be used for communication between different handlers. This convenience
  /// methods deals with string attachments. Use #get_data_attachment() for all other data types.
  ///
  /// \param key     The key of the attachment.
  /// \return        The value of the attachment, or \c NULL if there is no such attachment.
  virtual const char* get_attachment(const char* key) = 0;

  /// Removes an attachment from the connection.
  ///
  /// Attachments can be used for communication between different handlers.
  ///
  /// \param key     The key of the attachment.
  virtual void remove_attachment(const char* key) = 0;

  /// Reads incoming CGI data into a supplied buffer.
  ///
  /// This method can only be called by CGI handlers. The total amount of data is limited by the
  /// advised Content-Length header.
  ///
  /// \note This method blocks until the given amount of bytes have been read, until the content
  ///       length is exhausted, or the connection was closed.
  ///
  /// \param size     The size of the buffer.
  /// \param buffer   The buffer to write the the data into.
  /// \return         The number of bytes read or 0 if no more data will come.
  virtual Sint32 read_data(Size size, char* buffer) = 0;

  /// Query whether this is a secure (i.e. HTTPS) connection
  /// \return A bool indicating whether this is a secure connection
  virtual bool is_secure_connection() = 0;
};

/// A WebSocket state handler that can be installed to a WebSocket connection to handle events of
/// the WebSocket.
///
/// After its installation, the WebSocket state handler is called whenever the WebSocket changes its
/// state. The WebSocket state handler can query the WebSocket's state by calling
/// #mi::http::IWeb_socket::get_state().
class IWeb_socket_state_handler : public mi::base::Interface_declare<0x70ad6206, 0x38f0, 0x4f2a,
                                    0xb7, 0x5d, 0x8f, 0x90, 0x64, 0x3e, 0xd0, 0x06>
{
public:
  /// The handle() function is called when the WebSocket changes its state.
  ///
  /// \param web_socket  The WebSocket connection whose state changes.
  virtual void handle(IWeb_socket* web_socket) = 0;
};

/// A WebSocket data handler that can be installed to a WebSocket connection to handle data arriving
/// at the WebSocket.
///
/// After its installation, the WebSocket data handler is called when new data arrives on the
/// connection.
class IWeb_socket_data_handler : public mi::base::Interface_declare<0xbe989e2c, 0xf1e6, 0x492a,
                                   0xb6, 0x42, 0x1f, 0xd7, 0x30, 0x1f, 0xa2, 0x9f>
{
public:
  /// The handle() function is called when new data is received.
  ///
  /// \param web_socket     The WebSocket connection on which new data has arrived.
  /// \param buffer         A buffer containing received data.
  /// \param binary_frame   Indicates whether this data buffer is binary or text.
  virtual void handle(IWeb_socket* web_socket, neuraylib::IBuffer* buffer, bool binary_frame) = 0;
};

/// The WebSocket connection class represents a connection that is built on top of an HTTP
/// connection.
///
/// \note
///   For sending data, the function #mi::http::IWeb_socket::write() can be used.
///
/// \note
///   For receiving data, an #mi::http::IWeb_socket_data_handler needs to be installed. This handler
///   is called when new data arrives.
///
/// \note
///    For handling state changes, an #mi::http::IWeb_socket_state_handler needs to be installed.
///    This handler is called whenever the WebSocket changes its state.
class IWeb_socket : public mi::base::Interface_declare<0x52fd1beb, 0x4c6f, 0x4456, 0x86, 0x9c, 0xfd,
                      0xf4, 0x12, 0x52, 0x0a, 0xae>
{
public:
  /// Returns the peer's address of the connection.
  ///
  /// \return         The peer's address of the connection.
  virtual const char* get_peer_address() const = 0;

  /// Returns the local address of the connection.
  ///
  /// \return         The local address of the connection.
  virtual const char* get_local_address() const = 0;

  /// Returns the URL path that the WebSocket request is sent to.
  ///
  /// \return         The URL path that the WebSocket request is sent to.
  virtual const char* get_url_path() const = 0;

  /// This class represents different states that a WebSocket can be in.
  enum State
  {
    /// The initial state.
    WS_STATE_INIT,
    /// The client has sent a request and awaits a response.
    WS_STATE_CONNECTING,
    ///  A connection has been established.
    WS_STATE_CONNECTED,
    /// The closing handshake has been started.
    WS_STATE_CLOSING,
    /// The closing handshake has been completed or the or the underlying TCP connection has
    /// been closed.
    WS_STATE_CLOSED,
    /// An error has occurred.
    WS_STATE_ERROR,
    WS_STATE_FORCE_32_BIT = 0xffffffffU
  };

  /// Returns the state of the connection.
  ///
  /// \return         The state of the WebSocket connection.
  virtual State get_state() const = 0;

  /// Sets a state handler to the WebSocket connection.
  ///
  /// The state handler is called whenever the WebSocket changes its state. The state handler is a
  /// deferred callback, i.e., it is called after the #set_state_handler() method has been
  /// executed.
  ///
  /// \note When this method is called and the WebSocket is in the state CONNECTED, the state
  /// handler is also called back although there is no state change in this case.
  ///
  /// The handler is removed when the connection is closed.
  ///
  /// \param handler   The data handler.
  virtual void set_state_handler(IWeb_socket_state_handler* handler) = 0;

  /// Sets a data handler to the WebSocket connection.
  ///
  /// The data handler is called whenever new data arrives at the WebSocket. The data handler is a
  /// deferred callback, i.e., if data is available for reading at the WebSocket, the data handler
  /// is called after the #set_data_handler() method has been executed.
  ///
  /// The handler is removed when the connection is closed.
  ///
  /// \param handler   The data handler.
  virtual void set_data_handler(IWeb_socket_data_handler* handler) = 0;

  /// Writes data from a buffer to the connection.
  ///
  /// There is no limit on the buffer size on the sender's side. However, a WebSocket receiver
  /// currently limits the size of a receive buffer to 50 000 000 bytes. Thus, applications should
  /// keep the size of their transmitted data buffers below this limit. Otherwise, the transmitted
  /// data buffers will be truncated.
  ///
  /// \param buffer         The buffer containing data to be written to the socket.
  /// \param binary_frame   This flag indicates whether the data will be transmitted as binary
  ///                       frame or as text frame. For example, HTML 5 video data needs to be
  ///                       transmitted as binary frames.
  /// \return               The number of bytes written or -1 in case of errors in which case \c
  ///                       errno contains further information.
  virtual Difference write(neuraylib::IBuffer* buffer, bool binary_frame = false) = 0;

  /// Prints a string to the connection.
  ///
  /// \param string         The string to be written.
  /// \param binary_frame   This flag indicates whether the data will be transmitted as binary
  ///                       frame or as text frame. For example, HTML 5 video data needs to be
  ///                       transmitted as binary frames.
  /// \return               \c true, in case of success, and \c false in case of failure.
  virtual bool print(const char* string, bool binary_frame = false) = 0;

  /// Prints a string to the connection.
  ///
  /// \param string   The string to be written using \c printf()-like format specifiers, followed
  ///                 by matching arguments. The formatted message is limited to 16383 characters.
  ///                 The string will always be transmitted as text frame (see also #print()).
  /// \return         \c true, in case of success, and \c false in case of failure.
  bool printf(const char* string, ...)
#ifdef MI_COMPILER_GCC
    __attribute__((format(printf, 2, 3)))
#endif
  {
    va_list args;
    va_start(args, string);
    char buffer[16384];
#ifdef MI_COMPILER_MSC
    vsnprintf_s(&buffer[0], sizeof(buffer), sizeof(buffer) - 1, string, args);
#else
    vsnprintf(&buffer[0], sizeof(buffer), string, args);
#endif
    bool result = this->print(&buffer[0]);
    va_end(args);
    return result;
  }

  /// Closes the connection.
  virtual void close() = 0;

  /// Returns the HTTP connection associated with this WebSocket connection.
  virtual IConnection* get_http_connection() = 0;
};

mi_static_assert(sizeof(IWeb_socket::State) == sizeof(Uint32));

/// WebSocket handlers are responsible for handling WebSocket requests.
///
/// For every request the handlers are asked if they are responsible for the request. They will be
/// asked in the order they where added. If the \c handle() function of a handler returns \c true
/// the subsequent handlers will not be asked anymore.
///
/// \see #mi::http::IServer::install( IWeb_socket_handler*),
///      #mi::http::IServer::remove( IWeb_socket_handler*)
class IWeb_socket_handler : public mi::base::Interface_declare<0xb784d27c, 0x3b80, 0x432e, 0x89,
                              0xa0, 0x13, 0xe7, 0x33, 0x04, 0x82, 0x5c>
{
public:
  /// Handles an incoming WebSocket request.
  ///
  /// Returns \c true, if the request was completely handled, i.e., no more handlers should
  /// be called. Returns \c false, if the request was not completely handled, i.e., subsequent
  /// handlers should be called.
  ///
  /// \param web_socket The WebSocket on which the request came in.
  /// \return         \c true, if the request was completely handled, or \c false otherwise.
  virtual bool handle(IWeb_socket* web_socket) = 0;
};

/// Request handlers are responsible for handling HTTP requests.
///
/// For every request the handlers are asked if they are responsible for the request. They will be
/// asked in the order they where added. If the \c handle() function of a handler returns \c true
/// the subsequent handlers will not be asked anymore.
///
/// \see #mi::http::IServer::install(IRequest_handler*),
///      #mi::http::IServer::remove(IRequest_handler*)
class IRequest_handler : public mi::base::Interface_declare<0x8747d0dd, 0x1e27, 0x4413, 0xa0, 0xd4,
                           0x07, 0x60, 0x8f, 0xed, 0xfc, 0xf9>
{
public:
  /// Handles a request coming in on a connection.
  ///
  /// Returns \c true, if the request was was completely handled, i.e., no more handlers should
  /// be called. Returns \c false, if the request was not completely handled, i.e., the subsequent
  /// handlers should be called. In the latter case no calls to #mi::http::IConnection::print() or
  /// #mi::http::IConnection::enqueue() must have been used on the
  /// connection.
  ///
  /// \param connection   The connection on which the request came in.
  /// \return             \c true, if the request was completely handled, or \c false otherwise.
  virtual bool handle(IConnection* connection) = 0;
};

/// CGI request handlers are responsible for handling HTTP requests.
///
/// CGI handlers are very similar to normal request handlers, but they will be called before any
/// body data has been processed. They can then be used to consume the data by calling
/// #mi::http::IConnection::read_data() on the connection until all data is consumed.
///
/// For every request the handlers are asked if they are responsible for the request. They will be
/// asked in the order they where added. If the \c handle() function of a handler returns \c true
/// the subsequent handlers will not be asked anymore.
///
/// \see #mi::http::IServer::install(ICGIRequest_handler*),
///      #mi::http::IServer::remove(ICGIRequest_handler*)
class ICGIRequest_handler : public mi::base::Interface_declare<0xa7fe482e, 0x65f8, 0x4a4c, 0x87,
                              0x21, 0xff, 0x19, 0x21, 0x36, 0xda, 0xe0>
{
public:
  /// Handles a request coming in on a connection.
  ///
  /// Returns \c true, if the request was was completely handled, i.e., no more handlers should
  /// be called. Returns \c false, if the request was not completely handled, i.e., the subsequent
  /// handlers should be called. In the latter case no calls to #mi::http::IConnection::print() or
  /// #mi::http::IConnection::enqueue() must have been used on the
  /// connection.
  ///
  /// \param connection   The connection on which the request came in.
  /// \return             \c true, if the request was completely handled, or \c false otherwise.
  virtual bool handle(IConnection* connection) = 0;
};

/// Response handlers can be used to modify responses generated by request handlers.
///
/// For every response the handlers are asked if they want to modify the response. They will be
/// asked in the order they where added. This all happens after a responsible request handler issued
/// the response.
///
/// \see #mi::http::IServer::install(IResponse_handler*),
///      #mi::http::IServer::remove(IResponse_handler*)
class IResponse_handler : public mi::base::Interface_declare<0xa9386278, 0x6938, 0x45a7, 0xa2, 0x3b,
                            0xbb, 0x35, 0xf7, 0xe9, 0x28, 0x20>
{
public:
  /// Handles a response on a connection.
  ///
  /// \param connection   The connection on which the response is to be sent.
  virtual void handle(IConnection* connection) = 0;
};

/// The server builds a framework for the handlers.
class IServer : public mi::base::Interface_declare<0x9923b273, 0x082f, 0x403a, 0x83, 0x57, 0xcd,
                  0x23, 0x9b, 0xcf, 0x68, 0x4c>
{
public:
  /// Starts the server listening on the given address.
  ///
  /// This method can be called at most once. Additionally, the method #start_ssl() can also be
  /// called for secure connections.
  ///
  /// The server will run within a separate thread; all requests will be handled in their own
  /// threads as well.
  ///
  /// \see #shutdown()
  ///
  /// \param listen_address   The address to listen on.
  /// \return
  ///                                -  0: Success.
  ///                                - -1: Listen failed, e.g. \p listen_address already in use.
  ///                                - -2: The method has already been called.
  ///                                - -7: Invalid listen address
  virtual Sint32 start(const char* listen_address) = 0;

  /// Starts the server in SSL mode listening on the given address.
  ///
  /// This method can be called at most once. Additionally, the method #start() can also be called
  /// for unsecure connections.
  ///
  /// The server will run within a separate thread; all requests will be handled in their own
  /// threads as well.
  ///
  /// \see #shutdown()
  ///
  /// \param listen_address          The address to listen on.
  /// \param cert_file               The file containing the server's certificate. The certificate
  ///                                needs to be in PEM format. The DER format is not supported.
  /// \param private_key_file        The file containing the server's private key.
  /// \param password                The password for decrypting the private key.
  ///
  /// \return
  ///                                -  0: Success.
  ///                                - -1: Listen failed, e.g. \p listen_address already in use.
  ///                                - -2: The method has already been called.
  ///                                - -3: Invalid certificate.
  ///                                - -4: Invalid private key.
  ///                                - -5: Mismatch of certificate and private key.
  ///                                - -6: Cannot load SSL module.
  ///                                - -7: Invalid listen address
  virtual Sint32 start_ssl(const char* listen_address, const char* cert_file,
    const char* private_key_file, const char* password) = 0;

  /// Shuts down a server that has been previously started.
  ///
  /// The server will stop accepting new requests and shut down as fast as possible. However,
  /// requests currently being handled will be served until they are done.
  ///
  /// \see #start()
  virtual void shutdown() = 0;

  /// Sets the default MIME type to be used when no MIME type was found.
  ///
  /// \param mime_type   The default MIME type.
  /// \return            0, in case of success, or -1 in case of an error.
  virtual Sint32 set_default_mime_type(const char* mime_type) = 0;

  /// Adds a new MIME type to the server.
  ///
  ///  The server will use the given \p extension / \p mime_type pairs to set the content-type
  /// header in the response based on the extension of the requested URL. Note that a handler may
  /// overwrite the MIME type (as any header field), if necessary.
  ///
  /// \param extension   The extension identifying the MIME type.
  /// \param mime_type   The MIME type to be set when the extension was found.
  virtual void add_mime_type(const char* extension, const char* mime_type) = 0;

  /// Returns the MIME type registered for a certain extension.
  ///
  /// \param extension   The file extension to lookup.
  /// \return            The registered MIME type or \c NULL.
  virtual const char* lookup_mime_type(const char* extension) = 0;

  /// Adds a new request handler to the server.
  ///
  /// \param handler   The handler to be installed.
  virtual void install(IRequest_handler* handler) = 0;

  /// Removes a previously installed request handler from the server.
  ///
  /// \param handler   The handler to be removed.
  virtual void remove(IRequest_handler* handler) = 0;

  /// Adds a new WebSocket request handler to the server.
  ///
  /// \param handler   The handler to be installed.
  virtual void install(IWeb_socket_handler* handler) = 0;

  /// Removes a previously installed WebSocket request handler from the server.
  ///
  /// \param handler   The handler to be removed.
  virtual void remove(IWeb_socket_handler* handler) = 0;

  /// Adds a new CGI request handler to the server.
  ///
  /// \param handler   The handler to be installed.
  virtual void install(ICGIRequest_handler* handler) = 0;

  /// Removes a previously installed CGI request handler from the server.
  ///
  /// \param handler   The handler to be removed.
  virtual void remove(ICGIRequest_handler* handler) = 0;

  /// Adds a new response handler to the server.
  ///
  /// \param handler   The handler to be installed.
  virtual void install(IResponse_handler* handler) = 0;

  /// Removes a previously installed response handler from the server.
  ///
  /// \param handler   The handler to be removed.
  virtual void remove(IResponse_handler* handler) = 0;

  /// Sets the identification string of the server.
  ///
  /// This string is automatically put in the "Server" header of each response. A handler might
  /// overwrite it, if necessary on a per URL basis.
  ///
  /// \param id_string   The identification string.
  virtual void set_identification(const char* id_string) = 0;

  /// Returns the listen address of the server.
  virtual const char* get_listen_address() = 0;

  /// Returns the SSL listen address of the server.
  virtual const char* get_ssl_listen_address() = 0;

  /// Returns the number of existing HTTP connections in this server.
  ///
  /// \return The current number of connections.
  virtual Size get_nr_of_connections() = 0;

  /// Returns the number of HTTP connections in this server which actually handle requests.
  ///
  /// \return The current number of active connections.
  virtual Size get_nr_of_active_connections() = 0;

  /// Sets the idle timeout for keep-alive connections.
  ///
  /// The default is not to timeout.
  ///
  /// \param nr_of_seconds   Seconds of idleness allowed before the server closes the connection.
  virtual void set_keep_alive_timeout(Uint32 nr_of_seconds) = 0;

  /// Returns the idle timeout for keep-alive connections.
  virtual Uint32 get_keep_alive_timeout() const = 0;

  /// Sets the send buffer size of the socket.
  ///
  /// Increasing the buffer size can be useful in the case of high latency connections.
  ///
  /// \param send_buffer_size   Size of socket send buffer.
  virtual void set_send_buffer_size(Uint32 send_buffer_size) = 0;

  /// Returns the send buffer size of the socket.
  virtual Uint32 get_send_buffer_size() const = 0;

  /// Sets the size limit for the body of HTTP POST requests.
  ///
  /// \param http_post_body_limit  The new size limit.
  virtual void set_http_post_body_limit(Uint32 http_post_body_limit) = 0;

  /// Returns the size limit for the body of HTTP POST requests.
  virtual Uint32 get_http_post_body_limit() const = 0;
};

/// The factory can be used to instantiate the built-in HTTP classes.
///
/// In addition to the HTTP server it can create a number of built-in request and response handlers.
/// Those handlers need to be installed with one or more servers using the server's \c install()
/// methods. Unless indicated otherwise a handler obtained from the factory function can be
/// installed to multiple HTTP servers.
class IFactory : public mi::base::Interface_declare<0xddded154, 0x4be8, 0x42b6, 0x81, 0x68, 0x21,
                   0x16, 0xc7, 0xbd, 0x63, 0x40>
{
public:
  /// Creates a new HTTP server.
  virtual IServer* create_server() = 0;

  /// Creates a new file handler for the server.
  ///
  /// The handler will map a given file or directory into the URL namespace at the \p root_url.
  ///
  /// The handler needs to be installed with the desired server using
  /// #mi::http::IServer::install(IRequest_handler*).
  ///
  /// \param root_url       The URL needs to start with this prefix to be served by the handler.
  /// \param root_path      When a matching URL is found \p root_url is replaced by this argument
  ///                       to get the path of a file to be served. If this file can be opened for
  ///                       reading, it will be sent to the client.
  /// \param is_recursive   Indicates whether handler serves only a single file or a directory
  ///                       tree.
  /// \return               The new request handler.
  virtual IRequest_handler* create_file_handler(
    const char* root_url, const char* root_path, bool is_recursive = true) = 0;

  /// Creates a new redirect handler which will redirect requests to a certain URL to a new URL.
  ///
  /// The handler needs to be installed with the desired server using
  /// #mi::http::IServer::install(IRequest_handler*).
  ///
  /// \param source_url     If this URL is found it is redirected to the \p target_url.
  /// \param target_url     Target of the redirection.
  virtual IRequest_handler* create_redirect_handler(
    const char* source_url, const char* target_url) = 0;

  /// Creates a new log handler.
  ///
  /// This handler will write a request log to a file with a given path. The file format of the
  /// log file is the so called 'Combined log format' used by various HTTP servers.
  ///
  /// The handler needs to be installed with the desired server using
  /// #mi::http::IServer::install(IResponse_handler*).
  ///
  /// \param path Path of the log file to be written.
  virtual IResponse_handler* create_log_handler(const char* path) = 0;

  /// Creates a new chunked encoding handler.
  ///
  /// If installed on a server it will allow to keep connections alive even if the size of the
  /// generated content is not know in advance. This is done by installing a data handler on a
  /// connection if the response does not contain the data size when it is sent. The response
  /// handler will modify the response and then modify all data passed to the client to be sent in
  /// chunks. This handler needs to be installed as the \em last response handler to work
  /// correctly. Exempt from this are response handlers which do not change the data on the way to
  /// its clients, e.g., the log handler.
  ///
  /// The handler needs to be installed with the desired server using
  /// #mi::http::IServer::install(IResponse_handler*).
  virtual IResponse_handler* create_chunked_encoding_handler() = 0;

  /// Creates a client-side WebSocket connection that will attempt to establish a connection to
  /// the specified server's address and will send WebSocket requests to the specified URL.
  ///
  /// \param web_socket_address   The address to be connected to.
  /// \param connect_timeout      The timeout interval in seconds when attempting to connect to a
  ///                             server.
  /// \return                     A valid WebSocket if a connection can be established to the
  ///                             server, or \c NULL in case of failures.
  ///
  /// \note
  ///   The WebSocket address must have one of the following two formats:
  ///   - \c "ws://host_address[:host_port]/url" for unsecure connections, or
  ///   - \c "wss://host_address[:host_port]/url" for secure connections.
  ///
  virtual IWeb_socket* create_client_web_socket(
    const char* web_socket_address, Float32 connect_timeout) = 0;
};

/*@}*/ // end group mi_neuray_http

} // namespace http

} // namespace mi

#endif // MI_NEURAYLIB_HTTP_H
