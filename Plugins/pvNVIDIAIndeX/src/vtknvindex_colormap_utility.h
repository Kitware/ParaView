// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
