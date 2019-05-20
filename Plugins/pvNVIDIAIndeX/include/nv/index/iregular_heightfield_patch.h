/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Distributed subset (patch) of a regular heightfield dataset.

#ifndef NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_PATCH_H
#define NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_PATCH_H

#include <mi/dice.h>
#include <mi/math/vector.h>

#include <nv/index/idistributed_data_subset.h>

namespace nv
{
namespace index
{

/// Distributed data storage class for regular heightfield scene elements.
/// @ingroup nv_index_data_storage
///
class IRegular_heightfield_patch
  : public mi::base::Interface_declare<0xbf4f2ce, 0xc58d, 0x42d6, 0x89, 0x59, 0xb7, 0x82, 0x35,
      0x70, 0xe8, 0x0d, IDistributed_data_subset>
{
public:
  /// Initialize the patch data with the patch rectangle bounding
  /// box.  This doesn't allocate any data storage, therefore, the
  /// call of a \c generate_*_storage() method is needed for storage
  /// allocation. This method must be called only once and called
  /// before the \c generate_*_storage().
  ///
  /// \param[in] patch_rectangle The patch's rectangle bounding box
  ///
  /// \return true when initialization was successful
  ///
  virtual bool initialize(const mi::math::Bbox_struct<mi::Sint32, 2>& patch_rectangle) = 0;

  /// Initialize the patch data storage with the patch data.  This
  /// allocates the heightfield patch memory and initialize the
  /// height and normal value at once. The memory layout represents a
  /// 2D grid structure and is aligned in x-first and y-last order,
  /// were x refers to the patch's width in its local coordinate
  /// system and y to the patch's length.
  ///
  /// The method can be called only once to initialize the memory.
  ///
  /// \deprecated Shall only be used to be compatible with
  /// existing applications. To initialize the patch data, use the
  /// \c generate_*_storage() methods to get the data storage.
  /// Then, directly initialize the storage. In this way, the
  /// application can skip to operate on an application-side
  /// memory buffer, which can possibly improve the performance.
  ///
  /// \param patch_rectangle      The patch's rectangle bounding box
  /// \param elevation_values     An array for elevation values. Not
  ///                             initialized when 0. The array size must match
  ///                             with the patch_rectangle size suggesting.
  /// \param normal_values        An array for normal values. Not initialized when
  ///                             0. The array size must match with the
  ///                             patch_rectangle size suggesting.
  /// \return true when initialization succeeded
  ///
  virtual bool initialize(const mi::math::Bbox_struct<mi::Sint32, 2>& patch_rectangle,
    const mi::Float32* elevation_values,
    const mi::math::Vector_struct<mi::Float32, 3>* normal_values = 0) = 0;

  /// Allocate the elevation value data memory storage according to
  /// the bounding box.
  ///
  /// Prerequisite: call \c initialize(patch_rectangle).
  ///
  /// \return Allocated elevation memory storage pointer, 0 on failure.
  ///
  virtual mi::Float32* generate_elevation_storage() = 0;

  /// Allocate the normal memory storage according to the bounding
  /// box.
  ///
  /// Prerequisite: call \c initialize(patch_rectangle).
  ///
  /// \return Allocated normal memory storage pointer, 0 on failure.
  ///
  virtual mi::math::Vector_struct<mi::Float32, 3>* generate_normal_storage() = 0;

  /// Returns the continuous memory block that represents the elevation values of the
  /// heightfield's patch.
  ///
  /// \return Two-dimensional array of elevation values.
  ///
  virtual const mi::Float32* get_elevation_values() const = 0;

  /// Returns the number of grid points. This is the same as the number of elements in the the
  /// elevation value array size and the normal value array.
  ///
  /// \return Size of the elevation value array.
  ///
  virtual mi::Size get_nb_grid_points() const = 0;

  /// Returns the normal array associated with the elevation values
  /// in this heightfield patch.  Use the information from \c
  /// get_nb_grid_points() to get the number of elements in this array.
  ///
  /// \return Two-dimensional array of normal vectors.
  ///
  virtual const mi::math::Vector_struct<mi::Float32, 3>* get_normal_values() const = 0;

  /// The patch's extent in its local x, y, z.
  ///
  /// \return Bounding box of the patch.
  ///
  virtual const mi::math::Bbox_struct<mi::Sint32, 3>& get_bounding_box() const = 0;

  /// The patch's extent in its local x, and y.
  ///
  /// \return Bounding box of the patch.
  ///
  virtual const mi::math::Bbox_struct<mi::Sint32, 2>& get_bounding_rectangle() const = 0;

  /// Returns the height extent of this heightfield patch.  This
  /// contains the minimum and the maximum elevation values. Note:
  /// the elevation range is not the global, but only local for this
  /// patch.
  ///
  /// \return Minimum and maximum elevation value (K-value) in this heightfield patch.
  ///
  virtual const mi::math::Vector_struct<mi::Float32, 2>& get_height_range() const = 0;

  /// Returns the height value that encodes a hole in the heightfield data.
  ///
  /// \note Always use \c is_hole() to check for holes, do not compare height values with the
  /// return value of this function. Such a comparison would always fail if NaN is used, as
  /// comparing anything to NaN will always return false.
  ///
  /// \return Height value that represents holes.
  ///
  static inline mi::Float32 get_hole_value()
  {
    return mi::base::numeric_traits<mi::Float32>::quiet_NaN();
  }

  /// Returns whether the given height value represent a hole in the heightfield.
  ///
  /// \param[in] height Heightfield height value.
  ///
  /// \return True if the given height value represents a hole.
  ///
  static inline bool is_hole(mi::Float32 height)
  {
    // Use binary comparison so that it will work when holes are marked with NaN, as comparing
    // anything to NaN will always be false. This is also potentially faster than isnan().
    const mi::Uint32 height_bin = mi::base::binary_cast<mi::Uint32>(height);
    const mi::Uint32 hole_bin = mi::base::binary_cast<mi::Uint32>(get_hole_value());

    return height_bin == hole_bin;
  }
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_PATCH_H
