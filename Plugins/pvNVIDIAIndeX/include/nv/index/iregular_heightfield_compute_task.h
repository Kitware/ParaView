/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for user-defined compute tasks that apply to heightfield patches.

#ifndef NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_COMPUTE_TASK_H
#define NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_COMPUTE_TASK_H

#include <mi/base/types.h>
#include <mi/dice.h>
#include <mi/math/color.h>
#include <mi/math/vector.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_data_computing

/// Interface class for heightfield compute tasks that operate on the elevation values of one
/// heightfield patch.
///
/// Passing an instance of an user-defined implementation of this class to the exposed interface
/// class \c IRegular_heightfield_data_edit::edit() executes the compute task on the heightfield
/// patch stored locally on a cluster machine.
///
class IRegular_heightfield_compute_task : public mi::base::Interface_declare<0xe236fc25, 0x8491,
                                            0x48fa, 0xb4, 0x0f, 0xfc, 0xad, 0xeb, 0x22, 0x51, 0x04>
{
public:
  /// The region of interest defines the bounding patch in which the compute
  /// operation shall take place. The region of interest is defined in the heightfield's
  /// IJ space. Based on the returned bounding patch, the distributed rendering and
  /// computing system can determine which heightfield patches need to be considered
  /// for the compute task.
  ///
  /// \param[out] roi     Returns the region of interest in which the heightfield
  ///                     modifications shall take place.
  ///
  virtual void get_region_of_interest_for_compute(
    mi::math::Bbox_struct<mi::Sint32, 2>& roi) const = 0;

  /// Apply user-defined operations on the heightfield dataset's elevation values.
  /// A re-compution of the normal values is not triggered automatically.
  ///
  /// \deprecated This will be replaced with compute_with_bounding_box in the future.
  ///
  /// \param[in] ij_patch_range       The range in which the elevation values of the
  ///                                 heightfield patch are defined. The patch range is given
  ///                                 in the heightfield's IJ space and includes an additional
  ///                                 patch boundary, which enables, for instance, rendering
  ///                                 interpolated normals.
  /// \param[in,out] elevation_values The elevation values defined in the given patch range.
  ///                                 The elevation values are given by a continuous array
  ///                                 whose layout corresponds to the given patch range
  ///                                 to efficiently process the values. That is, the
  ///                                 grid point of an elevation in the heightfield's IJ space
  ///                                 can be computed easily based on the array index and
  ///                                 the given patch range.
  ///                                 The array values may be changed to modify the
  ///                                 heightfield's elevation at the IJ grid points.
  /// \param[in] dice_transaction     The DiCE transaction to use for the operation
  ///
  /// \return                         Shall return \c true if modifications to the heightfield
  /// patch's
  ///                                 elevation values have taken place and \c false otherwise.
  ///
  virtual bool compute(const mi::math::Bbox_struct<mi::Sint32, 2>& ij_patch_range,
    mi::Float32* elevation_values, mi::neuraylib::IDice_transaction* dice_transaction) const = 0;

  /// In contrast to the interface method above, this method enables user-defined
  /// operations on the heightfield dataset's elevation as well as normal values.
  ///
  /// \deprecated This will be replaced with compute_with_bounding_box in the future.
  ///
  /// \param[in] ij_patch_range       The range in which the elevation values of the
  ///                                 heightfield patch are defined. The patch range is given
  ///                                 in the heightfield's IJ space and includes an additional
  ///                                 patch boundary, which enables, for instance, rendering
  ///                                 interpolated normals.
  /// \param[in,out] elevation_values The elevation values defined in the given patch range.
  ///                                 The elevation values are given by a continuous array
  ///                                 whose layout corresponds to the given patch range
  ///                                 to efficiently process the values. That is, the
  ///                                 grid point of an elevation in the heightfield's IJ space
  ///                                 can be computed easily based on the array index and
  ///                                 the given patch range.
  ///                                 The array values may be changed to modify the
  ///                                 heightfield's elevation at the IJ grid points.
  /// \param[in,out] normal_values    The normal values defined in the given patch range.
  ///                                 The normal values are given by a continuous array
  ///                                 whose layout corresponds to the given patch range
  ///                                 to efficiently process the values. That is, the
  ///                                 grid point of a normal in the heightfield's IJ space
  ///                                 can be computed easily based on the array index and
  ///                                 the given patch range.
  ///                                 The array values may be changed to modify the
  ///                                 heightfield's normal at the IJ grid points.
  /// \param[in] dice_transaction     The DiCE transaction to use for the operation
  ///
  /// \return                         Shall return \c true if modifications to the heightfield
  /// patch's
  ///                                 elevation values have taken place and \c false otherwise.
  ///
  virtual bool compute(const mi::math::Bbox_struct<mi::Sint32, 2>& ij_patch_range,
    mi::Float32* elevation_values, mi::math::Vector_struct<mi::Float32, 3>* normal_values,
    mi::neuraylib::IDice_transaction* dice_transaction) const
  {
    (void)ij_patch_range;   // avoid unused warnings
    (void)elevation_values; // avoid unused warnings
    (void)normal_values;    // avoid unused warnings
    (void)dice_transaction; // avoid unused warnings

    return false;
  }

  /// The compute tasks allows for user-defined elevation modifications for both
  /// user-defined elevation and user-defined normal modifications.
  /// The compute tasks need to inform the library which variant to choose
  ///
  /// \return             Tells the library which compute interface to use, i.e.,
  ///                     the \c compute() call that merely allows changing the elevation values
  ///                     or the \c compute() call that allows changing both the elevation and
  ///                     the normal values.
  ///
  virtual bool user_defined_normal_computation() const { return false; }
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_COMPUTE_TASK_H
