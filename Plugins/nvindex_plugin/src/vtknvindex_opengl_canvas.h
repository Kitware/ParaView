/* Copyright 2018 NVIDIA Corporation. All rights reserved.
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

#ifndef vtknvindex_opengl_canvas_h
#define vtknvindex_opengl_canvas_h

#include <string>

#include "vtkRenderer.h"

#include <mi/base/interface_implement.h>
#include <nv/index/iindex.h>

// This class implements a NVIDIA IndeX canvas that directs the rendering results to an attached
// OpenGL frame buffer.
class vtknvindex_opengl_canvas : public mi::base::Interface_implement<nv::index::Index_canvas>
{
public:
  vtknvindex_opengl_canvas();
  virtual ~vtknvindex_opengl_canvas();

  // Initialize OpenGL flags.
  void initialize_gl();

  // Prepare the rendering (called before receive tile).
  virtual void prepare();

  // Receive tile.
  // \deprecated
  virtual void receive_tile(mi::Uint8* span_buffer, mi::Uint32 buffer_size,
    const mi::math::Bbox_struct<mi::Uint32, 2>& covered_area);

  // Receive tile and blend it.
  // \deprecated
  virtual void receive_tile_blend(mi::Uint8* span_buffer, mi::Uint32 buffer_size,
    const mi::math::Bbox_struct<mi::Uint32, 2>& covered_area);

  // Finish the operations applied to the canvas (called once all tiles have been received).
  virtual void finish();

  // The present tile rendering using OpenGL is not capable of
  // using multiple threads, because OpenGL requires a context
  // bound to each thread.
  virtual bool is_multi_thread_capable() const;

  // Set/get OpenGL viewport resolution.
  virtual void set_buffer_resolution(
    const mi::math::Vector_struct<mi::Sint32, 2>& main_window_resolution);
  virtual mi::math::Vector_struct<mi::Sint32, 2> get_buffer_resolution() const;

  // Stores the VTK Renderer pointer.
  void set_vtk_renderer(vtkRenderer* vtk_renderer);

  std::string get_class_name() const;

private:
  vtknvindex_opengl_canvas(vtknvindex_opengl_canvas const&);
  vtknvindex_opengl_canvas const& operator=(vtknvindex_opengl_canvas const&);

  // Set main window size.
  mi::math::Vector_struct<mi::Sint32, 2> m_main_window_size;
  vtkRenderer* m_vtk_renderer;
};

#endif // vtknvindex_opengl_canvas_h
