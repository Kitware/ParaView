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

#include <vector>

#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include <vtkDiscretizableColorTransferFunction.h>

#include <nv/index/icolormap.h>
#include <nv/index/iscene.h>

#include "vtknvindex_colormap_utility.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_regular_volume_properties.h"
#include "vtknvindex_utilities.h"

//-------------------------------------------------------------------------------------------------
vtknvindex_colormap::vtknvindex_colormap(
  const mi::neuraylib::Tag& volume_colormap_tag, const mi::neuraylib::Tag& slice_colormap_tag)
  : m_changed(false)
  , m_color_mtime(0)
  , m_opacity_mtime(0)
  , m_volume_colormap_tag(volume_colormap_tag)
  , m_slice_colormap_tag(slice_colormap_tag)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_colormap::~vtknvindex_colormap()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_colormap::dump_colormap(mi::base::Handle<const nv::index::IColormap> const& cmap)
{
  mi::Uint32 const cmap_count = static_cast<mi::Uint32>(cmap->get_number_of_entries());
  std::string const vecname = "colormap_entry";
  INFO_LOG << "std::vector< mi::math::Color > " << vecname << ";" << std::endl;

  for (mi::Uint32 i = 0; i < cmap_count; ++i)
  {
    mi::math::Color_struct const colst = cmap->get_color(i);
    INFO_LOG << vecname << ".push_back(mi::math::Color(" << colst.r << ", " << colst.g << ", "
             << colst.b << ", " << colst.a << "));" << std::endl;
  }
}

//-------------------------------------------------------------------------------------------------
// Normalize input
void vtknvindex_colormap::normalize(const mi::math::Vector<mi::Float32, 2>& input_range,
  const mi::math::Vector<mi::Float32, 2>& scale_range,
  mi::math::Vector<mi::Float32, 2>& output_range)
{
  output_range.x = (input_range.x - scale_range.x) / (scale_range.y - scale_range.x);
  output_range.y = (input_range.y - scale_range.x) / (scale_range.y - scale_range.x);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_colormap::get_paraview_colormap(vtkVolume* vol,
  vtknvindex_regular_volume_properties* regular_volume_properties,
  mi::base::Handle<nv::index::IColormap>& colormap)
{
  vtkColorTransferFunction* app_color_transfer_function =
    vol->GetProperty()->GetRGBTransferFunction(0);
  vtkPiecewiseFunction* app_opacity_transfer_function = vol->GetProperty()->GetScalarOpacity(0);

  // Range of the colormap to be mapped to respective voxels.
  mi::Float64 color_range[2];
  app_color_transfer_function->GetRange(color_range);

  mi::math::Vector<mi::Float32, 2> voxel_range;
  regular_volume_properties->get_voxel_range(voxel_range);

  // Normalize the range only if it is not floating point data.
  // If float, then set the domain values as-is from ParaView.
  std::string scalar_type;
  regular_volume_properties->get_scalar_type(scalar_type);

  // Colormap size used by ParaView
  // (see vtkOpenGLVolumeRGBTable).
  const mi::Uint64 array_size = 256;

  std::vector<mi::Float32> color_array;
  std::vector<mi::Float32> opacity_array;

  mi::math::Vector<mi::Float32, 2> domain_range;
  if (scalar_type != "float" && scalar_type != "double")
  {
    mi::math::Vector<mi::Float32, 2> scalar_range;
    regular_volume_properties->get_scalar_range(scalar_range);

    normalize(
      mi::math::Vector<mi::Float32, 2>(color_range[0], color_range[1]), scalar_range, domain_range);
  }
  else
  {
    domain_range.x = color_range[0];
    domain_range.y = color_range[1];
  }

  // Read color values from ParaView.
  color_array.resize(3 * array_size);
  // using Logarithmic scale?
  if (app_color_transfer_function->GetScale())
  {
    double color[3];
    for (mi::Uint32 i = 0; i < array_size; i++)
    {
      mi::Float32 value = static_cast<mi::Float32>(i) / array_size;
      value = voxel_range.x + (voxel_range.y - voxel_range.x) * value;
      app_color_transfer_function->GetColor(value, color);
      color_array[i * 3] = color[0];
      color_array[i * 3 + 1] = color[1];
      color_array[i * 3 + 2] = color[2];
    }
  }
  else // Linear scale
  {
    app_color_transfer_function->GetTable(
      voxel_range.x, voxel_range.y, array_size, &color_array[0]);
  }

  // Read opacity values from ParaView.
  opacity_array.resize(array_size);
  app_opacity_transfer_function->GetTable(
    voxel_range.x, voxel_range.y, array_size, &opacity_array[0]);

  std::vector<mi::math::Color_struct> colormap_entry;
  for (mi::Uint32 idx = 0; idx < array_size; ++idx)
  {
    colormap_entry.push_back(mi::math::Color(color_array[3 * idx + 0], color_array[3 * idx + 1],
      color_array[3 * idx + 2], opacity_array[idx]));
  }

  colormap->set_domain_boundary_mode(nv::index::IColormap::CLAMP_TO_EDGE);
  colormap->set_domain(domain_range.x, domain_range.y);
  colormap->set_colormap(&(colormap_entry[0]), array_size);

  m_color_mtime = app_color_transfer_function->GetMTime();
  m_opacity_mtime = app_opacity_transfer_function->GetMTime();
}

void vtknvindex_colormap::get_paraview_colormaps(vtkVolume* vol,
  vtknvindex_regular_volume_properties* regular_volume_properties,
  mi::base::Handle<nv::index::IColormap>& volume_colormap,
  mi::base::Handle<nv::index::IColormap>& slice_colormap)
{
  vtkColorTransferFunction* app_color_transfer_function =
    vol->GetProperty()->GetRGBTransferFunction(0);
  vtkPiecewiseFunction* app_opacity_transfer_function = vol->GetProperty()->GetScalarOpacity(0);
  vtkDiscretizableColorTransferFunction* app_discret_color_transfer_function =
    vtkDiscretizableColorTransferFunction::SafeDownCast(app_color_transfer_function);

  // Range of the colormap to be mapped to respective voxels.
  mi::Float64 color_range[2];
  app_color_transfer_function->GetRange(color_range);

  mi::math::Vector<mi::Float32, 2> voxel_range;
  regular_volume_properties->get_voxel_range(voxel_range);

  // Normalize the range only if it is not floating point data.
  // If float, set the domain values as-is from ParaView.
  std::string scalar_type;
  regular_volume_properties->get_scalar_type(scalar_type);

  // Colormap size used by ParaView
  // see vtkOpenGLVolumeRGBTable
  const mi::Uint64 array_size = 256;

  std::vector<mi::Float32> color_array;
  std::vector<mi::Float32> opacity_array;

  mi::math::Vector<mi::Float32, 2> domain_range;
  if (scalar_type != "float" && scalar_type != "double")
  {
    mi::math::Vector<mi::Float32, 2> scalar_range;
    regular_volume_properties->get_scalar_range(scalar_range);

    normalize(
      mi::math::Vector<mi::Float32, 2>(color_range[0], color_range[1]), scalar_range, domain_range);
  }
  else
  {
    domain_range.x = color_range[0];
    domain_range.y = color_range[1];
  }

  // Read color values from ParaView.
  color_array.resize(3 * array_size);

  // using Logarithmic scale?
  if (app_color_transfer_function->GetScale())
  {
    double color[3];
    for (mi::Uint32 i = 0; i < array_size; i++)
    {
      mi::Float32 value = static_cast<mi::Float32>(i) / array_size;
      value = voxel_range.x + (voxel_range.y - voxel_range.x) * value;
      app_color_transfer_function->GetColor(value, color);
      color_array[i * 3] = color[0];
      color_array[i * 3 + 1] = color[1];
      color_array[i * 3 + 2] = color[2];
    }
  }
  else // Linear scale
  {
    app_color_transfer_function->GetTable(
      voxel_range.x, voxel_range.y, array_size, &color_array[0]);
  }

  // Read opacity values from ParaView.
  opacity_array.resize(array_size);
  app_opacity_transfer_function->GetTable(
    voxel_range.x, voxel_range.y, array_size, &opacity_array[0]);

  std::vector<mi::math::Color_struct> colormap_entry;
  for (mi::Uint32 idx = 0; idx < array_size; ++idx)
  {
    colormap_entry.push_back(mi::math::Color(color_array[3 * idx + 0], color_array[3 * idx + 1],
      color_array[3 * idx + 2], opacity_array[idx]));
  }

  volume_colormap->set_domain_boundary_mode(nv::index::IColormap::CLAMP_TO_EDGE);
  volume_colormap->set_domain(domain_range.x, domain_range.y);
  volume_colormap->set_colormap(&(colormap_entry[0]), array_size);

  if (!app_discret_color_transfer_function->GetEnableOpacityMapping())
  {
    for (mi::Uint32 idx = 0; idx < array_size; ++idx)
      colormap_entry[idx].a = 1.f;
  }

  slice_colormap->set_domain_boundary_mode(nv::index::IColormap::CLAMP_TO_EDGE);
  slice_colormap->set_domain(domain_range.x, domain_range.y);
  slice_colormap->set_colormap(&(colormap_entry[0]), array_size);

  m_color_mtime = app_color_transfer_function->GetMTime();
  m_opacity_mtime = app_opacity_transfer_function->GetMTime();
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_colormap::update_scene_colormaps(vtkVolume* vol,
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
  vtknvindex_regular_volume_properties* regular_volume_properties)
{
  vtkColorTransferFunction* app_color_transfer_function =
    vol->GetProperty()->GetRGBTransferFunction(0);
  vtkPiecewiseFunction* app_opacity_transfer_function = vol->GetProperty()->GetScalarOpacity(0);

  // Check if the colormap has changed, if not simply return.
  // TODO: Check if there is a better way to do this.
  if (m_color_mtime == app_color_transfer_function->GetMTime() &&
    m_opacity_mtime == app_opacity_transfer_function->GetMTime())
  {
    m_changed = false;
    return;
  }

  assert(dice_transaction.is_valid_interface());

  // Update the existing colormap.
  mi::base::Handle<nv::index::IColormap> volume_colormap(
    dice_transaction->edit<nv::index::IColormap>(m_volume_colormap_tag));
  assert(volume_colormap.is_valid_interface());

  mi::base::Handle<nv::index::IColormap> slice_colormap(
    dice_transaction->edit<nv::index::IColormap>(m_slice_colormap_tag));
  assert(slice_colormap.is_valid_interface());

  get_paraview_colormaps(vol, regular_volume_properties, volume_colormap, slice_colormap);

  // Dump colormap into the console, copy/paste directly into.
  // dump_colormap(colormap);

  m_changed = true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_colormap::changed() const
{
  return m_changed;
}
