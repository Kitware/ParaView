/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief The interface class representing a distributed brick of the entire large-scale regular
/// volume dataset.

#ifndef NVIDIA_INDEX_IREGULAR_VOLUME_BRICK_H
#define NVIDIA_INDEX_IREGULAR_VOLUME_BRICK_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data_subset.h>

namespace nv
{
namespace index
{

/// The interface class \c IDistributed_data_subset represents the super class for a smaller subset
/// of a large-scale
/// dataset. The distributed rendering and computing environment decomposes the entire space into
/// smaller sized
/// subregions. Each of these subregions contain only a fraction of the entire dataset uploaded to
/// NVIDIA IndeX.
/// Typically, a fragment of a regular volume is called a volume brick or a fragment of a regular
/// heightfield is
/// called a patch. The data import callback (\c IDistributed_data_import_callback) is responsible
/// for creating
/// and loading/generating the contents of the data subset.
/// @ingroup nv_index_data_storage
template <typename T>
class IRegular_volume_brick : public mi::base::Interface_declare<0x92b66c73, 0x6141, 0x4c5d, 0xa6,
                                0xb1, 0xfd, 0x9f, 0xdb, 0x41, 0xd2, 0x26, IDistributed_data_subset>
{
public:
  typedef T Voxel_type;

public:
  /// Allocate the voxel data memory storage according to the bounding box.
  /// Prerequisite: never called initialize() yet. Can be called only once.
  ///
  /// \param[in] bounding_box     The bounding box, i.e., the size, for the volume allocation.
  //
  /// \return                     Returns the allocated memory pointer and returns null when failed.
  //
  virtual T* generate_voxel_storage(const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box) = 0;

  /// Returns the continuous memory block that represents the brick. The brick data is stored as
  /// according to
  /// the datatype T.
  /// The size of memory allocated for the voxel data corresponds to the brick's bounding box size
  /// plus the extra
  /// voxel boundary at each brick side and the datatype.
  ///
  /// \return     Returns a pointer to the volume brick's raw voxel data that is aligned in
  ///             x-first and z-last order and includes the extra voxel boundary (in case there is
  ///             one).
  ///
  virtual const T* get_voxels() const = 0;

  /// The brick's width, height, and depth extent in x, y, and z direction in the brick's
  /// local coordinate system.
  ///
  /// \return     The bounding box of the brick. The bounding box does not include the extra voxel
  /// layer.
  ///
  virtual const mi::math::Bbox_struct<mi::Sint32, 3>& get_bounding_box() const = 0;

  // ---------------------------------------------------------------------------------------------------------------

  /// Allocate the volume brick's memory. The brick's memory layout represents a 3D grid structure
  /// and is aligned in
  /// x-first and z-last order, were x refers to the brick's width in its local coordinate system
  /// and z to the
  /// the brick's depth.
  /// The size of the continuous memory block relies on the brick's width, height, and depth, the
  /// number of
  /// voxel layers that extend the brick as well as the voxel's data type (e.g., 8-bit).
  ///
  /// The method can be called only once to initialize the memory.
  ///
  /// This method is a convenience method to execute two things:
  /// 1. allocate_data(), 2. copy the voxels data into this brick.
  ///
  /// \deprecated Shall only be used to be compatible with
  ///             existing applications. To initialize the brick data, use the
  ///             \c generate_voxel_storage() method to get the data storage.
  ///             Then, directly initialize the storage. In this way, the
  ///             application can skip to operate on an application-side memory
  ///             buffer, which can possibly improve the performance.
  ///
  /// \param bounding_box   The brick's width, height, and depth in x, y, and z direction in the
  /// brick's
  ///                       local coordinate system.
  /// \param voxels         The number of voxel layers extending the brick data in each of the three
  /// dimensions.
  ///
  virtual bool initialize(
    const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box, const T* voxels) = 0;
};

/// @ingroup nv_index_data_storage
class IRegular_volume_brick_uint8
  : public mi::base::Interface_declare<0x99f2ea96, 0x71fa, 0x4f42, 0xbf, 0x2c, 0xbb, 0xe8, 0x1e,
      0xd4, 0xf8, 0x79, IRegular_volume_brick<mi::Uint8> >
{
};

/// @ingroup nv_index_data_storage
class IRegular_volume_brick_uint16
  : public mi::base::Interface_declare<0x51419327, 0x49fc, 0x4a14, 0x9b, 0x79, 0x27, 0x13, 0xba,
      0x55, 0x48, 0x5a, IRegular_volume_brick<mi::Uint16> >
{
};

/// @ingroup nv_index_data_storage
class IRegular_volume_brick_float32
  : public mi::base::Interface_declare<0x5584361d, 0x30f8, 0x4942, 0x87, 0x38, 0x48, 0x5e, 0x72,
      0xc1, 0x28, 0xe9, IRegular_volume_brick<mi::Float32> >
{
};

/// @ingroup nv_index_data_storage
class IRegular_volume_brick_rgba8ui
  : public mi::base::Interface_declare<0xbdcd313b, 0x3da9, 0x4474, 0xb9, 0xa3, 0x77, 0x71, 0x1b,
      0xf0, 0xac, 0xab, IRegular_volume_brick<mi::math::Vector_struct<mi::Uint8, 4> > >
{
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IREGULAR_VOLUME_BRICK_H
