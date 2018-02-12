/******************************************************************************
 * Copyright 2018 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Distributed subsets of sparse volume datasets.

#ifndef NVIDIA_INDEX_ISPARSE_VOLUME_SUBSET_H
#define NVIDIA_INDEX_ISPARSE_VOLUME_SUBSET_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>
#include <mi/math/vector.h>

#include <nv/index/idistributed_data_subset.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_data_storage
/// Voxel format of sparse volume voxel data.
///
enum Sparse_volume_voxel_format
{
  SPARSE_VOLUME_VOXEL_FORMAT_UINT8 = 0x00, ///< Scalar voxel format with uint8 precision
  SPARSE_VOLUME_VOXEL_FORMAT_VEC2_UINT8,   ///< Vector voxel format with 2 components and uint8
                                           /// precision per component
  SPARSE_VOLUME_VOXEL_FORMAT_VEC3_UINT8,   ///< Vector voxel format with 3 components and uint8
                                           /// precision per component
  SPARSE_VOLUME_VOXEL_FORMAT_VEC4_UINT8,   ///< Vector voxel format with 4 components and uint8
                                           /// precision per component
  SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32,      ///< Scalar voxel format with float32 precision
  SPARSE_VOLUME_VOXEL_FORMAT_VEC2_FLOAT32, ///< Vector voxel format with 2 components and float32
                                           /// precision per component
  SPARSE_VOLUME_VOXEL_FORMAT_VEC3_FLOAT32, ///< Vector voxel format with 3 components and float32
                                           /// precision per component
  SPARSE_VOLUME_VOXEL_FORMAT_VEC4_FLOAT32, ///< Vector voxel format with 4 components and float32
                                           /// precision per component

  SPARSE_VOLUME_VOXEL_FORMAT_COUNT
};

/// @ingroup nv_index_data_storage
/// This class allows external input of a set of individual volume bricks associated with a sparse
/// volume dataset into NVIDIA IndeX. An instance of this class is created through an instance of
/// \c ISparse_volume_subset and is associated with an individual attribute of a potentially larger
/// dataset.
///
/// This class represents a pool storage for a set of volume bricks of a certain voxel type with
/// specific brick dimensions and brick ghost voxel-border size. The capacity of the brick pool is
/// fixed and specified at creation time.
///
class ISparse_volume_subset_brick_pool : public mi::base::Interface_declare<0x3a727d84, 0xeea4,
                                           0x41fc, 0x8d, 0x9f, 0x8d, 0xe2, 0x36, 0x84, 0xc2, 0xaa>
{
public:
  /// Returns the capacity of the pool. The capacity denotes the maximum number of bricks that can
  /// be inserted into the brick pool.
  ///
  virtual mi::Size get_capacity() const = 0;

  /// Returns the current size of the pool. The size denotes the actual number of bricks that are
  /// currently inserted in the brick pool.
  ///
  virtual mi::Size get_size() const = 0;

  /// Returns the voxel format of the brick pool.
  ///
  virtual Sparse_volume_voxel_format get_voxel_format() const = 0;

  /// Returns the dimensions of the bricks in the brick pool. The brick dimensions are including a
  /// potential border
  /// of ghost voxels around the actual brick data. The effective brick dimensions then is
  /// calculated by:
  /// * brick_dimensions - 2 * brick_overlap.
  ///
  virtual mi::math::Vector_struct<mi::Uint32, 3> get_brick_dimensions() const = 0;

  /// Returns the size of the optional brick border of ghost voxels around individual bricks.
  ///
  virtual mi::Uint32 get_brick_overlap() const = 0;

  /// Insert an attribute-volume brick into the brick pool.
  ///
  /// \param[in]      brick_position          Denotes the min-corner (lower-left-back) of the volume
  /// brick in global
  ///                                         voxel coordinates relative to the entire sparse volume
  ///                                         dataset.
  /// \param[in]      brick_data              Raw data pointer to the volume brick data. The caller
  /// retains the
  ///                                         ownership of this data pointer.
  ///
  /// \returns        Returns a valid brick index at which the volume brick is inserted into the
  /// brick pool. Valid
  ///                 indices fall in the range of 0 to the pool capacity - 1. An error during the
  ///                 insertion of a
  ///                 volume brick is indicated with the return value -1.
  ///
  virtual mi::Sint32 insert_brick(
    const mi::math::Vector_struct<mi::Sint32, 3>& brick_position, const void* brick_data) = 0;

  /// Returns a raw pointer to the internal volume brick pool storage array. This data pointer can
  /// be utilized to
  /// access the pool data for binary dumps of the entire pool contents.
  ///
  /// The memory size of the returned data array is equal to to pools capacity multiplied by the
  /// memory size of a single
  /// volume brick, which is calculated by the brick dimensions times the size of the current voxel
  /// format.
  ///
  /// \returns        Returns a raw pointer to the internal volume brick pool storage array.
  ///
  virtual const void* get_brick_pool_data() const = 0;
};

/// @ingroup nv_index_data_storage
/// Distributed data storage class for sparse volume subsets.
///
/// The data import for sparse volume data associated with \c ISparse_volume_scene_element instances
/// with
/// NVIDIA IndeX is performed through instances of this sub-set class. A sub-set of a sparse volume
/// is defined
/// by all the volume-data bricks associated with a rectangular subregion of the entire
/// scene/dataset. This
/// interface class provides methods to input volume data for one or multiple attributes of a
/// dataset. The
/// individual attributes are defined by separate brick pools, storing only the volume bricks
/// specific to their
/// associated attributes.
///
/// This interface class provides methods for generating attribute brick-pools and querying all
/// brick pools. A
/// brick pool represents storage for a specified number of bricks with specific brick dimensions,
/// brick ghost voxel-border size and brick format. The actual volume-brick data is input through
/// the
/// \c ISparse_volume_subset_brick_pool interface obtained through an instance of this interface
/// class.
///
class ISparse_volume_subset : public mi::base::Interface_declare<0x9b8a8982, 0x5215, 0x4933, 0x93,
                                0xc6, 0x31, 0x35, 0x2a, 0x37, 0xff, 0x7a, IDistributed_data_subset>
{
public:
  /// Generates an attribute brick-pool instance of type \c ISparse_volume_subset_brick_pool. The
  /// associated
  /// brick-pool instance can be queried through the \c get_attribute_brick_pool() method.
  ///
  /// \param[in]      brick_dimensions        Dimensions of the individual bricks in the requested
  /// brick pool.
  ///                                         The specification of the dimension is including an
  ///                                         optional border
  ///                                         of ghost voxels around the actual brick data. The
  ///                                         effective brick
  ///                                         dimensions then are calclulated as brick_dimensions -
  ///                                         2 * brick_overlap.
  /// \param[in]      brick_overlap           Size of an optional brick border of ghost voxels used
  /// for
  ///                                         interpolation purposes.
  /// \param[in]      brick_voxel_format      Voxel format of the bricks in the requested brick
  /// pool.
  /// \param[in]      pool_capacity           Requested capacity in number of volume bricks of the
  /// requested brick pool.
  /// \param[in]      initial_pool_data       An optional raw pointer to a data array containing the
  /// brick data
  ///                                         for initializing the brick pool. The ownership of the
  ///                                         data remains
  ///                                         with the caller.
  /// \param[in]      initial_pool_data_size  The memory size of the optional initialization data
  /// array. This size
  ///                                         should be a multiple of the size of an individual
  ///                                         volume brick and
  ///                                         smaller than the requested volume pool capacity.
  ///
  /// \returns        Returns valid attribute index (0-based) at which the the newly generated
  /// attribute
  ///                 brick pool is generated at. Failure at generating an appropriate brick pool is
  ///                 indicated by the return value -1. A valid attribute index can be used to
  ///                 acquire
  ///                 a pointer to the associated \c ISparse_volume_subset_brick_pool instance
  ///                 through
  ///                 the \c get_attribute_brick_pool() method.
  ///
  virtual mi::Sint32 generate_attribute_brick_pool(
    const mi::math::Vector_struct<mi::Uint32, 3>& brick_dimensions, mi::Uint32 brick_overlap,
    Sparse_volume_voxel_format brick_voxel_format, mi::Size pool_capacity,
    void* initial_pool_data = 0, mi::Size initial_pool_data_size = 0ull) = 0;

  /// Returns the current number active brick pools.
  ///
  /// \returns        The current number active brick pools.
  ///
  virtual mi::Sint32 get_nb_attribute_brick_pools() const = 0;

  /// This method allows to query an interface pointer to an \c ISparse_volume_subset_brick_pool
  /// instance
  /// for a valid attribute index. A valid attribute index is either directly obtained through the
  /// \c generate_attribute_brick_pool() method or lies in the range (0..N) where N is the current
  /// number
  /// of attribute brick pools returned by \c get_nb_attribute_brick_pools().
  ///
  /// \param[in]      attribute_index         Valid attribute index of the queried brick-pool
  /// instance.
  ///
  /// \returns        Valid interface pointer to an \c ISparse_volume_subset_input_brick_pool
  /// instance.
  ///                 The ownership of the returned interface pointer remains with the sparse volume
  ///                 sub-set instance and will only remain valid for its life time. In case of
  ///                 errors or
  ///                 a query for an invalid attribute index a null-pointer is returned.
  ///
  virtual ISparse_volume_subset_brick_pool* get_attribute_brick_pool(
    mi::Sint32 attribute_index) = 0;

  /// This method allows to write a compact cache file of successfully loaded subset data to the
  /// file system
  /// for more efficient future loading operations.
  virtual bool store_internal_data_representation(const char* output_filename) const = 0;
  virtual bool load_internal_data_representation(const char* input_filename) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ISPARSE_VOLUME_SUBSET_H
