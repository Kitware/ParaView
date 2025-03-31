/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Backward compatibility for deprecated depth buffer API.

#ifndef NVIDIA_INDEX_IOPENGL_APPLICATION_BUFFER_H
#define NVIDIA_INDEX_IOPENGL_APPLICATION_BUFFER_H

#include <mi/dice.h>
#include <nv/index/iapplication_depth_buffer.h>

namespace nv
{
namespace index
{

/// Typedef for backward compatibility with deprecated \c IOpengl_application_buffer.
/// \deprecated Use \c Application_depth_buffer_opengl instead.
typedef IApplication_depth_buffer_gl IOpengl_application_buffer;

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IOPENGL_APPLICATION_BUFFER_H
