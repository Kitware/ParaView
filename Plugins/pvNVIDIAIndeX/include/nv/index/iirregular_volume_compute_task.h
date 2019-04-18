/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for user-defined compute tasks that apply to volume bricks.

#ifndef NVIDIA_INDEX_IIRREGULAR_VOLUME_COMPUTE_TASK_H
#define NVIDIA_INDEX_IIRREGULAR_VOLUME_COMPUTE_TASK_H

#include <mi/neuraylib/dice.h>

#include <nv/index/idistributed_data_subset.h>
#include <nv/index/iirregular_volume_subset.h>

namespace nv
{
namespace index
{

/// Interface class for a volume compute tasks operating on the voxel values of one irregular volume
/// data.
///
/// Passing an instance of an user-defined implementation of this class to the exposed interface
/// class \c IIrregular_volume_data_edit::edit() execute the compute task on the given volume data
/// that is stored locally on a cluster machine.
///
/// @ingroup nv_index_data_computing
class IIrregular_volume_compute_task : public mi::base::Interface_declare<0x71e7409, 0x77cb, 0x49c6,
                                         0xaa, 0x26, 0x94, 0x62, 0x59, 0xfe, 0xfa, 0x28>
{
public:
  /// Specifies which compute task method shall be invoked
  /// by the NVIDIA IndeX library when processing the irregular
  /// volume subset data.
  enum Operation_mode
  {
    /// Updating the scalar values attached at cell vertices.
    OPERATION_MODE_SCALAR_VALUE_EDITING = 1,
    /// Updating the positions and possibly scalar values at the
    /// cell vertices but not changing the volumes topology.
    OPERATION_MODE_VERTEX_EDITING = 2,
    /// Reset the irregular volume subset that is contained in the given
    /// 3D area or bounding box.
    OPERATION_MODE_TOPOLOGY_EDITING = 3
  };

  /// Perform a user-defined operation on the volume subset scalar/attribute  data.
  ///
  /// \param[in] bbox                 The bounding box that contains the present provided
  ///                                 irregular volume subset.
  ///
  /// \param[in,out] subset_data      Irregular volume subset.
  ///
  /// \param[in] dice_transaction     The current DiCE transaction.
  ///
  /// \return                         Shall return \c true if modifications have taken place
  ///                                 and \c false otherwise.
  ///                                 If it returns \c true, then the NVIDIA IndeX library
  ///                                 automatically updates the internal data representation of
  ///                                 the irregular volume for rendering.
  ///
  virtual bool edit(const mi::math::Bbox_struct<mi::Float32, 3>& bbox,
    IIrregular_volume_subset* subset_data,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;

  /// Create distributed volume subset data using user-defined operations.
  ///
  /// \param[in] bbox                 The bounding box that contains the present provided
  ///                                 irregular volume subset.
  ///
  /// \param[in] subset_data          Irregular volume subset.
  ///
  /// \param[in] factory              Factory class that creates irregular volume subset.
  ///
  /// \param[in] dice_transaction     The current DiCE transaction.
  ///
  /// \return                         Shall return the new irregular volume subset data that.
  ///                                 replaces the former one.
  ///
  virtual nv::index::IIrregular_volume_subset* edit(
    const mi::math::Bbox_struct<mi::Float32, 3>& bbox, const IIrregular_volume_subset* subset_data,
    nv::index::IData_subset_factory* factory,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;

  /// Specifies which compute task method the NVIDIA IndeX library triggers.
  ///
  /// \return                         Returns a mode that links to the above compute methods.
  ///
  virtual Operation_mode get_operation_mode() const = 0;
};

/// Mixin class for implementing the IIrregular_volume_compute_task interface.
///
/// This mixin class provides a default implementation of some of the pure
/// virtual methods of the IIrregular_volume_compute_task interface.
/// @ingroup nv_index_data_computing
class Irregular_volume_compute_task
  : public mi::base::Interface_implement<IIrregular_volume_compute_task>
{
public:
  /// Empty implementations
  virtual nv::index::IIrregular_volume_subset* edit(
    const mi::math::Bbox_struct<mi::Float32, 3>& /*bbox*/,
    const IIrregular_volume_subset* /*subset_data*/, nv::index::IData_subset_factory* /*factory*/,
    mi::neuraylib::IDice_transaction* /*dice_transaction*/) const
  {
    return 0;
  }
  using mi::base::Interface_implement<IIrregular_volume_compute_task>::edit;

  /// Apply an editing to the scalar values only.
  ///
  /// \return compute mode for the task.
  ///
  virtual Operation_mode get_operation_mode() const { return OPERATION_MODE_SCALAR_VALUE_EDITING; }
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IIRREGULAR_VOLUME_COMPUTE_TASK_H
