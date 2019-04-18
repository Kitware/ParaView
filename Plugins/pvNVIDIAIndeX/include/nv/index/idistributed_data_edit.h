/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for user-defined editing operations on heightfields and volumes.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_DATA_EDIT_H
#define NVIDIA_INDEX_IDISTRIBUTED_DATA_EDIT_H

#include <mi/neuraylib/dice.h>

#include <nv/index/iirregular_volume_compute_task.h>
#include <nv/index/iregular_heightfield_compute_task.h>
#include <nv/index/iregular_volume_compute_task.h>
#include <nv/index/isparse_volume_compute_task.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_data_edit
/// This interface class represents an entry point for user-defined editing tasks that operate on
/// the regular volume distributed in the cluster. The editing operation modifies the amplitude
/// values
/// of the volume datasets.
///
/// An instance of this class can be retrieved from \c
/// IRegular_volume_data_locality::create_data_edit()
/// for a given subset of the regular volume. The computing task that actually performs the
/// user-defined volume editing operation needs to be implemented by a class derived from the
/// interface \c IRegular_volume_compute_task and passed to #execute_compute_task().
///
/// In order to be able to process the given the volume data, the calling distributed rendering
/// algorithm
/// (e.g., \c IDistributed_compute_algorithm) has to make sure to query the interface only for those
/// volume bricks that are available on the current cluster host where the compute unit
/// (fragment of the fragmented job) runs on, i.e., the volume's brick data needs to be
/// available locally on that machine.
///
class IRegular_volume_data_edit : public mi::base::Interface_declare<0xe096f971, 0xb9c1, 0x477f,
                                    0x9a, 0x86, 0x0c, 0xa9, 0x06, 0xdf, 0x39, 0x35>
{
public:
  /// Applies the given compute task on the current volume brick.
  ///
  /// \param[in] compute_task     The compute task performs the editing
  ///                             operation and can be any user-defined
  ///                             technique or algorithm.
  /// \param[in] dice_transaction The DiCE transaction used for the operation.
  ///
  virtual void execute_compute_task(const IRegular_volume_compute_task* compute_task,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;

  /// Returns on which device (if any) the data for the current subset of the
  /// regular volume is stored.
  ///
  /// \param[in] dice_transaction The DiCE transaction used for the operation.
  ///
  /// \return Number of the CUDA device, or a negative value if the data is not stored on any
  /// device.
  ///
  virtual mi::Sint32 get_assigned_device(
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;
};

/// @ingroup nv_index_data_edit
/// This interface class represents an entry point for user-defined editing tasks that operate on
/// sparse-volume data distributed in the cluster. The editing operation modifies the voxel values
/// of the volume datasets.
///
/// An instance of this class can be retrieved from \c
/// IRegular_volume_data_locality::retrieve_sparse_volume_data_locality()
/// for a given subset of the sparse volume. The computing task that actually performs the
/// user-defined volume editing operation needs to be implemented by a class derived from the
/// interface \c ISparse_volume_compute_task and passed to #execute_compute_task().
///
/// In order to be able to process the given the volume data, the calling distributed rendering
/// algorithm
/// (e.g., \c IDistributed_compute_algorithm) has to make sure to query the interface only for those
/// volume subsets that are available on the current cluster host where the compute unit
/// (fragment of the fragmented job) runs on, i.e., the volume's subset data needs to be
/// available locally on that machine.
///
class ISparse_volume_data_edit : public mi::base::Interface_declare<0xe976cf3, 0xedcd, 0x4318, 0x8e,
                                   0xa7, 0xb0, 0x84, 0x1, 0xab, 0x43, 0xf6>
{
public:
  /// Applies the given compute task on the current sparse-volume subset.
  ///
  /// \param[in] compute_task     The compute task performs the editing
  ///                             operation and can be any user-defined
  ///                             technique or algorithm.
  /// \param[in] dice_transaction The DiCE transaction used for the operation.
  ///
  virtual void execute_compute_task(const ISparse_volume_compute_task* compute_task,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;
};

/// @ingroup nv_index_data_edit
/// This interface class represents an entry point for user-defined editing tasks
/// that operate on the heightfield patches to modify their elevation values.
///
/// An instance of this class can be retrieved from \c
/// IRegular_heightfield_data_locality::create_data_edit()
/// for a given subset of the heightfield. The computing task that actually performs the
/// user-defined
/// heightfield editing needs to be implemented by a class derived from the interface \c
/// IRegular_heightfield_compute_task and passed to #edit().
///
/// In order to be able to process the given heightfield patch, the calling distributed rendering
/// algorithm (e.g., \c IDistributed_compute_algorithm) has to make sure to query the interface
/// only for those heightfield patches that are available on the present cluster host where the
/// compute
/// unit (fragment of the fragmented job) runs on, i.e., the heightfield's patch data needs to be
/// available locally on that machine.
///
class IRegular_heightfield_data_edit : public mi::base::Interface_declare<0x5b1758d2, 0x8527,
                                         0x4a2b, 0xa1, 0x55, 0x4f, 0xa6, 0x78, 0x2e, 0xc5, 0xfb>
{
public:
  /// Applies the given compute task on the current heightfield patch
  /// with a given bounding box. This invokes
  /// compute_with_bounding_box() method of compute_task.
  ///
  /// \param[in] compute_task     The compute task performs the editing
  ///                             operation and can be any user-defined
  ///                             technique or algorithm.
  /// \param[in] dice_transaction The DiCE transaction used for the operation.
  ///
  virtual void edit(IRegular_heightfield_compute_task* compute_task,
    mi::neuraylib::IDice_transaction* dice_transaction) = 0;

  /// Returns the updated bounding box of the heightfield patch data associated with
  /// the compute task. The updated bounding box shall be passed
  /// to the distributed compute algorithm (\c IDistributed_compute_algorithm),
  /// which can then trigger the data updates in the cluster environment.
  ///
  /// \param[out] bbox   The bounding box of the heightfield patch after running
  ///                    the compute algorithm. The bounding box is defined in the
  ///                    heightfield's IJK space.
  ///
  virtual void get_updated_bounding_box(mi::math::Bbox_struct<mi::Float32, 3>& bbox) = 0;
};

/// @ingroup nv_index_data_edit
/// This interface class represents an entry point for user-defined editing tasks that operate on
/// the irregular volume distributed in the cluster.
///
/// An instance of this class can be retrieved from \c
/// IIrregular_volume_data_locality::create_data_edit()
/// for a given subset of the irregular volume subsets. The computing task that actually performs
/// the
/// user-defined volume editing operation needs to be implemented by a class derived from the
/// interface \c IIrregular_volume_compute_task and passed to #execute_compute_task().
///
/// In order to be able to process the given the volume data, the calling distributed rendering
/// algorithm
/// (e.g., \c IDistributed_compute_algorithm) has to make sure to query the interface only for those
/// irregular
/// volume subsets that are available on the current cluster host where the compute unit
/// (fragment of the fragmented job) runs on, i.e., the volume's data needs to be available locally
/// on that machine.
///
class IIrregular_volume_data_edit : public mi::base::Interface_declare<0xb4baa9c9, 0x1219, 0x422f,
                                      0x8e, 0x22, 0xad, 0x05, 0x8d, 0xba, 0x9d, 0xb5>
{
public:
  /// Applies the given compute task on the current irregular volume data.
  ///
  /// \param[in] compute_task     The compute task performs the editing
  ///                             operation and can be any user-defined
  ///                             technique or algorithm.
  /// \param[in] dice_transaction The DiCE transaction used for the operation.
  ///
  virtual void execute_compute_task(const IIrregular_volume_compute_task* compute_task,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_DATA_EDIT_H
