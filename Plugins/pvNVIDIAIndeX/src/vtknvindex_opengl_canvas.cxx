/* Copyright 2019 NVIDIA Corporation. All rights reserved.
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
#include "vtkPVConfig.h"
#if PARAVIEW_VERSION_MAJOR == 5 && PARAVIEW_VERSION_MINOR >= 6
#define USE_VTK_OGL_STATE
#endif

#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_opengl_canvas.h"

#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#ifdef USE_VTK_OGL_STATE
#include "vtkOpenGLState.h"
#endif
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
vtknvindex_opengl_canvas::~vtknvindex_opengl_canvas()
{
  // empty
}

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
#ifdef USE_VTK_OGL_STATE
  vtkOpenGLState* ostate = m_vtk_ogl_render_window->GetState();

  if (ostate)
  {
    ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ostate->vtkglDisable(GL_LIGHTING);
  }
#else
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_LIGHTING);
#endif
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

#ifdef USE_VTK_OGL_STATE
  vtkOpenGLState* ostate = m_vtk_ogl_render_window->GetState();

  if (ostate)
  {
    ostate->ResetGLBlendFuncState();
    ostate->ResetGLDepthMaskState();
    ostate->ResetEnumState(GL_DEPTH_TEST);
  }
#endif
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
#ifdef USE_VTK_OGL_STATE
  vtkOpenGLState* ostate = m_vtk_ogl_render_window->GetState();

  if (ostate)
  {
    ostate->vtkglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ostate->vtkglEnable(GL_DEPTH_TEST);
    ostate->vtkglDepthMask(GL_TRUE);
  }
#else
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
#endif
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
