// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtknvindex_opengl_canvas.h"
#include "vtknvindex_forwarding_logger.h"

#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkRenderWindow.h"

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

//-------------------------------------------------------------------------------------------------
vtknvindex_opengl_canvas::vtknvindex_opengl_canvas()
{
  m_main_window_size.x = 0;
  m_main_window_size.y = 0;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_opengl_canvas::~vtknvindex_opengl_canvas() = default;

//-------------------------------------------------------------------------------------------------
bool vtknvindex_opengl_canvas::is_multi_thread_capable() const
{
  return false;
}

//-------------------------------------------------------------------------------------------------
mi::math::Vector_struct<mi::Sint32, 2> vtknvindex_opengl_canvas::get_buffer_resolution() const
{
  return m_main_window_size;
}

//-------------------------------------------------------------------------------------------------
std::string vtknvindex_opengl_canvas::get_class_name() const
{
  return std::string("vtknvindex_opengl_canvas");
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_opengl_canvas::initialize_gl()
{
  vtkOpenGLState* ostate = m_vtk_ogl_render_window->GetState();

  if (ostate)
  {
    ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ostate->vtkglDisable(GL_LIGHTING);
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_opengl_canvas::prepare()
{
  // Set blending mode for pre-multiplied alpha values
  // (i.e. RGB values were already multiplied by the alpha).
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Disable depth buffer writes and depth test, as we are doing 2D operations without
  // considering depth information.
  glDepthMask(GL_FALSE);
  glDisable(GL_DEPTH_TEST);

  vtkOpenGLState* ostate = m_vtk_ogl_render_window->GetState();

  if (ostate)
  {
    ostate->ResetGLBlendFuncState();
    ostate->ResetGLDepthMaskState();
    ostate->ResetEnumState(GL_DEPTH_TEST);
  }
}

//-------------------------------------------------------------------------------------------------
mi::math::Vector_struct<mi::Uint32, 2> vtknvindex_opengl_canvas::get_resolution() const
{
  return mi::math::Vector<mi::Uint32, 2>(m_main_window_size.x, m_main_window_size.y);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_opengl_canvas::receive_tile(
  mi::Uint8* buffer, mi::Uint32 /*buffer_size*/, const mi::math::Bbox_struct<mi::Uint32, 2>& area)
{
  m_vtk_renderer->GetRenderWindow()->MakeCurrent();

  const mi::Sint32 src_width = area.max.x - area.min.x;
  const mi::Sint32 src_height = area.max.y - area.min.y;

  if (src_width <= 0 || src_height <= 0)
  {
    return;
  }

  if (!buffer)
  {
    DEBUG_LOG << "No image buffer contents to be rendered possibly due to internal optimizations.";
    return;
  }

  vtkWindow* win = m_vtk_renderer->GetVTKWindow();
  double* tvp = win->GetTileViewport();
  int* img_size = win->GetSize();

  const mi::math::Bbox_struct<mi::Uint32, 2> tile = { static_cast<mi::Uint32>(tvp[0] * img_size[0]),
    static_cast<mi::Uint32>(tvp[1] * img_size[1]), static_cast<mi::Uint32>(tvp[2] * img_size[0]),
    static_cast<mi::Uint32>(tvp[3] * img_size[1]) };

  mi::math::Bbox_struct<mi::Uint32, 2> src_tile = { area.min.x < tile.min.x ? tile.min.x
                                                                            : area.min.x,
    area.min.y < tile.min.y ? tile.min.y : area.min.y,
    area.max.x > tile.max.x ? tile.max.x : area.max.x,
    area.max.y > tile.max.y ? tile.max.y : area.max.y };

  if (src_tile.min.x >= src_tile.max.x || src_tile.min.y >= src_tile.max.y)
    return;

  const mi::math::Bbox_struct<mi::Uint32, 2> dst_tile = { src_tile.min.x - tile.min.x,
    src_tile.min.y - tile.min.y, src_tile.max.x - tile.min.x, src_tile.max.y - tile.min.y };

  src_tile.min.x -= area.min.x;
  src_tile.min.y -= area.min.y;
  src_tile.max.x -= area.min.x;
  src_tile.max.y -= area.min.y;

  vtkOpenGLClearErrorMacro();

  m_vtk_ogl_render_window->DrawPixels(dst_tile.min.x, dst_tile.min.y, dst_tile.max.x - 1,
    dst_tile.max.y - 1, src_tile.min.x, src_tile.min.y, src_tile.max.x - 1, src_tile.max.y - 1,
    src_width, src_height, 4, VTK_UNSIGNED_CHAR, buffer);

  vtkOpenGLStaticCheckErrorMacro("Failed after vtknvindex_opengl_canvas::receive_tile.");
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_opengl_canvas::receive_tile_blend(
  mi::Uint8* buffer, mi::Uint32 buffer_size, const mi::math::Bbox_struct<mi::Uint32, 2>& area)
{
  // Blending is used by default.
  receive_tile(buffer, buffer_size, area);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_opengl_canvas::finish()
{
  // Restore default settings.
  vtkOpenGLState* ostate = m_vtk_ogl_render_window->GetState();

  if (ostate)
  {
    ostate->vtkglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ostate->vtkglEnable(GL_DEPTH_TEST);
    ostate->vtkglDepthMask(GL_TRUE);
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_opengl_canvas::set_buffer_resolution(
  const mi::math::Vector_struct<mi::Sint32, 2>& main_window_resolution)
{
  m_main_window_size = main_window_resolution;

  vtkWindow* win = m_vtk_renderer->GetVTKWindow();
  mi::Sint32* window_size = win->GetActualSize();

  glViewport(0, 0, window_size[0], window_size[1]);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_opengl_canvas::set_vtk_renderer(vtkRenderer* vtk_renderer)
{
  m_vtk_renderer = vtk_renderer;
  m_vtk_ogl_render_window = vtkOpenGLRenderWindow::SafeDownCast(m_vtk_renderer->GetVTKWindow());
}
