/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Video plugin API

#ifndef MI_NEURAYLIB_IVIDEO_PLUGIN_H
#define MI_NEURAYLIB_IVIDEO_PLUGIN_H

#include <mi/base/types.h>
#include <mi/base/plugin.h>
#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

class IBuffer;
class ICanvas;
class ICanvas_cuda;
class IPlugin_api;

/** \addtogroup mi_neuray_plugins
@{
*/

/// Type of video encoder plugins
#define MI_NEURAY_VIDEO_PLUGIN_TYPE "video v21"

/// A buffer for video data representing a frame.
class IVideo_data : public
    mi::base::Interface_declare<0xbdd686fa,0x3e37,0x43aa,0xbd,0xe6,0x7b,0xab,0x9f,0x3e,0x1c,0xfc>
{
public:
    /// Returns a pointer to the data of the buffer.
    /// \return A pointer to the beginning of the buffer.
    virtual Uint8* get_data() = 0;

    /// Returns the size of the buffer.
    /// \return The size of the buffer.
    virtual Size get_data_size() const = 0;

    /// Indicates whether this frame is a key frame
    /// \return \c true if this frame is a key frame, \c false otherwise.
    virtual bool is_key_frame() const = 0;
};


/// Abstract interface for video encoders.
///
/// Note that the video encoder has an internal state. Hence, a separate encoder is needed for each
/// stream. It is not possible to use one encoder for different streams, even if all parameters are
/// identical. Therefore, the terms "encoder" and "stream" are used synonymously below.
class IVideo_encoder : public
    mi::base::Interface_declare<0x572ef6ad,0xbc37,0x417d,0xbf,0x7c,0x17,0x17,0x4e,0x96,0xa3,0x06>
{
public:
    /// Initializes the video stream.
    ///
    /// \param resolution_x The width of the video stream.
    /// \param resolution_y The height of the video stream.
    /// \param[out] out     Optionally, the method  may return data. The ownership of \c *out is
    ///                     passed to the caller as for a return value. In particular, the caller
    ///                     must call release() when \c *out is no longer needed.
    /// \return
    ///                     -   0: If the encoder was successfully initialized.
    ///                     -  -1: Invalid parameters.
    ///                     -  -2: If the specified resolution is not supported.
    ///                     -  -3: Already initialized or closed.
    ///                     -  -4: No suitable device (hardware encoders only)
    ///                     -  -5: Failed to initialize encoder or other needed libraries.
    ///                     - <-5: For unspecified error.
    virtual Sint32 init(
        Uint32 resolution_x, Uint32 resolution_y,
        IVideo_data** out) = 0;

    /// Sets a parameter for the video stream.
    ///
    /// \param name         The name of the parameter to be changed.
    /// \param value        The new value to be set for the parameter.
    /// \return             \c true, if the parameter could be successfully set,
    ///                     \c false otherwise.
    virtual bool set_parameter(const char* name, const char* value) = 0;

    /// Returns the value of a parameter for the video stream.
    ///
    /// \param name         The name of the parameter to query.
    /// \return             The value of the parameter, or NULL if it doesn't exist.
    virtual const char* get_parameter(const char* name) = 0;

    /// Returns the \p index -th supported pixel type. The canvas passed to encode_canvas() must
    /// use one of the supported types.
    ///
    /// The pixel types should be ordered, from index 0 for the most preferred to the least
    /// preferred type. See \ref mi_neuray_types for a list of supported pixel types.
    ///
    /// \param index   The index of the pixel type to be returned.
    /// \return        The \p index -th supported pixel type, \c NULL if \p index is out of
    ///                bounds.
    virtual const char* get_supported_type(Uint32 index) const = 0;

    /// Encodes the pixel data contained in a canvas.
    ///
    /// \param canvas       Encode this canvas.
    /// \param[out] out     The encoded data. The ownership of \c *out is passed to the caller as
    ///                     for a return value. In particular, the caller must call release() when
    ///                     \c *out is no longer needed.
    /// \return
    ///                     -   0: Canvas successfully encoded.
    ///                     -  -1: Invalid parameters.
    ///                     -  -2: Not initialized or closed.
    ///                     -  -3: Memory/buffer allocation problem.
    ///                     -  -4: Failed to encode frame.
    ///                     - <-4: For unspecified error.
    virtual Sint32 encode_canvas(const ICanvas* canvas, IVideo_data** out) = 0;

    /// Encodes the pixel data contained in a cuda canvas.
    ///
    /// \param canvas       Encode this cuda canvas.
    /// \param[out] out     The encoded data. The ownership of \c *out is passed to the caller as
    ///                     for a return value. In particular, the caller must call release() when
    ///                     \c *out is no longer needed.
    /// \return
    ///                     -   0: Canvas successfully encoded.
    ///                     -  -1: Invalid parameters.
    ///                     -  -2: Not initialized or closed.
    ///                     -  -3: Memory/buffer allocation problem.
    ///                     -  -4: Failed to encode frame.
    ///                     -  -5: unsupported canvas type
    ///                     - <-5: For unspecified error.
    virtual Sint32 encode_canvas(const ICanvas_cuda* canvas, IVideo_data** out) = 0;

    /// Closes the video stream.
    ///
    /// \param[out] out     Returns any remaining encoded data here. The ownership of \c *out is
    ///                     passed to the caller as for a return value. In particular, the caller
    ///                     must call release() when \c *out is no longer needed.
    /// \return
    ///                     -   0: Successfully closed.
    ///                     -  -1: Not initialized or already closed.
    ///                     -  -2: Failed to perform cleanup.
    ///                     - <-2: Unspecified error.
    virtual Sint32 close(IVideo_data** out) = 0;

    /// Returns a concise single-line unique identifier.
    virtual const char* get_identifier() const = 0;
};

/// Abstract interface for video decoders.
///
/// Note that the video decoder has an internal state. Hence, a separate decoder is needed for each
/// stream. It is not possible to use one decoder for different streams, even if all parameters are
/// identical.
class IVideo_decoder : public
    mi::base::Interface_declare<0xe7fa52c7,0xd881,0x4a29,0x9e,0x82,0x3b,0xdd,0xa6,0xcf,0x14,0xc8>
{
public:
    /// Initializes the video decoder. If not enough data is provided then 1 is returned and
    /// this method must be called again with more data. If enough data was present to
    /// decode one or more frames, then those frames will be decoded as part of this call.
    /// After the call to init(), call decode_canvas() with data set to NULL to get any buffered
    /// frames.
    ///
    /// \param data           Video stream data to be decoded.
    /// \return
    ///                       -   0: If the decoder was successfully initialized.
    ///                       -   1: Need more data.
    ///                       -  -1: Invalid parameters.
    ///                       -  -2: Already initialized or closed.
    ///                       -  -3: No suitable device found (hardware decoders only).
    ///                       -  -4: Failed to initialize encoder or other needed libraries.
    ///                       -  -5: Error parsing data.
    ///                       - <-5: Unspecified error.
    virtual Sint32 init( IBuffer* data) = 0;

    /// Returns the encoded width of the video stream. Only available after a completed
    /// initialization.
    virtual Uint32 get_encoded_width() = 0;

    /// Returns the encoded height of the video stream. Only available after a completed
    /// initialization.
    virtual Uint32 get_encoded_height() = 0;

    /// Returns a video stream format parameter. Only available after initialization is completed.
    virtual const char* get_format_parameter( const char* parameter) = 0;

    /// Returns the \p index -th supported pixel type. The canvas passed to decode_canvas() must
    /// use one of the supported types.
    ///
    /// The pixel types should be ordered, from index 0 for the most preferred to the least
    /// preferred type. See \ref mi_neuray_types for a list of supported pixel types.
    ///
    /// \param index   The index of the pixel type to be returned.
    /// \return        The \p index -th supported pixel type, \c NULL if \p index is out of
    ///                bounds.
    virtual const char* get_supported_type( Uint32 index) const = 0;

    /// Decodes video stream data to a canvas.
    ///
    /// Note that the provided data might contain enough information for several frames in which
    /// case 1 is returned. Call multiple times with \c NULL for \p data until 0 (or 2) is
    /// returned. If the data is not enough for a complete frame then 2 is returned in which case
    /// decode_canvas() must be called with more data before the frame can be decoded.
    ///
    /// \param canvas          The target canvas.
    /// \param data            The data to decode.
    /// \return
    ///                        -   0: Frame successfully decoded.
    ///                        -   1: Successfully decoded frame, more frames are buffered.
    ///                        -   2: Insufficient data, call again with more data.
    ///                        -  -1: Invalid parameters.
    ///                        -  -2: Not initialized or closed.
    ///                        -  -3: Parse error.
    ///                        - <-3: Unspecified error.
    virtual Sint32 decode_canvas(neuraylib::ICanvas* canvas, IBuffer* data) = 0;

    /// Closes the video stream.
    ///
    /// \return
    ///        -  0: Decoder successfully closed.
    ///        - <0: Unspecified error.
    virtual Sint32 close() = 0;

    /// Returns a concise single-line unique identifier.
    virtual const char* get_identifier() const = 0;

    /// Sets a parameter of the decoder.
    ///
    /// \param name         The name of the parameter to be changed.
    /// \param value        The new value to be set for the parameter.
    /// \return             \c true, if the parameter could be successfully set,
    ///                     \c false otherwise.
    virtual bool set_parameter( const char* name, const char* value) = 0;

    /// Returns the value of a parameter of the decoder.
    ///
    /// \param name         The name of the parameter to query.
    /// \return             The value of the parameter.
    virtual const char* get_parameter( const char* name) = 0;
};


/// Abstract interface for video encoder plugins.
///
/// Video plugins need to return #MI_NEURAY_VIDEO_PLUGIN_TYPE in #mi::base::Plugin::get_type().
class IVideo_plugin : public base::Plugin
{
public:
    /// Returns the name of the plugin.
    ///
    /// For video plugins, typically the name of the video codec is used, for example, \c "x264".
    ///
    /// \note This method from #mi::base::Plugin is repeated here only for documentation purposes.
    virtual const char* get_name() const = 0;

    /// Initializes the plugin.
    ///
    /// \param plugin_api   Provides access to API components available for plugins.
    /// \return             \c true in case of success, and \c false otherwise.
    virtual bool init( IPlugin_api* plugin_api) = 0;

    /// De-initializes the plugin.
    ///
    /// \param plugin_api   Provides access to API components available for plugins.
    /// \return             \c true in case of success, and \c false otherwise.
    virtual bool exit( IPlugin_api* plugin_api) = 0;

    /// Creates a new encoder for a video stream.
    virtual IVideo_encoder* create_video_encoder() const = 0;

    /// Creates a new decoder for a video stream.
    virtual IVideo_decoder* create_video_decoder() const = 0;
};

/// API component that allow creation of installed video encoders and decoders.
class IVideo_codec_factory : public
    mi::base::Interface_declare<0x79be801,0x17a7,0x48d8,0x8a,0x4,0x73,0xd4,0x35,0x6d,0x2,0x28>
{
public:
    /// Creates a video encoder for a given codec.
    ///
    /// Returns a video encoder for the requested codec, or \c NULL if unsupported.
    virtual IVideo_encoder* create_video_encoder( const char* codec_name) = 0;

    /// Returns a video decoder for the requested codec, or \c NULL if unsupported.
    virtual IVideo_decoder* create_video_decoder( const char* codec_name) = 0;
};

/*@}*/ // end group mi_neuray_plugins

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IVIDEO_PLUGIN_H
