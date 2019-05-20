/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// Sub-grids of a corner-point grid.

#ifndef NVIDIA_INDEX_ICORNER_POINT_GRID_SUBSET_H
#define NVIDIA_INDEX_ICORNER_POINT_GRID_SUBSET_H

#include <nv/index/idistributed_data_subset.h>

namespace nv
{
namespace index
{

/// Defines the vertices and attributes of a subset (sub-grid) of a corner-point grid. The part of
/// each layer that intersects with a subregion is further subdivided into <em>patches</em>, each
/// having the same xy-size. Patches from different layers that have the same xy-coordinates are
/// organized as a <em>patch stack</em>.
///
/// @ingroup nv_index_data_storage
class ICorner_point_grid_subset
  : public mi::base::Interface_declare<0x46b85bee, 0xca99, 0x4edf, 0x85, 0x1a, 0x8b, 0x9d, 0xbd,
      0x4c, 0x96, 0xd5, IDistributed_data_subset>
{
public:
  /// Returns the bounding box of the subset.
  ///
  /// \return Bounding box in the local coordinate system.
  ///
  virtual const mi::math::Bbox_struct<mi::Float32, 3>& get_bounding_box() const = 0;

  /// Returns the general (non-clipped) size of the patches into which each layer in this subset
  /// is further subdivided.
  ///
  /// \return xy-size of a patch.
  ///
  virtual const mi::math::Vector_struct<mi::Uint32, 2> get_patch_size() const = 0;

  /// Returns the number of patch stacks available in the subset.
  ///
  virtual mi::Uint32 get_number_of_patch_stacks() const = 0;

  /// Returns the xy-position of the selected patch stack.
  ///
  /// \param[in] stack_index  Index of the patch stack for which to return the information,
  ///                         must be less than \c get_number_of_patch_stacks().
  ///
  /// \returns xy-position of the patch stack.
  ///
  virtual mi::math::Vector_struct<mi::Sint32, 2> get_patch_stack_position(
    mi::Uint32 stack_index) const = 0;

  virtual bool add_empty_layer(int layer) const = 0;

  virtual mi::Float32* add_patch(mi::Uint32 layer, mi::Uint32 stack_index,
    const mi::math::Vector_struct<mi::Uint32, 2>& patch_size) const = 0;

  virtual mi::Float32* add_patch_data(
    mi::Uint32 layer, mi::Uint32 stack_index, mi::Uint32 property_id) const = 0;

  virtual bool add_empty_patch(mi::Uint32 layer, mi::Uint32 stack_index) const = 0;
};
}
} // namespace

#endif // NVIDIA_INDEX_ICORNER_POINT_GRID_SUBSET_H
