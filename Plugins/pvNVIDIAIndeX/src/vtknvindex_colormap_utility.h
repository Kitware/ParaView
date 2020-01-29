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

#ifndef vtknvindex_colormap_utility_h
#define vtknvindex_colormap_utility_h

#include <mi/dice.h>

class vtkVolume;
class vtknvindex_regular_volume_properties;

namespace nv
{
namespace index
{
class IColormap;
class IScene;
}
}

// The class vtknvindex_colormap utility is used to create/update IndeX's scene colormap
// based on ParaView's current dataset transfer function.

class vtknvindex_colormap
{

public:
  vtknvindex_colormap(
    const mi::neuraylib::Tag& volume_colormap_tag, const mi::neuraylib::Tag& slice_colormap_tag);
  ~vtknvindex_colormap();

  // Dumps colormap's values to console.
  void dump_colormap(mi::base::Handle<const nv::index::IColormap> const& cmap);

  // Updates an already created colormap in NVIDIA IndeX scene graph.
  void update_scene_colormaps(vtkVolume* vol,
    const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
    vtknvindex_regular_volume_properties* regular_volume_properties);

  // Check for any changes on ParaView's transfer function.
  bool changed() const;

private:
  vtknvindex_colormap(const vtknvindex_colormap&) = delete;
  void operator=(const vtknvindex_colormap&) = delete;

  // Normalize input range to scale range.
  void normalize(const mi::math::Vector<mi::Float32, 2>& input_range,
    const mi::math::Vector<mi::Float32, 2>& scale_range,
    mi::math::Vector<mi::Float32, 2>& output_range);

  // Get ParaView's transfer function and write it to an IndeX colormap.
  void get_paraview_colormap(vtkVolume* vol,
    vtknvindex_regular_volume_properties* regular_volume_properties,
    mi::base::Handle<nv::index::IColormap>& colormap);

  // Get ParaView's transfer function and write it to an IndeX colormap including slices.
  void get_paraview_colormaps(vtkVolume* vol,
    vtknvindex_regular_volume_properties* regular_volume_properties,
    mi::base::Handle<nv::index::IColormap>& volume_colormap,
    mi::base::Handle<nv::index::IColormap>& slice_colormap);

  bool m_changed;             // true if color map was changed and needs to be updated.
  mi::Uint64 m_color_mtime;   // Modification time for color transfer function.
  mi::Uint64 m_opacity_mtime; // Modification time for opacity transfer function.

  mi::neuraylib::Tag m_volume_colormap_tag; // Colormap shared among all volumes.
  mi::neuraylib::Tag m_slice_colormap_tag;  // Colormap shared among all slices.
};

#endif
