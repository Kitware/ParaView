// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtknvindex_opengl_canvas_h
#define vtknvindex_opengl_canvas_h

#include <string>

#include "vtkOpenGLRenderWindow.h"
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
  void prepare() override;

  // Returns the resolution of the canvas in pixels.
  mi::math::Vector_struct<mi::Uint32, 2> get_resolution() const override;

  // Receive tile.
  // \deprecated
  void receive_tile(mi::Uint8* span_buffer, mi::Uint32 buffer_size,
    const mi::math::Bbox_struct<mi::Uint32, 2>& covered_area) override;

  // Receive tile and blend it.
  // \deprecated
  void receive_tile_blend(mi::Uint8* span_buffer, mi::Uint32 buffer_size,
    const mi::math::Bbox_struct<mi::Uint32, 2>& covered_area) override;

  // Finish the operations applied to the canvas (called once all tiles have been received).
  void finish() override;

  // The present tile rendering using OpenGL is not capable of
  // using multiple threads, because OpenGL requires a context
  // bound to each thread.
  bool is_multi_thread_capable() const override;

  // Set/get OpenGL viewport resolution.
  void set_buffer_resolution(const mi::math::Vector_struct<mi::Sint32, 2>& main_window_resolution);
  mi::math::Vector_struct<mi::Sint32, 2> get_buffer_resolution() const;

  // Stores the VTK Renderer pointer.
  void set_vtk_renderer(vtkRenderer* vtk_renderer);

  std::string get_class_name() const;

private:
  vtknvindex_opengl_canvas(vtknvindex_opengl_canvas const&) = delete;
  void operator=(vtknvindex_opengl_canvas const&) = delete;

  // Set main window size.
  mi::math::Vector_struct<mi::Sint32, 2> m_main_window_size;
  vtkRenderer* m_vtk_renderer;
  vtkOpenGLRenderWindow* m_vtk_ogl_render_window;
};

#endif // vtknvindex_opengl_canvas_h
