/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Depth buffer provided by the application.

#ifndef NVIDIA_INDEX_IAPPLICATION_DEPTH_BUFFER_H
#define NVIDIA_INDEX_IAPPLICATION_DEPTH_BUFFER_H

#include <mi/base/interface_declare.h>
#include <mi/math/vector.h>

namespace nv
{
namespace index
{

/// Depth buffer provided by an application, enabling NVIDIA IndeX to do depth-correct
/// compositing with a rendering performed by the application.
///
/// The application will do its rendering first and then call \c IIndex_rendering::render(),
/// passing the depth buffer. The rendering result returned by NVIDIA IndeX can then be
/// alpha-blended for the application's rendering to get a correctly composited result.
///
/// \ingroup nv_index_rendering
///
class IApplication_depth_buffer :
    public mi::base::Interface_declare<0xc6efc2dd,0x0ef0,0x4e38,0x86,0x4d,0xca,0xd3,0x35,0x76,0xae,0x9a>
{
public:
    /// Returns the resolution of the depth buffer.
    ///
    /// \return resolution (width, height) in pixels
    virtual mi::math::Vector_struct<mi::Uint32, 2> get_resolution() const = 0;
};

/// Depth buffer in OpenGL format provided by an application, enabling NVIDIA IndeX to do
/// depth-correct compositing with a rendering performed by the application.
///
/// The depth buffer must be be in normalized unsigned integer format. It can be retrieved with an
/// OpenGL call such as:
/// \code
/// glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, buffer);
/// \endcode
///
/// \ingroup nv_index_rendering
///
class IApplication_depth_buffer_gl :
    public mi::base::Interface_declare<0x4d85444e,0x79f6,0x4e36,0x91,0xa8,0xb0,0xf2,0xda,0x40,0xfe,0xc3,
                                       nv::index::IApplication_depth_buffer>
{
public:
    /// Return a pointer to the the depth buffer
    ///
    /// \return pointer to buffer, or 0 if not initialized.
    virtual mi::Uint32* get_z_buffer_ptr() = 0;

    /// Returns the precision of the depth buffer values.
    ///
    /// \return Precision of the depth buffer in bits (commonly 24 bits)
    virtual mi::Uint32 get_z_buffer_precision() const = 0;
};

/// Generic floating point depth buffer format provided by an application, enabling NVIDIA IndeX to
/// do depth-correct compositing with a rendering performed by the application.
///
/// \ingroup nv_index_rendering
///
class IApplication_depth_buffer_float :
    public mi::base::Interface_declare<0x455960ae,0x8db9,0x4480,0x87,0x0c,0xe1,0xd3,0x70,0xef,0x73,0xed,
                                       nv::index::IApplication_depth_buffer>
{
public:
    /// Returns the pointer of the depth buffer. May be 0 if not initialized.
    ///
    /// \return pointer to the depth buffer
    virtual const mi::Float32* get_buffer() const = 0;

    /// Returns the depth value closest to the camera (typically 0.0).
    ///
    /// \return depth value
    virtual mi::Float32 get_near_value() const = 0;

    /// Returns the depth value farthest from the camera (typically 1.0).
    ///
    /// \return depth value
    virtual mi::Float32 get_far_value() const = 0;

    /// Returns whether buffer should be flipped vertically. NVIDIA IndeX expect the buffer to have
    /// its origin in the bottom left (as in OpenGL). If the buffer origin is in the upper left
    /// instead, this method should return \c true so that the buffer is flipped vertically along
    /// the y-axis.
    ///
    /// \return True if buffer should be flipped,
    ///
    virtual bool get_flip_vertically() const = 0;

    /// Returns the row pitch of the depth buffer. If used, it will define the number of bytes in
    /// each row, which otherwise defaults to width * sizeof(Float32). The padding data will be
    /// ignored by NVIDIA IndeX.
    ///
    /// \return Row pitch in bytes, will be ignored if less than width * sizeof(Float32).
    ///         The pitch must be a multiple of 4.
    ///
    virtual mi::Uint32 get_row_pitch() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IAPPLICATION_DEPTH_BUFFER_H
