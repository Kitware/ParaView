/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file   idistributed_data_edit.h
/// \brief  Interfaces for distributed data processing.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_DATA_PROCESSING_H
#define NVIDIA_INDEX_IDISTRIBUTED_DATA_PROCESSING_H

#include <mi/base/types.h>
#include <mi/math/vector.h>
#include <mi/math/bbox.h>

#include <mi/neuraylib/dice.h>

namespace nv
{
namespace index
{

/// Base interface for distributed datatype specific subset data processing.
///
/// The interface class \c IData_subset_processing_task serves as a base class for 
/// distributed datatype specific interfaces such as \c ISparse_volume_compute_task
/// or \c IIrregular_volume_compute_task, which operate on sparse volume or irregular
/// volume data subsets.
///
/// An instance of derived classes can be passed to the interface class 
/// \c IData_subset_compute_task_processing using the interface method
/// \c IData_subset_compute_task_processing::execute_compute_task(). Once passed 
/// the compute task's implementation is executed on a per-subset data granularity. 
/// In general, a \c IData_subset_processing_task can be considered a SIMD 
/// (single instruction, multiple data) operation exploiting data subset parallism. 
/// That is, the actual implementation of a derived class can be applied to each an 
/// every subset of a distributed dataset in parallel and in that way resembles the 
/// execution of CUDA kernels. Depending on whether a subset data representation
/// (see \c IDistributed_data_subset) exposes the repective interface methods,
/// CUDA-based and/or CPU-based programming can be used. 
/// 
/// Common applications that can be implemented using the \c IData_subset_processing_task 
/// include the in-situ and in-transit algorithms that combine data simulation
/// and data visualization or parallel data analysis tasks such as the generation
/// of volume histogram. 
/// 
/// \ingroup nv_index_data_edit
///
class IData_subset_processing_task : 
    public mi::base::Interface_declare<0x4b47717d,0x4860,0x4340,0x90,0x53,0xb4,0x68,0x2b,0x00,0x50,0x30>
{
public:
};

/// Receives user-implemented compute tasks and applies them to distributed data subsets.
///  
/// The \c IData_subset_compute_task_processing interface class receives an user-implemented 
/// compute tasks through #execute_compute_task() method and schedules its execution against a
/// distributed data subset. 
/// Internally, the NVIDIA IndeX system makes the subset data resources available and invokes the 
/// the compute task's compute callback for processing it. Once the compute task finished the  
/// distributed datatype specific operations, it typically signals 
/// the NVIDIA IndeX system if modifications have been applied to the subset data. If  
/// modifications occured then the NVIDIA IndeX system update the internal structures and resources
/// accordingly.
/// 
/// The an instance of a \c IData_subset_compute_task_processing implementation is typically
/// exposed through the \c IDistributed_data_job interface. Implemented distribtued data job run 
/// against an distributed dataset that is uploaded to the GPU cluster. The job spans a thread or 
/// execution for each data subset of the dataset. Here, the subset data is either stored locally
/// and is then processed by the method \c IDistributed_data_job::execute_subset or stored on a remote 
/// cluster machine and is then processed  by the method \c IDistributed_data_job::execute_subset_remote.
/// With either method, the job exposes an \c IData_subset_compute_task_processing interface
/// so that an application can apply its specific \c IData_subset_processing_task implementation.
///
/// \note   The \c IData_subset_compute_task_processing will be extended in the future 
///         to provide additional information besides the ability to invoke user-implemented 
///         compute tasks.
///
/// \ingroup nv_index_data_edit
///
class IData_subset_compute_task_processing : 
    public mi::base::Interface_declare<0x27ff87c3,0xacc3,0x4466,0x8b,0x0c,0x61,0x39,0xe8,0x59,0x61,0xb6>
{
public:
    /// Applies the passed user-implemented compute task to a subset of the distribute dataset.
    /// The execution will make the subset data available and invoke the 
    /// methods of the compute task using the subset data representation as input parameter.
    ///
    /// \param[in] compute_task     The compute task performs the editing
    ///                             operation and can be any user-defined
    ///                             technique or algorithm.
    ///
    /// \param[in] dice_transaction The DiCE transaction used for the operation.
    ///
    virtual void execute_compute_task(
        const IData_subset_processing_task* compute_task,
        mi::neuraylib::IDice_transaction*   dice_transaction) const = 0;
};


/// Heightfield data specific compute tasks that operate on the values of a heightfield's patch.
///
/// Derived implementations of the interface class \c IRegular_heightfield_compute_task can be applied 
/// to the \c IRegular_heightfield scene elements. Derived class need to implement the interface methods 
/// #compute() to operate on elevation and normal values of the height field,
/// #user_defined_normal_computation to tell the NVIDIA INdeX system if normal values have been modified,
/// and #get_region_of_interest_for_compute to define the 2D area of interest that the compute task
/// may operate in.
///
/// Passing an instance of an user-defined implementation of this class to the interface
/// class \c IRegular_heightfield_data_edit::edit() executes the compute task on the heightfield
/// patch stored locally on a cluster machine.
///
/// \deprecated Once the former \c IRegular_heightfield is replaced by the newer tile 
///             based \c IHeight_field_scene_element, then this class becomes obsolete and 
///             will be removed from the NVIDIA IndeX API.
///
/// \ingroup nv_index_data_edit
///
class IRegular_heightfield_compute_task :
    public mi::base::Interface_declare<0xe236fc25,0x8491,0x48fa,0xb4,0x0f,0xfc,0xad,0xeb,0x22,0x51,0x04,IData_subset_processing_task>
{
public:
    /// The region of interest defines the 2D area in which the compute operation shall take place. 
    /// The region of interest is defined in the height field's
    /// local space. Based on the returned bounding patch, the NVIDIA IndeX system
    /// can determine which heightfield patches need to be considered
    /// by the compute task.
    ///
    /// \param[out] roi     Returns the region of interest in which the heightfield
    ///                     modifications shall take place. The roi is defined in the
    ///                     height field's local space.
    ///
    virtual void get_region_of_interest_for_compute(
        mi::math::Bbox_struct<mi::Sint32, 2>& roi) const = 0;

    /// Apply user-defined operations on the heightfield data set's elevation values.
    /// A re-computation of the normal values is not triggered automatically.
    ///
    /// \param[in] ij_patch_range       The range in which the elevation values of the
    ///                                 heightfield patch are defined. The patch range is given
    ///                                 in the height field's local space and includes an additional
    ///                                 patch boundary, which enables, for instance, rendering
    ///                                 interpolated normals.
    /// \param[in,out] elevation_values The elevation values defined in the given patch range.
    ///                                 The elevation values are given by a continuous array
    ///                                 whose layout corresponds to the given patch range
    ///                                 to efficiently process the values. That is, the
    ///                                 grid point of an elevation in the height field's local space
    ///                                 can be computed easily based on the array index and
    ///                                 the given patch range.
    ///                                 The array values may be changed to modify the
    ///                                 height field's elevation at the IJ grid points.
    /// \param[in] dice_transaction     The DiCE transaction to use for the operation.
    ///
    /// \return                         Shall return \c true if modifications to the height field's
    ///                                 elevation values have been applied and \c false otherwise.
    ///
    virtual bool compute(
        const mi::math::Bbox_struct<mi::Sint32, 2>&     ij_patch_range,
        mi::Float32*                                    elevation_values,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// This compute method enables user-defined operations on both the height field's 'elevation and the normal values.
    ///
    ///
    ///
    /// \param[in] ij_patch_range       The range in which the elevation values of the
    ///                                 heightfield patch are defined. The patch range is given
    ///                                 in the height field's local space and includes an additional
    ///                                 patch boundary, which enables, for instance, rendering
    ///                                 interpolated normals.
    /// \param[in,out] elevation_values The elevation values defined in the given patch range.
    ///                                 The elevation values are given by a continuous array
    ///                                 whose layout corresponds to the given patch range
    ///                                 to efficiently process the values. That is, the
    ///                                 grid point of an elevation in the height field's local space
    ///                                 can be computed easily based on the array index and
    ///                                 the given patch range.
    ///                                 The array values may be changed to modify the
    ///                                 heightfield's elevation at the IJ grid points.
    /// \param[in,out] normal_values    The normal values defined in the given patch range.
    ///                                 The normal values are given by a continuous array
    ///                                 whose layout corresponds to the given patch range
    ///                                 to efficiently process the values. That is, the
    ///                                 grid point of a normal in the height field's local space
    ///                                 can be computed easily based on the array index and
    ///                                 the given patch range.
    ///                                 The array values may be changed to modify the
    ///                                 heightfield's normal at the ij grid points.
    /// \param[in] dice_transaction     The DiCE transaction to use for the operation.
    ///
    /// \return                         Shall return \c true if modifications to the heightfield patch's
    ///                                 elevation values have been applied and \c false otherwise.
    ///
    virtual bool compute(
        const mi::math::Bbox_struct<mi::Sint32, 2>&     ij_patch_range,
        mi::Float32*                                    elevation_values,
        mi::math::Vector_struct<mi::Float32, 3>*        normal_values,
        mi::neuraylib::IDice_transaction*               dice_transaction) const
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

///  
///
/// This interface class represents an entry point for user-defined editing tasks
/// that operate on the heightfield patches to modify their elevation values.
///
/// An instance of this class can be retrieved from \c IRegular_heightfield_data_locality::create_data_edit()
/// for a given subset of the heightfield. The computing task that actually performs the user-defined
/// heightfield editing needs to be implemented by a class derived from the interface \c
/// IRegular_heightfield_compute_task and passed to #edit().
///
/// In order to be able to process the given heightfield patch, the calling distributed rendering
/// algorithm (e.g., \c IDistributed_compute_algorithm) has to make sure to query the interface
/// only for those heightfield patches that are available on the present cluster host where the compute
/// unit (fragment of the fragmented job) runs on, i.e., the height field's patch data needs to be
/// available locally on that machine.
///
/// \deprecated Once the former \c IRegular_heightfield is replaced by the newer tile 
///             based \c IHeight_field_scene_element, then this class becomes obsolete and 
///             will be removed from the NVIDIA IndeX API.
///
/// \ingroup nv_index_data_edit
///
class IRegular_heightfield_data_edit :
    public mi::base::Interface_declare<0x5b1758d2,0x8527,0x4a2b,0xa1,0x55,0x4f,0xa6,0x78,0x2e,0xc5,0xfb,IData_subset_compute_task_processing>
{
public:
    /// Applies the given compute task on the current heightfield patch with a given bounding box.
    ///
    /// \param[in] compute_task     The height field compute task performs the editing
    ///                             operation and can be any user-defined
    ///                             technique or algorithm.
    /// \param[in] dice_transaction The DiCE transaction used for the operation.
    ///
    virtual void edit(
        IRegular_heightfield_compute_task*  compute_task,
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;

    /// Returns the updated bounding box of the heightfield patch data associated with the compute task.
    /// The updated bounding box shall be passed to the NVIDIA IndeX system to trigger,
    /// the data updates in the cluster environment if needed.
    ///
    /// \param[out] bbox   The bounding box of the heightfield patch after running
    ///                    the compute algorithm. The bounding box is defined in the
    ///                    height field's IJK space.
    ///
    virtual void get_updated_bounding_box(
        mi::math::Bbox_struct<mi::Float32, 3>& bbox) = 0;

    /// Applies the given compute task on the current sparse-volume subset.
    ///
    /// \param[in] compute_task     The height field compute task performs the
    ///                             operation and can be any user-defined
    ///                             technique or algorithm.
    /// \param[in] dice_transaction The DiCE transaction used for the operation.
    ///
    virtual void execute_compute_task(
        const IData_subset_processing_task* compute_task,
        mi::neuraylib::IDice_transaction*   dice_transaction) const = 0;
};


}} // namespace index / nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_DATA_PROCESSING_H
