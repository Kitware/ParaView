// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtknvindex_opengl_app_buffer_h
#define vtknvindex_opengl_app_buffer_h

#include <nv/index/iopengl_application_buffer.h>

#include <cassert>
#include <string>

// The class represents the application's OpenGL frame buffer.
class vtknvindex_opengl_app_buffer
  : public mi::base::Interface_implement<nv::index::IOpengl_application_buffer>
{
public:
  vtknvindex_opengl_app_buffer();
  virtual ~vtknvindex_opengl_app_buffer();

  // Returns the resolution (width,height) in pixels.
  // Returns (-1, -1) if not initialized.
  mi::math::Vector_struct<mi::Uint32, 2> get_resolution() const override;

  // Get the pointer to the Z-buffer, same structure as OpenGL Z-buffer.
  // Returns nullptr if not initialized.
  // This returns writable raw pointer. Use with care.
  // Depth value is in the range [0, maxuint].
  // Returns pointer to the top address of mi::Uint32[pixel_count].
  mi::Uint32* get_z_buffer_ptr() override;

  // Set/Get Z-buffer precision (bits).
  void set_z_buffer_precision(mi::Uint32 precision);
  mi::Uint32 get_z_buffer_precision() const override;

  // Resizes the buffer, results in memory reallocation.
  void resize_buffer(mi::math::Vector_struct<mi::Sint32, 2> const& new_resolution);

  // Clear the frame buffer.
  void clear_buffer();

  // Returns true when the buffer is allocated.
  bool is_buffer_allocated() const;

  // Returns true when the buffer can be used.
  bool is_valid() const;

private:
  vtknvindex_opengl_app_buffer(vtknvindex_opengl_app_buffer const&) = delete;
  void operator=(vtknvindex_opengl_app_buffer const&) = delete;

  // Deallocate memory
  void delete_memory();

  // Allocate memory according to the new_resolution.
  // Deallocation/initialization must be done before this call.
  void allocate_memory(mi::math::Vector<mi::Sint32, 2> const& new_resolution);

  mi::Uint32* m_z_buffer;                       // Z-buffer.
  mi::Uint32 m_z_buffer_precision;              // Z-buffer precision.
  mi::math::Vector<mi::Sint32, 2> m_resolution; // Buffer resolution.
};

#endif
