/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attributes controlling texture filtering modes.

#ifndef NVIDIA_INDEX_ITEXTURE_FILTER_MODE_H
#define NVIDIA_INDEX_ITEXTURE_FILTER_MODE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute

/// The interface class representing a texture filter mode.
///
/// A texture filter mode specifies what kind of interpolation of texture values
/// is performed upon rendering a textured primitive (\c IPlane, \c IRegular_heightfield).
///
/// This scene attribute is used to control the texture filtering for 2D textures mapped
/// mapped to surface compute primitives (i.e. \c IPlane, \c IRegular_heightfield) using
/// a compute technique (\c IDistributed_compute_technique).
///
class ITexture_filter_mode : public mi::base::Interface_declare<0xd80ecbb6, 0x3154, 0x45ae, 0x9d,
                               0x7, 0x8c, 0x19, 0xdf, 0xa, 0xd5, 0x8f, nv::index::IAttribute>
{
};

/// An interface class representing nearest-neighbor texture filtering.
///
class ITexture_filter_mode_nearest_neighbor
  : public mi::base::Interface_declare<0x9a606499, 0x567a, 0x4a1f, 0xbf, 0x83, 0xec, 0xb9, 0xfb,
      0x2b, 0x55, 0x68, nv::index::ITexture_filter_mode>
{
};

/// An interface class representing linear texture filtering.
///
class ITexture_filter_mode_linear
  : public mi::base::Interface_declare<0xe287e50f, 0x8ecf, 0x4f0e, 0x99, 0xef, 0x51, 0x81, 0xc0,
      0x60, 0xdd, 0x2c, nv::index::ITexture_filter_mode>
{
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ITEXTURE_FILTER_MODE_H
