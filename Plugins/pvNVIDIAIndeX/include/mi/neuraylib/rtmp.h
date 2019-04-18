/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief An RTMP server.

#ifndef MI_NEURAYLIB_RTMP_H
#define MI_NEURAYLIB_RTMP_H

#include <mi/base/interface_declare.h>

namespace mi
{

class IData;
class IMap;

namespace neuraylib
{
class IVideo_encoder;
class IVideo_data;
}

/// Namespace for the RTMP server of the \neurayApiName.
/// \ingroup mi_neuray
namespace rtmp
{

class IConnection;
class IStream;

/** \defgroup mi_neuray_rtmp RTMP server
    \ingroup mi_neuray
*/

/** \addtogroup mi_neuray_rtmp

    The RTMP server module implements an RTMP server. It listens for incoming connections
    and calls the installed connect handlers to see if an incoming connection should be accepted.
    If it is accepted an RTMP connection is started and upon incoming RTMP commands the installed
    command handlers will be called.

    The server is multithreaded and will start one thread for each connection.

    Any number of connect handler classes may be installed to the RTMP server. Each of those
    handler classes has to implement a handle() function. This function gets the connection.
    From the connection the handle() function may get the various parameters of the connect
    packet and if it wants to claim it, it should return \c true. If all return \c false,
    the connection is refused.

    To create a server the #mi::rtmp::IFactory needs to be obtained from the \neurayApiName using
    #mi::neuraylib::INeuray::get_api_component(). The factory can then be used to create as many
    RTMP servers as needed.

    The handler classes are the only ones that need to be implemented by a user. And then only
    the ones where the user actually wants to do something. If not implemented, they will simply
    not be called. The Frame_event_handler runs in a separate thread to avoid that long rendering
    times affect the framerate.

    An example:

    \code

    void render_a_canvas_and_save() {  ... your code ... }
    mi::neuraylib::ICanvas* get_the_last_rendered_canvas() { ... your code ... }

    // The render loop
    class My_render_handler : public mi::Interface_implement<mi::rtmp::IRender_event_handler>
    {
    public:
        bool handle( mi::rtmp::IStream* stream)
        {
            render_a_canvas_and_save();
            return true;
         }
    };

    // The Frame loop, the actual handler where encoded video data is collected. Note that
    // it runs in another thread than the Render_event_handler which means the canvas accesses
    // need to be synchronized.
    class My_frame_handler : public mi::Interface_implement<mi::rtmp::IFrame_event_handler>
    {
    public:
        bool handle(
            mi::rtmp::IStream* stream, mi::neuraylib::IVideo_data** out, bool outqueue_is_full)
        {
            if (outqueue_is_full) // not an error, just no use in encoding a new frame
                return true;
            mi::base::Handle<mi::neuraylib::ICanvas> canvas( get_the_last_rendered_canvas());
            mi::base::Handle<mi::neuraylib::IVideo_encoder> codec( stream->get_video_codec());
            return codec->encode_canvas( canvas.get(), out) == 0;
        }
    };

    // Stream handler
    class My_stream_handler : public mi::Interface_implement<mi::rtmp::IStream_event_handler>
    {
    public:
        bool handle(
            bool is_create,
            mi::rtmp::IStream* stream,
            const mi::IData* command_arguments)
        {
            if( is_create) {
                mi::base::Handle<mi::rtmp::IRender_event_handler> render_event_handler(
                    new My_render_handler());
                stream->register_render_event_handler( render_event_handler.get());
                mi::base::Handle<mi::rtmp::IFrame_event_handler> frame_event_handler(
                    new My_frame_handler());
                stream->register_frame_event_handler( frame_event_handler.get());
            }
            return true;
        }
    };

    // Connect handler
    class My_connect_handler : public mi::Interface_implement<mi::rtmp::IConnect_event_handler>
    {
    public:
        bool handle(
            bool is_create,
            mi::rtmp::IConnection* connection,
            const mi::IData* cmd_arguments,
            const mi::IData* user_arguments)
        {
            if( is_create) {
                mi::base::Handle<mi::rtmp::IStream_event_handler> stream_event_handler(
                    new My_stream_handler());
                connection->register_stream_event_handler( stream_event_handler.get());
            }
            return true;
        }
    };

    mi::base::Handle<mi::rtmp::IFactory> rtmp_factory(
        neuray->get_api_component<mi::rtmp::IFactory>());
    mi::base::Handle<mi::rtmp::IServer>  rtmp_server( rtmp_factory->create_server());
    mi::base::Handle<mi::rtmp::IConnect_event_handler> connect_handler( new My_connect_handler());
    rtmp_server->install( connect_handler.get());
    rtmp_server->start( "0.0.0.0:1935");

    \endcode

    \note Some concepts might be easier to understand if having read the Adobe RTMP specification
          [\ref RTMPSPEC10 " RTMPSPEC10"]
*/

/** \addtogroup mi_neuray_rtmp
@{
*/

/// Superclass of all handlers of call events.
///
/// A subclass gets registered via #mi::rtmp::IConnection::register_remote_call_handler() and will
/// then be called in the case of a call event.
class ICall_event_handler : public mi::base::Interface_declare<0x9751dc66, 0xb064, 0x4ae8, 0xaa,
                              0x32, 0x54, 0x89, 0x41, 0x86, 0xcc, 0x1d>
{
public:
  /// Called on a remote call event.
  ///
  /// A call event is a form of remote procedure call from the client application to the RTMP
  /// server. Call handlers for remote commands are registered together with the name of the
  /// remote procedure call. The command and user objects supplied represent the command and
  /// user objects sent in the RTMP Call packet. The last argument is the outgoing response
  /// object where for example an error description or system statistics can be provided.
  ///
  /// \note The \p connection parameter is only valid during the call of the #handle() method and
  /// cannot be stored (even if proper reference counting is used).
  ///
  /// \param connection               The connection on which this call event applies to.
  /// \param procedure_name           The name of the procedure call
  /// \param command_arguments        The command object from the client.
  /// \param user_arguments           The user object from the client.
  /// \param[out] response_arguments  An empty object which the handler can use to pass data back
  ///                                 to the client in the response.
  /// \return                         \c true if the call succeeded, \c false otherwise.
  virtual bool handle(IConnection* connection, const char* procedure_name,
    const IData* command_arguments, const IData* user_arguments, IData** response_arguments) = 0;
};

/// Superclass of all handlers of create stream events.
///
/// A subclass gets registered via #mi::rtmp::IConnection::register_stream_event_handler() and will
/// be called in the case of a create stream event or when the stream is removed.
class IStream_event_handler : public mi::base::Interface_declare<0x103c7914, 0xe2be, 0x43aa, 0xa6,
                                0xc7, 0x23, 0xdd, 0x10, 0x5e, 0xb5, 0x61>
{
public:
  /// Called on a stream event.
  ///
  /// \note The \p stream parameter is only valid during the call of the #handle() method and
  /// cannot be stored (even if proper reference counting is used).
  ///
  /// \param is_create           Indicates if this is a create or remove event.
  /// \param stream              The new stream or the one being removed.
  /// \param command_arguments   The command object from the client.
  /// \return                    If \p is_create is \c false, the return value is ignored.
  ///                            If \p is_create is \c true, the return value indicates whether
  ///                            the stream shall get created.
  virtual bool handle(bool is_create, IStream* stream, const IData* command_arguments) = 0;
};

/// The connection class represents a connection from a client to the server. An instance of this
/// interface is passed into some of the registered callbacks.
///
/// \note
///   A pointer to an IConnection interface is only valid inside the callback's handle method.
class IConnection : public mi::base::Interface_declare<0x9a4d6604, 0x78f3, 0x4948, 0x98, 0x37, 0x73,
                      0x80, 0x0c, 0x57, 0xee, 0x35>
{
public:
  /// Registers a call event handler for the passed procedure name.
  ///
  /// \param call_handler      The call event handler for that procedure name. The value \c NULL
  ///                          removes the installed handler.
  /// \param procedure_name    The name of the remote call procedure. If not passed or
  ///                          set to \c NULL this will mean this call event handler will be the
  ///                          the default handler when no other specific handler was found.
  virtual void register_remote_call_handler(
    ICall_event_handler* call_handler, const char* procedure_name = 0) = 0;

  /// Registers a stream event handler.
  ///
  /// The stream event handler will get called when streams get created or removed.
  ///
  /// \param stream_event_handler   The stream event handler. The value \c NULL
  ///                               removes the installed handler.
  virtual void register_stream_event_handler(IStream_event_handler* stream_event_handler) = 0;

  /// Returns statistics for the connection.
  ///
  /// The contents are on purpose not documented but can be iterated over and printed for
  /// information purposes.
  ///
  /// \note The IMap returned need to be released to avoid a memory leak.
  ///
  /// \return   The statistics for the connection.
  virtual IMap* get_statistics() const = 0;

  /// Get the IP and port of the remote client.
  ///
  /// \return   The address of the peer of the connection.
  virtual const char* get_peer_address() const = 0;

  /// Sets a property on the connection.
  ///
  /// \param key     The key to set the value for.
  /// \param value   The value of the property.
  /// \return
  ///                -  0: Success.
  ///                - -1: Invalid key or value.
  virtual Sint32 set_property(const char* key, const char* value) = 0;

  /// Returns a property from the connection.
  ///
  /// \param key     The key to obtain the value for.
  /// \return        The value of the property or \c NULL if it does not exist.
  virtual const char* get_property(const char* key) const = 0;
};

/// Superclass of all handlers of connect events.
///
/// A subclass gets registered via #mi::rtmp::IServer::install() and will be called in the case of a
/// connect event. There can be several connect event handlers and on new connections each will
/// be called until one returns \c true or the list of handlers is exhausted.
class IConnect_event_handler : public mi::base::Interface_declare<0x57b2f74b, 0x3964, 0x45cf, 0x85,
                                 0xcd, 0xb3, 0xd3, 0x37, 0xbf, 0x09, 0xc2>
{
public:
  /// Called on a connect event.
  ///
  /// If the \p is_create parameter is \c true the supplied connection can be used to register
  /// handlers for stream and call events. If the connection should be allowed it should
  /// return \c true. This way several connect event handlers can be installed, each responsible
  /// for different applications. When a connection is closed, the same handler that
  /// initially allowed the connection is called again but with \p is_create set to \c false.
  /// The return value is only considered when \p is_create is \c true. Note that the arguments
  /// are no longer valid after the handler has returned so the connection pointer cannot
  /// be saved away.
  ///
  /// \note The \p connection parameter is only valid during the call of the #handle() method and
  /// cannot be stored (even if proper reference counting is used).
  ///
  /// \param is_create           Indicates whether this is a new or closed connection.
  /// \param connection          The corresponding connection which can be used to register call
  ///                            and stream event handlers.
  /// \param command_arguments   The command object from the client.
  /// \param user_arguments      The user object from the client.
  /// \return                    If \p is_create is \c false, the return value is ignored.
  ///                            If \p is_create is \c true, the return value indicates whether
  ///                            this connect event handler declares itself responsible for the
  ///                            connection.
  virtual bool handle(bool is_create, IConnection* connection, const IData* command_arguments,
    const IData* user_arguments) = 0;
};

/// Superclass of all handlers of play events.
///
/// A subclass gets registered via #mi::rtmp::IStream::register_play_event_handler() and will be
/// called in the case of a play event which can be a play or stop event.
class IPlay_event_handler : public mi::base::Interface_declare<0x59798950, 0x9a12, 0x48b3, 0x87,
                              0x14, 0x23, 0xb7, 0x2d, 0xd9, 0xaf, 0x71>
{
public:
  /// Called on a play or stop event.
  ///
  /// Play/stop events are similar to pause/resume events but are more expensive
  ///
  /// The event handler allows to filter play or stop events. If it returns \c true the render
  /// loop for the corresponding stream is affected as follows. If \p is_start is \c true the
  /// render event handler will be called as often as indicated by the maximum render rate for
  /// this stream. If \p is_start is \c false the render event handler will not be called anymore.
  /// If the play event handler returns \c false nothing changes.
  ///
  /// Play/stop events are similar to pause/resume events but are more expensive w.r.t. the
  /// internal state of the RTMP server.
  ///
  /// \note The \p stream parameter is only valid during the call of the #handle() method and
  /// cannot be stored (even if proper reference counting is used).
  ///
  /// \note A play handler is optional. If no handler is installed the play command
  /// from a video client will be allowed by default.
  ///
  /// \param is_start   \c true indicates a play event, \c false indicates a stop event.
  /// \param stream     The stream on which to start or stop playing.
  /// \param[out] out   Potential initialization/close video frame data can be returned here.
  /// \return           \c true if the event should be honored, or \c false if should be ignored.
  virtual bool handle(bool is_start, IStream* stream, neuraylib::IVideo_data** out) = 0;
};

/// Superclass of all handlers of pause events.
///
/// A subclass gets registered via #mi::rtmp::IStream::register_pause_event_handler() and will be
/// called in the case of a pause event which can be either pause or resume.
class IPause_event_handler : public mi::base::Interface_declare<0x5e4f3910, 0x3f00, 0x4e80, 0xa9,
                               0x77, 0x98, 0x15, 0x6d, 0xa0, 0x92, 0x8b>
{
public:
  /// Called on a pause or resume event.
  ///
  /// The event handler allows to filter pause or resume events. If it returns \c true the render
  /// loop for the corresponding stream is affected as follows. If \p is_pause is \c true the
  /// render event handler will not be called anymore. If \p is_pause is \c false the render event
  /// handler will again be called as often as indicated by the maximum render rate for the
  /// stream. If the pause event handler returns \c false nothing changes.
  ///
  /// Pause/resume events are similar to play/stop events but are less expensive w.r.t. the
  /// internal state of the RTMP server.
  ///
  /// \note The \p stream parameter is only valid during the call of the #handle() method and
  /// cannot be stored (even if proper reference counting is used).
  ///
  /// \param is_pause   \c true indicates a pause event, \c false indicates a resume event.
  /// \param stream     The stream on which to pause or resume playing.
  /// \return           \c true if the event should be honored, or \c false if should be ignored.
  virtual bool handle(bool is_pause, IStream* stream) = 0;
};

/// Superclass of all handlers of render events.
///
/// This is a synthetic handler that does not appear in the RTMP Command Messages section of the
/// Adobe RTMP specification [\ref RTMPSPEC10].
/// It allows the RTMP server to implement the render loop. It can be ignored if no rendering
/// is done.
class IRender_event_handler : public mi::base::Interface_declare<0xe3f21dfb, 0xe285, 0x4733, 0xab,
                                0x10, 0x3f, 0x0c, 0x5c, 0x3d, 0xdd, 0x7a>
{
public:
  /// Called on a render event.
  ///
  /// \note The \p stream parameter is only valid during the call of the #handle() method and
  /// cannot be stored (even if proper reference counting is used).
  ///
  /// \param stream   The stream on which the render handler is registered.
  /// \return         \c true in case of success, or \c false if an error occurred and the stream
  ///                 should be stopped by the RTMP server.
  virtual bool handle(IStream* stream) = 0;
};

/// Superclass of all handlers of frame events.
///
/// This is a synthetic handler that does not appear in the RTMP Command Messages section of the
/// Adobe RTMP specification [\ref RTMPSPEC10]
/// but is required to allow the RTMP server to maintain the correct frame rate of
/// video streams. It is running in another thread than the render thread which means
/// implementers need to think about synchronization. It should also never use more time than
/// 1/framerate for an optimal client-side experience.
class IFrame_event_handler : public mi::base::Interface_declare<0x0dacca64, 0x41ae, 0x407f, 0xbf,
                               0x9b, 0x97, 0xaf, 0xe7, 0x92, 0x12, 0xbd>
{
public:
  /// Called on a frame event.
  ///
  /// When the \p outqueue_is_full parameter is set the bandwidth is either not enough or the
  /// client is not consuming the frames fast enough. Encoding a new large frame will then
  /// queue up data which will degrade interactivity. Perhaps encoding the old canvas would
  /// then be preferable as most codecs in that case produce very small P-frames.
  ///
  /// \note The \p stream parameter is only valid during the call of the #handle() method and
  /// cannot be stored (even if proper reference counting is used).
  ///
  /// \param stream   The stream on which the frame handler is registered.
  /// \param[out] out An empty video frame that should be filled with video encoded frame data.
  /// \param outqueue_is_full Set to \c true when the sending queue to the client is buffering up.
  /// \return         \c true in case of success, or \c false if an error occurred and the stream
  ///                 should be stopped by the RTMP server.
  virtual bool handle(IStream* stream, neuraylib::IVideo_data** out, bool outqueue_is_full) = 0;
};

/// Representing an RTMP stream
class IStream : public mi::base::Interface_declare<0xa6532316, 0x9e4c, 0x4e12, 0x92, 0x63, 0xc6,
                  0x2a, 0x5c, 0xda, 0xdd, 0x28>
{
public:
  /// Tells the RTMP server which codec will be used on the stream.
  ///
  /// If no codec is set the data passed via the IFrame_event_handler will be sent out
  /// as is without any RTMP encapsulation.
  ///
  /// \param name   The name of the codec to use, for example "screen video", "sorenson" or
  ///               "h264".
  /// \return       \c true if that codec is supported, i.e., the RTMP server has support for this
  ///               codec (possibly through a video plugin) and the codec is one of the supported
  ///               formats within an RTMP stream.
  virtual bool use_codec(const char* name) = 0;

  /// Returns the video codec for this stream.
  ///
  /// The method #use_codec() must be called first to set the video code for this stream.
  ///
  /// \return   The video encoder for this stream.
  virtual neuraylib::IVideo_encoder* get_video_codec() = 0;

  /// Registers the render event handler.
  ///
  /// \param render_event_handler   The render event handler to install. The value \c NULL
  ///                               removes the installed handler.
  virtual void register_render_event_handler(IRender_event_handler* render_event_handler) = 0;

  /// Registers the frame event handler.
  ///
  /// \param frame_event_handler    The frame event handler to install. The value \c NULL
  ///                               removes the installed handler.
  virtual void register_frame_event_handler(IFrame_event_handler* frame_event_handler) = 0;

  /// Registers the play event handler.
  ///
  /// \param play_event_handler     The play event handler to install. The value \c NULL
  ///                               removes the installed handler.
  virtual void register_play_event_handler(IPlay_event_handler* play_event_handler) = 0;

  /// Registers the pause event handler.
  ///
  /// \param pause_event_handler    The pause event handler to install. The value \c NULL
  ///                               removes the installed handler.
  virtual void register_pause_event_handler(IPause_event_handler* pause_event_handler) = 0;

  /// Returns the name of the current playing stream.
  ///
  /// This is the name passed in as an argument by the RTMP player in the RTMP protocol play
  /// message.
  ///
  /// \return   The name of this stream or \c NULL if not applicable.
  virtual const char* get_stream_name() const = 0;

  /// Sets a property on the stream.
  ///
  /// Currently, the only reserved system property is "render_rate". It indicates how often the
  /// RTMP server will try to call the registered render event handler. Bandwidth throttling
  /// and longer render times make this rate a not always reachable target. If this variable
  /// is left unset by the user the RTMP server will try to find a suitable default.
  ///
  /// \note It is recommended that user property names begin with the prefix \c "user_" to
  ///       avoid future name clashes.
  ///
  /// \param key     The key to set the value for.
  /// \param value   The value of the property
  /// \return
  ///                -  0: Success.
  ///                - -1: There is no property with that name.
  ///                - -2: Invalid value.
  virtual Sint32 set_property(const char* key, const char* value) = 0;

  /// Returns a property from the stream.
  ///
  /// \param key     The key to obtain the value for.
  /// \return        The value of the property or \c NULL if it does not exist.
  virtual const char* get_property(const char* key) const = 0;

  /// Returns the connection for this stream.
  ///
  /// Useful for registering specific remote call handlers.
  ///
  /// \return the underlying IConnection on which this stream runs
  virtual IConnection* get_connection() = 0;
};

/// The server builds a framework for the handlers.
class IServer : public mi::base::Interface_declare<0xe0a7301d, 0xb555, 0x4fc6, 0xb5, 0x1d, 0x26,
                  0x1c, 0xbe, 0x73, 0xea, 0x47>
{
public:
  /// Starts the server listening on the given address.
  ///
  /// The server will run within a separate thread; all requests will be handled in their own
  /// threads as well.
  ///
  /// \param listen_address   The address to listen on
  /// \return                 0, if the server could be started, or -1 in case of errors
  ///
  /// \see #shutdown()
  virtual Sint32 start(const char* listen_address) = 0;

  /// Shuts down a server that has been previously started.
  ///
  /// The server will stop accepting new requests and shut down as fast as possible. However,
  /// requests currently being handled will be served until they are done.
  ///
  /// \see #start()
  virtual void shutdown() = 0;

  /// Adds a new connect event handler to the server.
  ///
  /// The connection handler will be called upon new connections and when connections are closed.
  ///
  /// \param handler The handler to be installed.
  virtual void install(IConnect_event_handler* handler) = 0;

  /// Removes a previously installed connect event handler from the server.
  ///
  /// \param handler The handler to be removed.
  virtual void remove(IConnect_event_handler* handler) = 0;

  /// Returns the listen address of the server.
  virtual const char* get_listen_address() const = 0;
};

/// The factory can be used to instantiate the built-in RTMP server.
class IFactory : public mi::base::Interface_declare<0x2e6055f1, 0xf94d, 0x4a2c, 0xb2, 0x0a, 0x30,
                   0xde, 0xb6, 0xde, 0xc8, 0x8c>
{
public:
  /// Creates a new RTMP server.
  /// \return The new RTMP server
  virtual IServer* create_server() = 0;
};

/*@}*/ // end group mi_neuray_rtmp

} // namespace rtmp

} // namespace mi

#endif // MI_NEURAYLIB_RTMP_H
