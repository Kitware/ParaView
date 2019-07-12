/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
///
/// \brief OpenGL application buffer interface for NVIDIA IndeX 
/// OpenGL integration.

#ifndef NVIDIA_INDEX_IOPENGL_APPLICATION_BUFFER_H
#define NVIDIA_INDEX_IOPENGL_APPLICATION_BUFFER_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/base/uuid.h>
#include <mi/math/vector.h>

namespace nv
{
namespace index
{

/// OpenGL z-buffer maintained by the application.
///
/// The application draws an OpenGL opaque object into this buffer and
/// transfers its contents to the NVIDIA IndeX library through this interface.
///
/// @ingroup nv_index_rendering
///
class IOpengl_application_buffer :
    public mi::base::Interface_declare<0xc66fb0b9,0x6e76,0x4a58,0xa6,0x3a,0xfa,0xb7,0x67,0x39,0x78,0x71>
{
public:
    /// Get the buffer resolution.
    ///
    /// \return resolution (width,height) in pixels
    virtual mi::math::Vector_struct<mi::Uint32, 2> get_resolution() const = 0;

    /// Get the pointer of the z-buffer. It has the same structure as the OpenGL
    /// z-buffer. May be 0 if not initialized.
    /// This returns a writable raw pointer, use with care.
    /// \return pointer to the top address of mi::Uint32[pixel_count]
    virtual mi::Uint32* get_z_buffer_ptr() = 0;

    /// Get z-buffer precision.
    ///
    /// \return precision of the z-buffer in bits (commonly 24 bits)
    virtual mi::Uint32 get_z_buffer_precision() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IOPENGL_APPLICATION_BUFFER_H
