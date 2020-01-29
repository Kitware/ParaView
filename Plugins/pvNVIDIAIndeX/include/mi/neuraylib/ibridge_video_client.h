/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief  Bridge video handling

#ifndef MI_UNPUBLISHED_BRIDGE_VIDEO_CLIENT_H
#define MI_UNPUBLISHED_BRIDGE_VIDEO_CLIENT_H

#include <mi/base/interface_declare.h>
#include <mi/neuraylib/typedefs.h>
#include <mi/neuraylib/idata.h>

namespace mi {

class IData;
class IString;

namespace neuraylib { class IBuffer; class ICanvas; }

namespace bridge {

class IVideo_source;
class IVideo_sink;

/** \addtogroup mi_neuray_bridge_client
@{
*/

/// Client-side video context that receives and decodes video frames from the corresponding
/// server-side video context.
///
/// An application using the video transmission facilities of the Bridge API must implement the
/// abstract interface #mi::bridge::IVideo_sink and set it by calling #set_video_sink() to receive
/// video frames.
///
/// Note that even though the focus is on video, it is equally valid to transmit any kind of data
/// suitable for streaming, and the application can deliver pure data buffers in addition to, or
/// instead of, video frames.
///
/// \see #mi::bridge::IClient_session::create_video_context(), #mi::bridge::IServer_video_context
class IClient_video_context : public
    mi::base::Interface_declare<0x4b817707,0xa206,0x46a6,0xa2,0xb4,0xb1,0x9b,0x3f,0x10,0x78,0xeb>
{
public:
    /// Sets the video sink that will receive the decoded video frames sent from the server.
    ///
    /// \param video_sink   The video sink to set. Replaces any previously set video sink (if any).
    ///                     Pass \c NULL to disconnect the currently set video sink.
    virtual void set_video_sink( IVideo_sink* video_sink) = 0;

    /// Returns the currently set video sink, or \c NULL if none is set.
    virtual IVideo_sink* get_video_sink() const = 0;

    /// Sets the GPU to use for nvcuvid hardware h264 decoding.
    ///
    /// \param device    The CUDA ID the GPU to use, or -1 to enable auto selection.
    /// \return          0 in case of success, < 0 in case of failures.
    virtual Sint32 set_nvcuvid_device( Sint32 device) = 0;

    /// Returns the GPU uses for nvcuvid hardware h264 decoding.
    ///
    /// \return          The CUDA ID of the GPU to use, or -1 if auto selection is enabled.
    virtual Sint32 get_nvcuvid_device() const = 0;

    /// Closes the video stream associated with this context and frees all resources.
    ///
    /// This will also close the associated video context on the server.
    ///
    /// \see #set_video_sink(), #mi::bridge::IServer_video_context
    virtual void close() = 0;

    /// Returns the ID of this video context.
    ///
    /// The ID needs to be communicated to the server-side part of the application to obtain the
    /// server-side counterpart of this video context.
    virtual Uint32 get_id() const = 0;
};

/// Represents the data for a single video frame.
///
/// \see #mi::bridge::IVideo_sink::video_frame()
class IVideo_frame : public
    mi::base::Interface_declare<0xaf02b8cc,0x772f,0x4415,0x95,0xae,0x8c,0xb4,0xa3,0xf6,0xd9,0xdd>
{
public:
    /// Returns the canvas that contains the decoded frame data.
    virtual neuraylib::ICanvas* get_canvas() const = 0;

    /// The video format used when encoding this frame on the server.
    virtual const char* get_video_format() const = 0;

    /// The encoding time in seconds.
    virtual Float32 get_encode_time() const = 0;

    /// The decoding time in seconds
    virtual Float32 get_decode_time() const = 0;

    /// The size of the encoded frame in bytes.
    virtual Size get_compressed_size() const = 0;

    /// Returns a video format attribute by name.
    /// Currently only the attribute "bitrate" is supported and only for video format
    /// "h264".
    virtual IData* get_video_format_attribute(const char* attribute) const = 0;

    template <typename T>
    T* get_video_format_attribute(const char* attribute) const
    {
        IData* attr = get_video_format_attribute(attribute);
        if(!attr)
            return 0;
        return static_cast<T*>(attr->get_interface( typename T::IID()));
    }
};

/// Abstract interface to receive video frames produced by the corresponding server-side video
/// context.
///
/// \see #mi::bridge::IClient_video_context::set_video_sink()
class IVideo_sink : public
    mi::base::Interface_declare<0xa94825d1,0x1ed8,0x4465,0x8d,0x80,0x9e,0x1f,0xf7,0xc6,0x1e,0x65>
{
public:
    /// Called when a frame has arrived from the corresponding server side video context.
    ///
    /// The \p frame_data buffer is specified by the server-side application when the frame is
    /// produced and can contain any kind of additional information about the frame. For
    /// applications that are not video-centric, the frame data can also be sent instead of a video
    /// frame. The raw data buffer will be delivered to the client as is (not encoded or compressed
    /// in any way) and it is up to the application to define the format of the data.
    ///
    /// \param video_frame     The video frame to display. Can be \c NULL.
    /// \param frame_data      Additional frame data. Can be \c NULL.
    virtual void video_frame( IVideo_frame* video_frame, neuraylib::IBuffer* frame_data) = 0;

    /// Called when progress messages arrive for the next frame from the server-side video source.
    ///
    /// \if IRAY_API This can be used to send progress messages for the frames produced by the
    /// server in a manner analogous to progress reporting performed by Iray render contexts. \endif
    ///
    /// \param value           A value indicating progress for the area.
    /// \param area            The area the progress message is for.
    /// \param message         A string containing a progress message or some progress data for the
    ///                        area.
    virtual void video_progress( Float64 value, const char* area, const char* message) = 0;

    /// Called if rendering or encoding on the server failed.
    ///
    /// \param error_code      Error code specified by the application.
    /// \param error_message   A short description of the error.
    virtual void video_error( Sint32 error_code, const char* error_message) = 0;

    /// Called when the video context has been closed.
    ///
    /// \param reason
    ///                        -  0: Closed by the client.
    ///                        -  1: Closed by the server.
    ///                        - -1: Network error.
    virtual void video_context_closed( Sint32 reason) = 0;
};

/*@}*/ // end group mi_neuray_bridge_client

} // namespace bridge

} // namespace mi

#endif // MI_UNPUBLISHED_BRIDGE_VIDEO_CLIENT_H
