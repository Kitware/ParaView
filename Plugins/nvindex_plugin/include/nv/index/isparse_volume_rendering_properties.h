/******************************************************************************
 * Copyright 2018 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling sparse volume rendering properties.

#ifndef NVIDIA_INDEX_ISPARSE_VOLUME_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_ISPARSE_VOLUME_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute
///
/// Filtering modes (interpolation) for sparse volume access.
///
enum Sparse_volume_filter_mode
{
  SPARSE_VOLUME_FILTER_NEAREST =
    0x00, ///< Access a single voxel with nearest filtering (a.k.a. point filtering).
  SPARSE_VOLUME_FILTER_TRILINEAR_POST =
    0x01 ///< Trilinear interpolation with post-classification, using hardware filtering.
};

/// @ingroup nv_index_scene_description_attribute
///
/// The interface class representing rendering properties for sparse volume data.
///
class ISparse_volume_rendering_properties
  : public mi::base::Interface_declare<0x718c9406, 0x99a7, 0x41cc, 0xb1, 0x48, 0xe, 0x36, 0x8d,
      0x67, 0x85, 0x19, nv::index::IAttribute>
{
public:
  /// Set the sampling distance used for a sparse volume scene element (\c
  /// ISparse_volume_scene_element).
  /// The default value used if 1.0f.
  ///
  /// \param[in]  sample_dist     Sampling distance (default value is 1.0f).
  ///
  virtual void set_sampling_distance(mi::Float32 sample_dist) = 0;
  /// Returns the sampling distance used for a sparse volume scene element (\c
  /// ISparse_volume_scene_element).
  virtual mi::Float32 get_sampling_distance() const = 0;

  /// Set the volume filter mode for a sparse volume scene element (\c
  /// ISparse_volume_scene_element).
  /// The default filter used is \c SPARSE_VOLUME_FILTER_NEAREST.
  ///
  /// \param[in]  filter_mode     Filter mode (default value is \c SPARSE_VOLUME_FILTER_NEAREST).
  ///
  virtual void set_filter_mode(Sparse_volume_filter_mode filter_mode) = 0;
  /// Returns the filter mode used for a sparse volume scene element (\c
  /// ISparse_volume_scene_element).
  virtual Sparse_volume_filter_mode get_filter_mode() const = 0;

  /// Enable or disable pre-integrated volume rendering for a sparse volume scene element (\c
  /// ISparse_volume_scene_element).
  /// Per default this is disabled.
  ///
  /// \param[in] enable           Enables the pre-integrated volume rendering technique
  ///                             if (\c true) otherwise the technique will be disabled (\c false).
  ///
  virtual void set_preintegrated_volume_rendering(bool enable) = 0;
  /// Returns if the pre-integrated volume rendering is enabled or disabled.
  virtual bool get_preintegrated_volume_rendering() const = 0;

  // to be removed! internal use only
  virtual void set_debug_visualization_option(mi::Uint32 o) = 0;
  virtual mi::Uint32 get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ISPARSE_VOLUME_RENDERING_PROPERTIES_H
