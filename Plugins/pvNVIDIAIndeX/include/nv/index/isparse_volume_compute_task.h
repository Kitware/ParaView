/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for user-defined compute tasks that apply to sparse-volume subset data.

#ifndef NVIDIA_INDEX_ISPARSE_VOLUME_COMPUTE_TASK_H
#define NVIDIA_INDEX_ISPARSE_VOLUME_COMPUTE_TASK_H

#include <mi/neuraylib/dice.h>

#include <nv/index/isparse_volume_subset.h>

namespace nv
{
namespace index
{

/// Interface class for a volume compute tasks operating on a sparse-volume subset.
///
/// Passing an instance of a user-defined implementation of this interface to
/// \c ISparse_volume_data_edit::execute_compute_task() executes the compute task on the
/// sparse-volume subset data that is stored locally on a cluster machine.
///
/// @ingroup nv_index_data_computing
class ISparse_volume_compute_task : public mi::base::Interface_declare<0xc9f68e21, 0xe3c6, 0x44df,
                                      0xac, 0xbb, 0x11, 0xf9, 0xa1, 0x8b, 0xb7, 0x97>
{
public:
  /// Perform a user-defined operation on the distributed sparse-volume data.
  ///
  /// \param[in] subset_data_bbox     The 3D bounding box that contains the voxel values of the
  ///                                 sparse-volume subset. The bounding box is given
  ///                                 in the volume's local space.
  ///
  /// \param[in,out] subset_data      An instance of \c ISparse_volume_subset_P9 granting access
  ///                                 to the volume data defined in the given 3D bounding box.
  ///
  /// \param[in] dice_transaction     The current DiCE transaction.
  ///
  /// \return                         Shall return \c true if modifications to the sparse-volume's
  ///                                 voxel values have taken place and \c false otherwise.
  ///                                 If it returns \c true, then the NVIDIA IndeX library
  ///                                 automatically updates the internal data representation of
  ///                                 the volume used for rendering.
  ///
  virtual bool compute(const mi::math::Bbox_struct<mi::Sint32, 3>& subset_data_bbox,
    ISparse_volume_subset* subset_data,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ISPARSE_VOLUME_COMPUTE_TASK_H
