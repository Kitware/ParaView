/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for user-defined compute tasks that apply to volume bricks.

#ifndef NVIDIA_INDEX_IREGULAR_VOLUME_COMPUTE_TASK_H
#define NVIDIA_INDEX_IREGULAR_VOLUME_COMPUTE_TASK_H

#include <mi/neuraylib/dice.h>

#include <nv/index/iregular_volume_data.h>

// Forward declaration, defined by driver_types.h in the CUDA SDK
struct cudaArray;

namespace nv
{
namespace index
{

/// Interface class for a volume compute tasks operating on the voxel values of one regular volume brick.
///
/// Passing an instance of an user-defined implementation of this class to the exposed interface
/// class \c IRegular_volume_data_edit::edit() execute the compute task on the given volume brick
/// that is stored locally on a cluster machine.
///
/// @ingroup nv_index_data_computing
class IRegular_volume_compute_task :
    public mi::base::Interface_declare<0x5320df33,0x4d84,0x4ee1,0x9f,0xab,0x57,0x67,0xaa,0xe8,0x12,0xc5>
{
public:
    /// Specifies how the compute task should be executed.
    enum Compute_mode
    {
        /// Execute on CPU
        COMPUTE_MODE_CPU         = 1,
        /// Execute on GPU
        COMPUTE_MODE_GPU         = 2
    };

    /// Perform a user-defined operation on the distributed volume bricks.
    ///
    /// \param[in] brick_bbox           The 3D bounding box that contains the voxel values of the
    ///                                 regular volume brick. The bounding box is given
    ///                                 in the volume's local space and includes the additional
    ///                                 voxel boundary, which is used, for instance, for efficient
    ///                                 tri-linear filtering.
    ///
    /// \param[in,out] voxel_data       An instance of \c IRegular_volume_data granting typed access
    ///                                 to the voxel values defined in the given 3D bounding box.
    ///                                 The voxel values are given by a continuous array. The 
    ///                                 array's layout corresponds to the given bounding box
    ///                                 to efficiently process the values. That is, the
    ///                                 voxel position of an voxel values in the regular
    ///                                 volume's local space can be computed
    ///                                 based on the array index and the given
    ///                                 bounding box.
    ///                                 The array values may be changed to modify the
    ///                                 volume's voxels at the 3D voxel positions.
    ///
    /// \param[in] dice_transaction     The current DiCE transaction.
    ///
    /// \return                         Shall return \c true if modifications to the regular volume's
    ///                                 amplitude values have taken place and \c false otherwise.
    ///                                 If it returns \c true, then the NVIDIA IndeX library
    ///                                 automatically updates the internal data representation of
    ///                                 the volume used for rendering.
    ///
    virtual bool compute(
        const mi::math::Bbox_struct<mi::Uint32, 3>& brick_bbox,
        IRegular_volume_data*                       voxel_data,
        mi::neuraylib::IDice_transaction*           dice_transaction) const = 0;

    /// Perform a user-defined operation on the distributed volume bricks in
    /// CUDA device memory. In most cases this will mean starting a CUDA kernel
    /// and processing given CUDA array.
    ///
    /// \see IRegular_volume_compute_task::compute()
    ///
    /// \param[in] brick_bbox           The 3D bounding box that contains the voxel values of the
    ///                                 regular volume brick.
    ///
    /// \param[in,out] voxel_data       A CUDA array that represent he voxel values
    ///                                 defined in the given 3D bounding box as
    ///                                 stored on the device.
    ///
    /// \param[in] dice_transaction     The current DiCE transaction.
    ///
    /// \return                         Shall return \c true if modifications to the regular volume's
    ///                                 amplitude values have taken place and \c false otherwise.
    ///
    virtual bool compute_gpu(
        const mi::math::Bbox_struct<mi::Uint32, 3>& brick_bbox,
        cudaArray*                                  voxel_data,
        mi::neuraylib::IDice_transaction*           dice_transaction) const = 0;

    /// Specifies how the compute task should be executed.
    ///
    /// \return compute mode for the task.
    ///
    virtual Compute_mode get_compute_mode() const = 0;
};

/// Mixin class for implementing the IRegular_volume_compute_task interface.
///
/// This mixin class provides a default implementation of some of the pure
/// virtual methods of the IRegular_volume_compute_task interface.
/// @ingroup nv_index_data_computing
class Regular_volume_compute_task :
    public mi::base::Interface_implement<IRegular_volume_compute_task>
{
public:
    /// Dummy implementation of GPU compute.
    ///
    /// \param[in] brick_bbox           The 3D bounding box that contains the voxel values of the
    ///                                 regular volume brick.
    ///
    /// \param[in,out] voxel_values     A CUDA array that represent he voxel values
    ///                                 defined in the given 3D bounding box as
    ///                                 stored on the device.
    ///
    /// \param[in] dice_transaction     The current DiCE transaction.
    ///
    /// \return                         false, as no data is modified.
    ///
    virtual bool compute_gpu(
        const mi::math::Bbox_struct<mi::Uint32, 3>& brick_bbox,
        cudaArray*                                  voxel_values,
        mi::neuraylib::IDice_transaction*           dice_transaction) const
    {
        (void)brick_bbox;       // avoid unused warnings
        (void)voxel_values;     // avoid unused warnings
        (void)dice_transaction; // avoid unused warnings

        return false;
    }

    /// Execute compute only on the CPU.
    ///
    /// \return compute mode for the task.
    ///
    virtual Compute_mode get_compute_mode() const
    {
        return COMPUTE_MODE_CPU;
    }
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IREGULAR_VOLUME_COMPUTE_TASK_H
