/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file   isparse_volume_compute_task.h
/// \brief  Compute tasks operating on subset data of sparse volumes.

#ifndef NVIDIA_INDEX_ISPARSE_VOLUME_COMPUTE_TASK_H
#define NVIDIA_INDEX_ISPARSE_VOLUME_COMPUTE_TASK_H

#include <mi/neuraylib/dice.h>

#include <nv/index/idistributed_data_edit.h>
#include <nv/index/isparse_volume_subset.h>

namespace nv
{
namespace index
{

/// Compute tasks operating on sparse-volume subsets.
///
/// An application can derive from the interface class \c ISparse_volume_compute_task 
/// to implement tailor-made data processing and analysis techniques.
/// Then passing an instance of the derived class to the  
/// \c IData_subset_compute_task_processing::execute_compute_task() method executes 
/// the callback \c compute(..) for each data subset of the sparse volume
/// that is stored locally, e.g., on the cluster machine.
///
/// \ingroup nv_index_data_edit
///
class ISparse_volume_compute_task :
    public mi::base::Interface_declare<0xc9f68e21,0xe3c6,0x44df,0xac,0xbb,0x11,0xf9,0xa1,0x8b,0xb7,0x97,IData_subset_processing_task>
{
public:
    /// Callback that performs a user-defined operations on distributed sparse-volume subset data.
    /// 
    /// The user-defined implementation of the \c compute(..) callback is invoked for each subset of 
    /// a distributed dataset. The implementation may either inspect or analyze the subset data or 
    /// perform operations and modifications to the data. The NVIDIA IndeX system ensure data 
    /// integrity when applying modifications. 
    ///  
    ///
    /// \param[in] subset_data_bbox     The 3D bounding box that contains the voxel values of the
    ///                                 sparse-volume subset. The bounding box is given
    ///                                 in the volume's local space.
    ///
    /// \param[in,out] subset_data      An instance of \c ISparse_volume_subset grants access
    ///                                 to the volume data defined in the given 3D bounding box.
    ///                                 The \c ISparse_volume_subset interface provides dedicated 
    ///                                 functionalities to query, access and edit specific to the
    ///                                 sparse volume data and its internal representations in
    ///                                 either host or CUDA device memory.
    ///
    /// \param[in] dice_transaction     The current DiCE transaction.
    ///
    /// \return                         Return \c true to signal the NVIDIA IndeX subsystem that 
    ///                                 modifications to the sparse-volume subset data have been 
    ///                                 applied and return \c false otherwise.
    ///                                 If modifications have been applied the NVIDIA IndeX subsystem
    ///                                 automatically updates all internal data structures and 
    ///                                 representation of the sparse volume on both the host and 
    ///                                 the CUDA device. 
    ///
    virtual bool compute(
        const mi::math::Bbox_struct<mi::Sint32, 3>& subset_data_bbox,
        ISparse_volume_subset*                      subset_data,
        mi::neuraylib::IDice_transaction*           dice_transaction) const = 0;

};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ISPARSE_VOLUME_COMPUTE_TASK_H
