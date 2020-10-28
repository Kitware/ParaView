/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file  idata_distribution.h
/// \brief Interfaces for implementing and launching distributed data jobs.

#ifndef NVIDIA_INDEX_IDATA_DISTRIBUTION_H
#define NVIDIA_INDEX_IDATA_DISTRIBUTION_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/neuraylib/iserializer.h>

#include <nv/index/idistributed_data_edit.h>
#include <nv/index/idistributed_data_locality.h>

namespace nv
{
namespace index
{

/// Distributed job interface enabling analysis and processing running against distributed subset data.
///
/// The \c IDistributed_data_job interface for implementing custom jobs resembles the
/// \c mi::neuraylib::IFragment_job interface on purpose. While DiCE's \c mi::neuraylib::IFragment_job
/// interface is meant to implement general cluster wide jobs, e.g., compute algorithms,
/// the \c IDistributed_data_job interface enables applications to implement algorithms
/// specificly for NVIDIA IndeX's distributed data. Here, the scheduling is based on the 
/// data locality, i.e., which cluster node stores which portion of the dataset. 
/// The job will then send out one execution thread per subset data so that all data subsets 
/// can be processed in parallel either locally or remotely. For local execution, the 
/// method #execute_subset is invoked to processes a single subset data and for remote execution, 
/// the method #execute_subset_remote is invoked. The method #execute_subset_remote has its
/// local counterpart #receive_subset_result in case the distributed data processing analyses
/// the data and derives information from the data that need to be assembled locally.
/// A common use-case for that is the histogram genereration.
///
/// Technically, the DiCE \c mi::neuraylib::Fragmented_job infrastructure is used to facilitate the   
/// distributed data analysis and processing. While the processing of subset data could still be 
/// implemented using the \c mi::neuraylib::Fragmented_job interface and, for instance, the 
/// information provided by the data locallity (see \c IDistributed_data_locality), the use of 
/// the \c IDistributed_data_job interface reduces the code complexity drastically and provides
/// immediate access to the data subset through \c IData_subset_processing_task and
/// \c IData_subset_compute_task_processing interfaces.
///
/// \ingroup nv_index_data_edit
///
class IDistributed_data_job :
    public mi::base::Interface_declare<0x841224fe,0x3c17,0x43b3,0xaa,0xc8,0xbc,0x32,0x72,0xc8,0x44,0xf1,
                                       mi::neuraylib::ISerializable>
{
public:
    /// A data locality query mode specifies which data subsets to process.
    /// Implementations of the class \c \c IDistributed_data_locality_query_mode 
    /// specify which distributed data scene element to process and typically also 
    /// provide additional query parameter such as the region of interest in 3D in
    /// case of a volume or a 2D in case of a height field.
    /// The query mode lets the NVIDIA IndeX system determine the cluster nodes and
    /// GPUs on which subset data is located and schedule the distributed data job
    /// respectively.   
    ///
    /// \return     Returns data locality query mode represented by the
    ///             \c IDistributed_data_locality_query_mode interface. If the
    ///             method returns an invalid query mode or just a null pointer
    ///             then the NVIDIA IndeX system prevents the distributed job
    ///             execution.
    /// 
    virtual IDistributed_data_locality_query_mode* get_scheduling_mode() const = 0;

    /// Receiving a distributed data locality from the NVIDIA IndeX system.
    ///
    /// Typically, the \c IDistributed_data_locality reports on which cluster 
    /// node the distributed portions of a dataset are hosted. These portions
    /// are represented as subset data and have a well-defined bounding box.
    ///
    /// The \c IDistributed_data_locality may be used by the application to gather
    /// details on the job execution as each execution thread of the job is sent to
    /// exactly one data subset. That is, the cluster host and the bounding box 
    /// could be derived and taken into account for further job execution.
    ///
    /// \param[in] determined_locality  The data locality determined 
    ///                                 by the NVIDIA IndeX system based on the 
    ///                                 information that this job implementation 
    ///                                 provides along with the #get_scheduling_mode.
    ///
    virtual void receive_data_locality(
        IDistributed_data_locality* determined_locality) = 0;

    /// Local execution of the distributed data job for a processing a data subset.
    ///
    /// The method is invoked by the NVIDIA IndeX system. It facilitates the processing 
    /// of a single data subset. For processing the data an user-implemented
    /// \c IData_subset_processing_task can be executed using the method
    /// \c IData_subset_compute_task_processing::execute_compute_task.
    ///
    /// \param[in] dice_transaction         The DiCE transaction enables access of the 
    ///                                     the distributed data store and launching of
    ///                                     additional jobs, e.g., to retrieve compute
    ///                                     results from different nodes.
    ///
    /// \param[in] data_distribution        The \c IData_distribution interface enables
    ///                                     to derive a data locality and schedule new 
    ///                                     distributed ja jobs.
    ///
    /// \param[in] data_subset_processing   The \c IData_subset_compute_task_processing interface
    ///                                     facilitates the execution of user-implemented
    ///                                     data processing tasks (see \c IData_subset_processing_task).
    /// 
    /// \param[in] data_subset_index        The data subset ID that curresponds and uniquely 
    ///                                     identifies the data subset that this execution call
    ///                                     processes.
    ///
    /// \param[in] data_subset_count        The total number of data subsets that are processed
    ///                                     by the distributed data job.
    ///  
    virtual void execute_subset(
        mi::neuraylib::IDice_transaction*                   dice_transaction,
        const nv::index::IData_distribution*                data_distribution,
        nv::index::IData_subset_compute_task_processing*    data_subset_processing,
        mi::Size                                            data_subset_index,      // not to confuse with subset data ID
        mi::Size                                            data_subset_count) = 0;

    /// Remote execution of the distributed data job for a processing a data subset.
    ///
    /// The method is invoked by the NVIDIA IndeX system on a remote node.
    /// It facilitates the processing of a single data subset. 
    /// For processing the data an user-implemented \c IData_subset_processing_task
    /// can be executed using the method
    /// \c IData_subset_compute_task_processing::execute_compute_task.
    ///
    /// \param[in] serializer               The serializer allows the application to 
    ///                                     communicate computed results, e.g. a mean
    ///                                     value, back to the calling job instance.
    ///                                     The correspondent #receive_subset_result
    ///                                     receives the returned result in its 
    ///                                     deserializer steam.
    ///
    /// \param[in] dice_transaction         The DiCE transaction enables access of the 
    ///                                     the distributed data store and launching of
    ///                                     additional jobs, e.g., to retrieve compute
    ///                                     results from different nodes.
    ///
    /// \param[in] data_distribution        The \c IData_distribution interface enables
    ///                                     to derive a data locality and schedule new 
    ///                                     distributed ja jobs.
    ///
    /// \param[in] data_subset_processing   The \c IData_subset_compute_task_processing interface
    ///                                     facilitates the execution of user-implemented
    ///                                     data processing tasks (see \c IData_subset_processing_task).
    /// 
    /// \param[in] data_subset_index        The data subset ID that curresponds and uniquely 
    ///                                     identifies the data subset that this execution call
    ///                                     processes.
    ///
    /// \param[in] data_subset_count        The total number of data subsets that are processed
    ///                                     by the distributed data job.
    ///  
    virtual void execute_subset_remote(
        mi::neuraylib::ISerializer*                         serializer,
        mi::neuraylib::IDice_transaction*                   dice_transaction,
        const nv::index::IData_distribution*                data_distribution,
        nv::index::IData_subset_compute_task_processing*    data_subset_processing,
        mi::Size                                            data_subset_index,
        mi::Size                                            data_subset_count) = 0;

    /// Receives the result of a remote distributed data job execution.
    ///
    /// The method is invoked by the NVIDIA IndeX system on the local node again and
    /// assembles the results computed by the correspondent #execute_subset_remote that
    /// was launched for the data subset with the same \c data_subset_index. 
    ///
    /// \param[in] deserializer             The deserializer receives the results  
    ///                                     computed on a remote node for the given
    ///                                     data subset in the correspondent 
    ///                                     #execute_subset_remote call.
    ///
    /// \param[in] dice_transaction         The DiCE transaction enables access of the 
    ///                                     the distributed data store and launching of
    ///                                     additional jobs, e.g., to retrieve compute
    ///                                     results from different nodes.
    ///
    /// \param[in] data_distribution        The \c IData_distribution interface enables
    ///                                     to derive a data locality and schedule new 
    ///                                     distributed ja jobs.
    ///
    /// \param[in] data_subset_index        The data subset ID that curresponds and uniquely 
    ///                                     identifies the data subset that this execution call
    ///                                     processes.
    ///
    /// \param[in] data_subset_count        The total number of data subsets that are processed
    ///                                     by the distributed data job.
    ///  
    virtual void receive_subset_result(
        mi::neuraylib::IDeserializer*           deserializer,
        mi::neuraylib::IDice_transaction*       dice_transaction,
        const nv::index::IData_distribution*    data_distribution,
        mi::Size                                data_subset_index,
        mi::Size                                data_subset_count) = 0;
};

/// Mixin class that implements the mi::neuraylib::Distributed_data_job interface.
///
/// The mixin class provides a default implementation of some of the pure virtual
/// methods of the \c nv::index::IDistributed_data_job interface. The documentation
/// here just lists the behavior of the default implementation,
/// see \c nv::index::IDistributed_data_job for the documentation of the
/// methods themselves.
///
template <mi::Uint32 id1, mi::Uint16 id2, mi::Uint16 id3,
          mi::Uint8  id4, mi::Uint8  id5, mi::Uint8  id6,  mi::Uint8 id7,
          mi::Uint8  id8, mi::Uint8  id9, mi::Uint8  id10, mi::Uint8 id11,
          class I = IDistributed_data_job>
class Distributed_data_job : public mi::neuraylib::Base<id1, id2, id3, id4, id5, id6, id7, id8, id9, id10, id11, I>
{
public:
    /// Receiving a distributed data locality from the NVIDIA IndeX job scheduler.
    ///
    /// \param[in] determined_locality  The data locality determined by the NVIDIA IndeX system.
    ///                                 This default implementation does not consider the
    ///                                 data locality further.
    ///
    virtual void receive_data_locality(
        IDistributed_data_locality* determined_locality)
    {
        (void)determined_locality;   // avoid unused warnings
        return; /* do nothing */
    }
};

/// Scheduling and launching distributed data analysis and processing jobs.
///
/// The scheduler takes an user-implemented \c IDistributed_data_job and executes
/// it. The scheduler first examines the job's scheduling mode and determines the 
/// data locality of the targeted distributed dataset. That is, the scheduler determines
/// on which cluster node which data subset is hosted. The scheduler then sends the 
/// execution threads to these cluster nodes for processing these datasets. One execution
/// thread per subset. The data locality will be passed back to the job for information 
/// and potential use.
///
/// An instance of a \c IDistributed_data_job_scheduler interface implementation is
/// created and exposed by the interace \c IData_distribution, which is available through
/// the \c ISession interface.
///
/// \ingroup nv_index_data_edit
///
class IDistributed_data_job_scheduler :
    public mi::base::Interface_declare<0x91224ed0,0x6e2a,0x48c9,0xb1,0x79,0x61,0x8e,0x22,0xd2,0x5e,0xf2,
                                       mi::base::IInterface>
{
public:
    /// Launching the user-implemented distributed data job.
    ///
    /// \param[in] job                  The user-implemented distributed data job 
    ///                                 for scheduling and cluster-wide execution.
    ///
    /// \param[in] dice_transaction     The DiCE transaction that this job runs in.
    ///
    virtual mi::Sint32 execute(
        IDistributed_data_job*                  job,
        mi::neuraylib::IDice_transaction*       dice_transaction) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IDATA_DISTRIBUTION_H
