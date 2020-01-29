/* Copyright 2020 NVIDIA Corporation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
  // Returns NULL if not initialized.
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

  // Get the pixel index. Assume that the buffer size is less than 2GB.
  static inline mi::Sint32 get_pixel_index(mi::Sint32 width, mi::Sint32 height, mi::Sint32 channel,
    mi::Sint32 x, mi::Sint32 y, mi::Sint32 rgba)
  {
    if ((0 <= x) && (x < width) && (0 <= y) && (y < height) && (0 <= rgba) && (rgba < channel))
      return ((channel * (x + (y * width))) + rgba);
  }

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
