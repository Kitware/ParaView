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

#include "vtknvindex_opengl_app_buffer.h"
#include "vtknvindex_forwarding_logger.h"
#include <vector>

//-----------------------------------------------------------------------------
vtknvindex_opengl_app_buffer::vtknvindex_opengl_app_buffer()
  : m_z_buffer(0)
  , m_z_buffer_precision(24)
  , m_resolution(-1, -1)
{
  // empty
}

//-----------------------------------------------------------------------------
vtknvindex_opengl_app_buffer::~vtknvindex_opengl_app_buffer()
{
  this->delete_memory();
}

//-----------------------------------------------------------------------------
mi::math::Vector_struct<mi::Uint32, 2> vtknvindex_opengl_app_buffer::get_resolution() const
{
  // mi::math::Vector_struct<mi::Sint32, 2> sint32_2_st;
  // sint32_2_st.x = m_resolution.x;
  // sint32_2_st.y = m_resolution.y;
  // return sint32_2_st;
  return mi::math::Vector<mi::Uint32, 2>(m_resolution.x, m_resolution.y);
}

//-----------------------------------------------------------------------------
mi::Uint32* vtknvindex_opengl_app_buffer::get_z_buffer_ptr()
{
  return m_z_buffer;
}

//-----------------------------------------------------------------------------
void vtknvindex_opengl_app_buffer::set_z_buffer_precision(mi::Uint32 precision)
{
  if (precision != 24 && precision != 32)
  {
    ERROR_LOG << "Trying to set unsupported z-buffer precision " << precision
              << ". Please verify the status of the X-server when the precision is reported as 0.";
    return;
  }

  m_z_buffer_precision = precision;
}

//-----------------------------------------------------------------------------
mi::Uint32 vtknvindex_opengl_app_buffer::get_z_buffer_precision() const
{
  return m_z_buffer_precision;
}

//-----------------------------------------------------------------------------
void vtknvindex_opengl_app_buffer::resize_buffer(
  mi::math::Vector_struct<mi::Sint32, 2> const& new_resolution)
{
  mi::math::Vector<mi::Sint32, 2> new_resolution_vec(new_resolution);
  if (m_resolution != new_resolution_vec)
  {
    this->delete_memory();
    this->allocate_memory(new_resolution_vec);
    this->clear_buffer();
  }
}

//-----------------------------------------------------------------------------
void vtknvindex_opengl_app_buffer::clear_buffer()
{
  if (m_z_buffer == 0)
    return;

  if (this->is_buffer_allocated())
  {
    mi::Sint64 const pixel_count = m_resolution.x * m_resolution.y;
    memset(m_z_buffer, 0, (pixel_count * sizeof(mi::Uint32)));
  }
}

//-----------------------------------------------------------------------------
bool vtknvindex_opengl_app_buffer::is_buffer_allocated() const
{
  if ((m_resolution.x > 0) && (m_resolution.y > 0) && (m_z_buffer != 0))
    return true;

  return false;
}

//-----------------------------------------------------------------------------
void vtknvindex_opengl_app_buffer::delete_memory()
{
  m_resolution = mi::math::Vector<mi::Sint32, 2>(-1, -1);
  if (m_z_buffer != 0)
  {
    delete[] m_z_buffer;
    m_z_buffer = 0;
  }
}

//-----------------------------------------------------------------------------
void vtknvindex_opengl_app_buffer::allocate_memory(
  mi::math::Vector<mi::Sint32, 2> const& new_resolution)
{
  if ((new_resolution.x <= 0) || (new_resolution.y <= 0))
  {
    ERROR_LOG << "vtknvindex_opengl_app_buffer::allocate_memory: "
              << "cannot allocate a negative memory size.";
    return;
  }

  m_resolution = new_resolution;
  mi::Sint64 const pixel_count = m_resolution.x * m_resolution.y;
  m_z_buffer = new mi::Uint32[pixel_count];
}
