/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Main API of the NVIDIA IndeX library.
///
/// This file defines the main API for integrators to use the NVIDIA IndeX library.

#ifndef NVIDIA_INDEX_IINDEX_H
#define NVIDIA_INDEX_IINDEX_H

#include <mi/dice.h>

#include <nv/index/iaffinity_information.h>
#include <nv/index/ibalancing_operations.h>
#include <nv/index/icluster_change_callback.h>
#include <nv/index/idistributed_data_locality.h>
#include <nv/index/ierror.h>
#include <nv/index/iindex_canvas.h>
#include <nv/index/iindex_scene_query.h>
#include <nv/index/iopengl_application_buffer.h>
#include <nv/index/iperformance_values.h>
#include <nv/index/iprogress_callback.h>
#include <nv/index/iscene_query_results.h>
#include <nv/index/itime_mapping.h>
#include <nv/index/iviewport.h>
#include <nv/index/ievent_tracing.h>

//
// Define modules for the API documentation.
// They will be listed in the same order in which they are defined here.
//

/// \defgroup nv_index NVIDIA IndeX API
///
/// \brief  The NVIDIA IndeX C++ library consists of pure virtual interfaces exposed through a single library entry point.
///
/// The NVIDIA IndeX C++ library exports a single factory function 
/// \c nv_index_factory which returns an instance of the interface class \c nv::index::IIndex.
/// The \c nv::index::IIndex interface enables an application to authenticate, configure, 
/// start up, operate with, and shut down the NVIDIA IndeX library.
/// Moreover, \c nv::index::IIndex represents the main or root interface for using 
/// NVIDIA IndeX in applications.
///
/// The NVIDIA IndeX libary is composed by various API components. The interface class
/// \c nv::index::IIndex gives access to the available API components.
/// These API components expose the the use of the data distribution, rendering and computing
/// functionalities and capabilities as well as the data structures that applications
/// operate on.
///
/// The NVIDIA IndeX C++ API declares pure virtual interfaces and relies on Plain-Old-Datatypes
/// only, which enables binary compatability on operating system platforms, i.e., the NVIDIA IndeX
/// library supports all Linux distributions without explicit recompilation.

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_session NVIDIA IndeX Session
/// \ingroup nv_index
///
/// \brief A session aggregates the entire environment relevant for an user's workspace.
///
/// A session aggregates the entire environment relevant for an user's workspace.
/// It contains the scene description and the data distribution, it facilitates different views 
/// on the data, allows to create and browse the scene contents and gives access to the distributed data
/// which exposes the assigned of data to nodes in the GPU cluster environment.

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_data_representation Distributed Data Representation
/// \ingroup nv_index
///
/// \brief Spatial decomposition enabling the distribution of data for distributed data visualization and computing. 
///
/// NVIDIA IndeX decomposes the entire space, which include the scene description, into 
/// spatially disjoint \e subregions. These subregions are assigned to nodes in the GPU cluster
/// environment and effectively prune the large-scale into smaller-sized portions (called 
/// \e data \e subsets, see @ref nv_index_data_subsets) for parallel and distributed 
/// data management.
/// Merely the subregions are assigned to cluster nodes but the contents of the data subsets
/// contained inside subregions is imported by each of the cluster nodes in parallel (see
/// @ref nv_index_data_import). That is, the logical spatial decomposition including a scene
/// declaration (see @ref nv_index_scene_description) is distributed to cluster nodes 
/// whereas the effective large-scale data is routed through a single cluster node but
/// imported by each and every cluster nodes individually. 

/// \defgroup nv_index_data_subsets Distributed Data Subsets
/// \ingroup nv_index_data_representation
///
/// \brief Data subsets represent major entities for distributed rendering and computing.
///
/// NVIDIA IndeX decomposes the scene into subregions. In particular the decomposition also 
/// subdivides those scene elements that represent large-scale datasets 
/// (see @ref nv_index_scene_description_distributed_data) into smaller-sized portions.
/// These are called \e data \e subsets. Data subsets are contained inside subregions,
/// i.e., each subregion that intersect part of the distributed dataset extent contains one 
/// data subset of the distributed dataset.
/// This way, data subsets represent the major entities for data distribution to cluster hosts and   
/// GPUs, for parallel and scalable distributed rendering on GPU clusters and for
/// parallel and scalable distributed data processing. 
///
/// For instance, the sparse volume dataset \c ISparse_volume_scene_element will be represented
/// by many \c ISparse_volume_subset distributed to cluster nodes. Each of the volume's 
/// subsets are rendered independently for scalable volume data visualization and each of the 
/// subsets may be processed, analysed and manipulated in parallel for scalable and distributed 
/// computing.  
///
/// Data subsets are distributed datatype specific and comprise tailored interfaces for
/// efficient access of the internal type-specific data structures as well as efficient
/// update of the internal data representation of a data subset contained in a subregion.
/// For instance, the interface class \c ISparse_volume_subset exposes methods for
/// writing new voxel values and thus updating the volume's data. Also, the interace class
/// provides methods for querying the internal brick and level-of-detail structures and 
/// attribute descriptions.

/// \defgroup nv_index_data_import Distributed Data Import Mechanism
/// \ingroup nv_index_data_representation
///
/// \brief Mechanism for efficient parallel and distributed data imports from arbitrary sources.
///
/// NVIDIA IndeX enables the data imports through a distributed import callback mechanism.
/// The import callback is triggered by nodes in the cluster in parallel. Those cluster nodes
/// that require the import of a data subset (see @ref nv_index_data_subsets) invoke the import 
/// calback for the data subset. By means of such distributed data imports, large-scale datasets
/// are never routed through a single cluster node, which would represent a major bottleneck, 
/// e.g., when loading terabytes of data. Instead, the import of data subsets runs in parallel
/// on each cluster node and by all cluster nodes in parallel as well.
/// As a result, NVIDIA IndeX is able to import terabytes of volume data within a 2-3 minutes 
/// rather than hours.
/// 
/// Besides this core mechanism that facilitates the accelerated data cluster-wide import of subset 
/// data, applications can implement import callbacks allowing for (1) the suppport of proprietary 
/// data formats and (2) any arbitrary data sources. 
/// Data source for import can obviously be file system but could also be remote storage nodes, 
/// Cloud storages (including AWS S3 cloud storage, Azure blob storage and GCP cloud bucket), or SQL 
/// databases, for example. Applications are also able to create synthetic data from a compute source.
///   
/// An NVIDIA IndeX package ships extension that include importers, e.g., 
/// for the commone <a href="https://www.openvdb.org/">OpenVDB</a> data representation,
/// for the common <a href="https://bluware.com/data-solutions/vds/">VDS data format</a>
/// or for plain raw data volume data.

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_data_distribution Data Distribution
/// \ingroup nv_index
///
/// \brief The distribution of data to nodes in the cluster may change over time but is always known and accessible by an application.
///

/// \defgroup nv_index_data_locality Data Locality
/// \ingroup nv_index_data_distribution
///
/// \brief Data locality information represents valuable knowledge, e.g., for running compute operations to the distributed data.
///

/// \defgroup nv_index_workload_balancing Workload Balancing
/// \ingroup nv_index_data_distribution
///
/// \brief Redistribution of data may occur if the distribution in unbalanced.  
///

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_scene_description Scene description
/// \ingroup nv_index
///
/// \brief The scene description is hierarchical and resembles those of scene graphs for rendering.  
///
/// The scene description defines everything in the scene,
/// including visible shapes, their attributes, and groups.

/// \defgroup nv_index_scene_description_distributed_data Distributed Datasets
/// \ingroup nv_index_scene_description
///
/// \brief Large-scale datasets for rendering and processing are specified as \e scene \e elements in the hierarchical scene description.  
///
/// Shapes are a visible scene object. These are created by
/// IScene::create_shape. The behavior of many shapes can be changed using the
/// scene attributes. \see IAttribute

/// \defgroup nv_index_scene_description_group Composing a Scene using Scene Groups
/// \ingroup nv_index_scene_description
///
/// \brief Scene groups are basic building blocks for implementing the hierarchical scene descriptions and grouping scene elements.
///
/// Scene groups construct a scene in a hierarchical form.

/// \defgroup nv_index_scene_description_shape Basic Shapes
/// \ingroup nv_index_scene_description
///
/// \brief Shapes are categorized into higher-level 3D geometries and 2D image space objects.
///
/// Shapes are a visible scene object. These are created by
/// IScene::create_shape. The behavior of many shapes can be changed using the
/// scene attributes. \see IAttribute

/// \defgroup nv_index_scene_description_object_shape Object Space Shapes
/// \ingroup nv_index_scene_description_shape
///
/// \brief Higher-level shapes are defined in 3D space and can be co-rendered with large-scale datasets.
///

/// \defgroup nv_index_scene_description_image_shape Image Space Shapes
/// \ingroup nv_index_scene_description_shape
///
/// \brief Image space shapes are defined in 2D space and can be co-rendered with large-scale datasets.
///

/// \defgroup nv_index_scene_description_attribute Scene attributes
/// \ingroup nv_index_scene_description
///
/// \brief Attributes effect the distributed datasets or shapes through the hierarchical scene description.
///
/// Scene attributes provide a common mechanism for controlling other scene
/// elements. These attributes can be created by IScene::create_attribute
/// method.

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_rendering Distributed Rendering
/// \ingroup nv_index
///
/// \brief \todo mn
///

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_computing Distributed Computing
/// \ingroup nv_index
///
/// \brief \todo mn
///

/// \defgroup nv_index_data_access Distributed Data Access
/// \ingroup nv_index_computing
///
/// \brief \todo mn
///

/// \defgroup nv_index_data_edit Distributed Compute Jobs
/// \ingroup nv_index_computing
///
/// \brief \todo mn
///

/// \defgroup nv_index_data_computing Distributed Rendering and Computing
/// \ingroup nv_index_computing
///
/// \brief \todo mn
///

// ----------------------------------------------------------------------------------

/// \defgroup xac NVIDIA IndeX Accelerated Computing Technology
/// \ingroup nv_index
///
/// \brief \todo mn
///
/// The NVIDIA IndeX Accelerated Computing (XAC) interfaces enable application-side
/// programming of CUDA programs that are compiled and linked with the NVIDIA IndeX 
/// infrastructure in realtime and then injected into the
/// the NVIDIA IndeX rendering or computing pipelines. XAC programs enable user-defined
/// distributed data 'shading' for high-fidelity scientific data visualization,
/// user-defined ray-sampling for querying and analyzing distributed data,
/// and GPU-based computing of the distributed data.
/// XAC programs are written in the CUDA programming language and the XAC libraries
/// provide additional CUDA routines specific to NVIDIA IndeX.

/// \defgroup xac_scene Sampling and Scene Information for XAC programs
/// \ingroup xac
///
/// \brief \todo mn
///
/// The NVIDIA IndeX XAC programs For each rendered frame, NVIDIA IndeX performs a front-to-back ray
/// casting procedure in a scene defined by an XAC class.

/// \defgroup xac_obj XAC elements
/// \ingroup xac
///
/// \brief \todo mn
///
/// Scene elements for XAC programs.
/// Predefined elements that are part of the XAC interface can provide information
/// about the current IndeX state as well as a set of geometric scene elements used.
/// for the final visualization output.

/// \defgroup xac_lib XAC functions
/// \ingroup xac
///
/// \brief \todo mn
///
/// Functions and macros for XAC programs.
/// The XAC interface also provides a set of convenience macros and functions for
/// printing debugging information, transformation handling, basic shading
/// operations and generic gradient operators.

/// \defgroup xac_compute XAC compute
/// \ingroup xac
///
/// \brief \todo mn
///
/// NVIDIA IndeX Accelerated Compute (XAC) facilities for gpu accelerated computing of data.

// ----------------------------------------------------------------------------------

/// \defgroup nv_scene_queries Built-in Picking Operation
/// \ingroup nv_index
///
/// \brief \todo mn
///

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_configuration Configuration
/// \ingroup nv_index
///
/// \brief General configuration settings that control the behavior of NVIDIA IndeX.
///
/// General configuration settings that control the behavior of NVIDIA IndeX.
///

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_utilities Utilities
/// \ingroup nv_index
///
/// \brief \todo mn
///

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_performance_measurement Performance measurement
/// \ingroup nv_index
///
/// \brief \todo mn
///

// ----------------------------------------------------------------------------------

/// \defgroup nv_index_error Errors
/// \ingroup nv_index
///
/// \brief \todo mn
///


/// Common namespace for all NVIDIA APIs.
namespace nv
{

/// Namespace for NVIDIA IndeX library APIs.
/// \ingroup nv_index
//
namespace index
{

/// Configuration of the NVIDIA IndeX cluster environment.
///
/// Interface to set and query the NVIDIA IndeX cluster configuration.
///
/// \ingroup nv_index_configuration
//
class ICluster_configuration :
    public mi::base::Interface_declare<0x84fcac5c,0x25d1,0x4456,0x88,0x22,0xe4,0x92,0xec,0xee,0xea,0x1a>
{
public:
    /// Set the service mode of the present cluster machine.
    ///
    /// \param service_mode     The service mode can be either 'rendering',
    ///                         'compositing', 'rendering_and_compositing', or
    ///                         'none'.
    /// \return                 Returns \c true on success.
    ///
    virtual bool set_service_mode(const char* service_mode) = 0;

    /// Get number of GPUs in the cluster environment that can be used for
    /// cluster-wide rendering and computing.
    ///
    /// \return         Returns the number of GPUs present in the cluster
    ///                 environment.
    ///
    virtual mi::Uint32 get_number_of_GPUs() const = 0;

    /// Get number of nodes in the cluster used for distributed, parallel rendering
    /// and computing.
    ///
    /// \return         The number of cluster nodes.
    ///
    virtual mi::Uint32 get_number_of_hosts() const = 0;

    /// Get number of nodes in the cluster used for distributed,
    /// parallel rendering only.
    ///
    /// \return         The number of cluster nodes assigned
    ///                 to the rendering service.
    ///
    virtual mi::Uint32 get_number_of_rendering_hosts() const = 0;

    /// Get number of nodes in the cluster used for distributed,
    /// parallel compositing only.
    ///
    /// \return         The number of cluster nodes assigned
    ///                 to the compositing service.
    ///
    virtual mi::Uint32 get_number_of_compositing_hosts() const = 0;

    /// Get cluster node id for the given node.
    ///
    /// \param[in] index    Index into the list of cluster nodes.
    ///
    /// \return             Returns the cluster node id.
    ///
    /// \todo Rename to get_node_id().
    ///
    virtual mi::Uint32 get_host_index(mi::Uint32 index) const = 0;

    /// Get the cluster node id of the local node that the viewer runs on.
    ///
    /// \return     Returns the cluster node id of the local node or 0 if
    ///             the node has not not yet connected to cluster.
    ///
    /// \todo Rename to get_local_node_id().
    ///
    virtual mi::Uint32 get_local_host_id() const = 0;

    /// Get hostname for the given host id.
    ///
    /// \param[in]  host_id     The host id used to query the hostname.
    ///
    /// \return                 Returns the hostname of the machine with
    ///                         the given id. Will return 0 if no information
    ///                         about that host is available (yet).
    ///
    virtual const char* get_host_name(mi::Uint32 host_id) const = 0;

    /// A sub-cluster represents a collections of machines. The collection of
    /// machines represents a subset of the machines in the entire cluster.
    /// A sub-cluster enables, for instance, scalable large-scale display
    /// support.
    ///
    /// \param[in] sub_cluster_id   Sets the sub-cluster id of the present
    ///                             cluster machine.
    ///
    /// \return                     Returns \c true on success.
    ///
    virtual bool set_sub_cluster_id(mi::Uint32 sub_cluster_id) = 0;

    /// Enable automatic subclustering of machines in the entire cluster using DiCE.
    ///
    /// \param[in] min_sub_cluster_size     Sets the minimum number of machines in a sub-cluster.
    ///
    /// \param[in] max_nr_of_sub_clusters   Sets the number of sub-clusters required.
    ///
    /// \return                             Returns \c true on success.
    ///
    virtual bool set_automatic_subclustering(
        mi::Uint32 min_sub_cluster_size,
        mi::Uint32 max_nr_of_sub_clusters) = 0;

    /// Register a callback that allows notifying the application
    /// if the cluster topology has changed.
    ///
    /// \param[in] callback                 The cluster change callback.
    ///
    virtual void register_callback(ICluster_change_callback* callback) = 0;
};

/// API component for creating the user session in the NVIDIA IndeX library.
///
/// \ingroup nv_index
///
class IIndex_session :
    public mi::base::Interface_declare<0x8ba1e5c7,0xaeb9,0x45ee,0xbb,0xf1,0xd6,0x25,0x91,0x70,0xce,0xaa>
{
public:
    /// Create an instance that represents the \c ISession that manages the
    /// general operations applied to the system. A session, for instance, contains
    /// the configuration (\c IConfig_settings) and a scene
    /// (\c IScene), but also exposes factory functionalities for creating
    /// scene elements or the virtual camera.
    ///
    /// \param[in] dice_transaction         The DiCE database transaction
    ///                                     used for creating a session.
    ///
    /// \param[in] session_name             The session may have a certain name to retrieve
    ///                                     the session by name without knowing the tag.
    ///                                     Enables failing viewer machines to continue
    ///                                     rendering or joining viewer machines to implement
    ///                                     multi-viewer/multi-scene scenarios for interactive
    ///                                     collaborations on shared large scale datasets.
    ///
    /// \return                             Returns the created \c ISession tag
    ///                                     on success. The caller takes ownership of the session
    ///                                     and is responsible for freeing the session from the
    ///                                     database by calling \c IDice_transaction::remove() when
    ///                                     it is not needed anymore.
    ///
    virtual mi::neuraylib::Tag_struct create_session(
        mi::neuraylib::IDice_transaction*   dice_transaction,
        const char*                         session_name = 0) = 0;

    /// Sets the user-defined affinity information required to match the spatial data distribution
    /// between an application and NVIDIA IndeX.
    ///
    /// \note Calling this method will flush the spatial data information inside the library and
    /// trigger reloading of all distributed data.
    ///
    /// \param[in] affinity_information     The affinity information is used by the NVIDIA IndeX library when
    ///                                     assigning subregions to hosts and GPUs in the cluster.
    ///                                     This class takes ownership of the parameter.
    ///
    virtual void set_affinity_information(IAffinity_information* affinity_information) = 0;

    /// Update the NVIDIA IndeX library session. Depending on data
    /// and database changes, this updates a session's internal
    /// state. For instance, if a scene element was modified by the
    /// user, then this call takes care of ensuring consistency of
    /// the internal data structures.
    ///
    /// \param[in] session_tag                  The tag of an \c ISession.
    /// \param[in] dice_transaction             The DiCE transaction that the
    ///                                         update runs in.
    virtual void update(
        mi::neuraylib::Tag_struct           session_tag,
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;

    /// A custom clock generates time ticks for time-dependent data visualizations.
    ///
    /// \param[in] clock_pulse_generator        A user-defined clock, this class takes ownership.
    ///
    virtual void set_clock(IClock_pulse_generator* clock_pulse_generator) = 0;

    /// A custom clock generates time ticks for time-dependent data visualizations.
    ///
    /// \returns            The registered user-defined clock.
    ///
    virtual IClock_pulse_generator* get_clock() const = 0;
};

typedef mi::Uint32  IFrame_identifier; ///< A frame's unique identifier.

/// The frame results store information gathered during the rendering process. Such information
/// include error information and the performance values.
///
///
/// \ingroup nv_index_rendering
///
class IFrame_results :
    public mi::base::Interface_declare<0xdbe8f991, 0xccbf, 0x4445, 0xad, 0x4d, 0x77, 0xf3, 0xd, 0x67, 0xcc, 0xff>
{
public:
    /// Returns an instance of the performance values interface \c IPerformance_values containing
    /// detailed performance information gathered during the rendering process.
    ///
    /// \note Use the \c mi::base::Handle template to store the returned interface pointer in order
    ///       to guarantee the correct destruction of the returned interface.
    ///
    /// \return         The the detailed performance values information
    ///                 gathered during the rendering process.
    ///
    virtual IPerformance_values* get_performance_values() const = 0;

    /// Returns an instance of the error set interface \c IError_set containing information about
    /// the success or failure of the rendering process.
    ///
    /// \note Use the \c mi::base::Handle template to store the returned interface pointer in order
    ///       to guarantee the correct destruction of the returned interface.
    ///
    /// \return         Returns an error set containing information about the success or failure
    ///                 of the rendering process.
    ///
    virtual IError_set* get_error_set() const = 0;
};

/// List of rendering results for multi-view rendering. Each list
/// entry contains the rendering results of a single viewport.
///
///
/// \ingroup nv_index_rendering
///
class IFrame_results_list :
    public mi::base::Interface_declare<0x9524bb73,0x4392,0x477b,0x9a,0x0f,0x5d,0xd6,0x10,0x32,0xc3,0x85>
{
public:
    /// Returns the number of frame results in the list.
    ///
    /// \return number of results
    virtual mi::Size size() const = 0;

    /// Returns the frame result at the given position.
    ///
    /// \param[in] index Position in the list
    /// \return frame results, or 0 when \c index is invalid
    virtual nv::index::IFrame_results* get(mi::Size index) const = 0;
};

/// The frame info callbacks receive details related to the frame to be rendered. Such details, for instance,
/// include the frame identifier that can be used to cancel a frame while rendering or the image tiles
/// rendered to the canvas.
/// The frame info allows the embedding application to derive further statistics other than the performance
/// values from the frame rendering.
///
/// The application needs to implement the methods of the frame info interface class receive and use
/// the frame details. An instance of the user-defined class can be passed to the IndeX render call
/// and the IndeX library takes care to populate the appropriate frame details.
///
/// \ingroup nv_index_rendering
///
class IFrame_info_callbacks :
    public mi::base::Interface_declare<0xa686de09,0x6605,0x46ae,0xa0,0x9a,0xd2,0x8c,0x8b,0xcc,0xf1,0x92>
{
public:
    /// Callback to receive the frame identifier that can be used to cancel the rendering of a frame.
    /// The method needs to be implemented to receive the frame identifier on the application side.
    ///
    /// \param[in] frame_id     The unique frame identifier exposed to the application.
    ///
    virtual void set_frame_identifier(const IFrame_identifier& frame_id) = 0;

    /// Callback to receive notifications about a dynamic memory allocation event during NVIDIA IndeX runtime
    ///
    /// \param[in]  host_id                 The unique host id on which the dynamic memory allocation occurred.
    /// \param[in]  device_id               The device id of GPU on the specified host associated with
    ///                                     the dynamic allocation event.
    /// \param[in]  memory_allocation_size  The size of the memory block requested to be allocated
    ///                                     in device memory (unit Byte).
    /// \param[in]  memory_available        The size of the available memory before the dynamic memory
    ///                                     allocation (unit Byte).
    /// \param[in]  memory_freed_up         The size of the memory freed up to fulfill the initial memory
    ///                                     allocation request.
    ///
    virtual void report_dynamic_memory_eviction(
        mi::Uint32  host_id,
        mi::Uint32  device_id,
        mi::Size    memory_allocation_size,
        mi::Size    memory_available,
        mi::Size    memory_freed_up) = 0;

    /// Callback to receive notifications about a GPU-device reset event during NVIDIA IndeX runtime.
    /// Following the detection of memory fragmentation issues which cause memory allocation errors
    /// while seemingly sufficient GPU-device memory is available a device reset will effectively
    /// de-fragment the device memory.
    ///
    /// \param[in]  host_id                 The unique host id on which the device memory reset is performed in.
    /// \param[in]  device_id               The device id of GPU on the specified host associated with
    ///                                     the device memory reset event.
    ///
    virtual void report_device_memory_reset(
        mi::Uint32  host_id,
        mi::Uint32  device_id) = 0;

    /// Callback to receive notification about internal workload balancing operations that occurred
    /// and NVIDIA IndeX produced when redistributing workload for scalable rendering and data processing.
    ///
    /// \param[in]  balancing_ops           An instance of the \c IBalancing_operations interface reporting
    ///                                     on occurred workload balancing operations. The user defined class 
    ///                                     owns this \c balancing_ops object.
    ///
    virtual void report_workload_balancing_operations(
        IBalancing_operations* balancing_ops) = 0;
};

/// Interface class for creating NVIDIA IndeX built-in canvases such as a CUDA memory canvas.
///
/// \ingroup nv_index_rendering
///
class IIndex_canvas_creation_properties :
    public mi::base::Interface_declare<0x788ee4b1,0xa0eb,0x47e5,0x91,0xf0,0x92,0x0a,0x30,0xfa,0xad,0x11>
{
public:
    /// Get the resolution that a built-in canvas shall be initialized with.
    ///
    /// \return     The resolution for creating a built-in canvas.
    ///
    virtual const mi::math::Vector_struct<mi::Uint32, 2>& get_resolution() const = 0;
};

/// Interface class for creating NVIDIA IndeX built-in a CUDA memory canvas.
///
/// \ingroup nv_index_rendering
///
class IIndex_cuda_canvas_creation_properties :
    public mi::base::Interface_declare<0xae97ca4b,0xe8c5,0x4c5b,0xa7,0xe6,0xd6,0x10,0xfd,0xad,0xcf,0xe1, nv::index::IIndex_canvas_creation_properties>
{
public:
    /// The CUDA device by which the CUDA memory canvas shall be managed.
    ///
    /// \return     The CUDA device ID.
    ///
    virtual mi::Sint32 get_cuda_device_id() const = 0;
};

/// Implements the properties required by NVIDIA IndeX for creating a CUDA canvas.
/// This implementation is to be moved to the application layer.
///
/// \ingroup nv_index_rendering
///
class Index_cuda_canvas_creation_properties : public mi::base::Interface_implement<nv::index::IIndex_cuda_canvas_creation_properties>
{
public:
    /// Create an instance that delivers all properties required for creating 
    /// a CUDA canvas.
    /// 
    /// \param[in]  cuda_device_id          Defines the cuda device on which a 
    ///                                     memory for a cuda canvas shall be
    ///                                     allocated.
    ///                                     If set to '-1' or an invalid/undefined
    ///                                     cuda device, then NVIDIA IndeX selects 
    ///                                     a device.
    ///
    /// \param[in]  resolution              The resolution of the requested
    ///                                     built-in canvas.
    ///
    Index_cuda_canvas_creation_properties(
        mi::Sint32                                          cuda_device_id,
        const mi::math::Vector_struct<mi::Uint32, 2>&       resolution)
      : m_cuda_device_id(cuda_device_id),
        m_resolution(resolution) {}

    /// A CUDA device hosts the canvas contents in the CUDA buffer.
    ///  \return    Returns the CUDA device id that stores the canvas contents.
    virtual mi::Sint32                                    get_cuda_device_id()  const { return m_cuda_device_id; }
    /// A canvas has a given resolution.
    ///  \return    Returns the resolutions of the canvas.  
    virtual const mi::math::Vector_struct<mi::Uint32, 2>& get_resolution()      const { return m_resolution;     }

private:
    mi::Sint32                              m_cuda_device_id;
    mi::math::Vector_struct<mi::Uint32, 2>  m_resolution;
};

/// Enables the rendering of a user-defined session/scene.
///
///
/// \ingroup nv_index_rendering
///
class IIndex_rendering :
    public mi::base::Interface_declare<0x435617a4,0x589b,0x47d9,0x99,0x17,0x05,0x82,0x7d,0x1d,0xfc,0x3e>
{
public:
    /// Internal built-in canvas types for external use in an application. 
    enum Index_builtin_canvas_type
    {
        IDX_CANVAS_CUDA_MEMORY = 0u, ///< A canvas that allocates the framebuffer in CUDA memory.
        IDX_CANVAS_MAIN_MEMORY = 1u  ///< A canvas that allocates the framebuffer in main memory.
    };

    /// Creates a built-in \c IIndex_canvas instance.
    /// While NVIDIA IndeX features the ability to implement user-defined canvases
    /// that the application can pass to the rendering process, NVIDIA IndeX can
    /// also create built-in canvases for use by an application.
    /// On the one hand, using a built-in canvas frees an integrator from
    /// implementing default canvases such as a simple main memory canvas.
    /// It should be noted, that built-in canvases can comprise specific capabilities 
    /// such as a CUDA memory canvas to which NVIDIA IndeX composites its
    /// rendering results without ever transferring rendering results back to 
    /// main memory.
    ///
    /// \param[in]  properties              Defines the specific canvas and
    ///                                     additional properties for creating
    ///                                     a canvas.
    ///
    /// \return                             Returns a built-in canvas.
    ///                                     The application takes ownership
    ///                                     of the returned canvas.
    ///
    virtual IIndex_canvas* create_canvas(
        IIndex_canvas_creation_properties*              properties) const = 0;

    /// Renders a frame of the scene and writes the resulting image
    /// tiles into the user-defined or a built-in \c IIndex_canvas.
    /// This method assumes a single viewport that covers the entire
    /// canvas.
    ///
    /// \note \c IIndex_session::update() needs to be called before
    /// this method, using the same DiCE transaction.
    ///
    /// \param[in] session_tag                  The \c ISession tag.
    /// \param[in] canvas                       The user-defined canvas where all
    ///                                         image tiles will be rendered into.
    /// \param[in] dice_transaction             The DiCE database transaction for
    ///                                         this render call.
    /// \param[in] progress                     The progress callback to give feedback
    ///                                         on the rendering process. The progress
    ///                                         callback can be set to 0 if not
    ///                                         needed.
    /// \param[in] frame_info                   Callback to receive details
    ///                                         about the frames rendered, can
    ///                                         be set to 0.
    /// \param[in] composite_immediately        Deprecated, should be set to \c true.
    /// \param[in] opengl_app_buffer            OpenGL application buffer, which
    ///                                         contains RGBA and depth information.
    ///                                         Set to 0 if OpenGL integration is not required.
    ///
    /// \return                                 Returns a frame results interface containing
    ///                                         information gathered during the
    ///                                         rendering process (e.g. error information
    ///                                         and performance values).
    ///
    virtual IFrame_results* render(
        mi::neuraylib::Tag_struct                session_tag,
        IIndex_canvas*                           canvas,
        mi::neuraylib::IDice_transaction*        dice_transaction,
        IProgress_callback*                      progress = 0,
        IFrame_info_callbacks*                   frame_info = 0,
        bool                                     composite_immediately = true,
        IOpengl_application_buffer*              opengl_app_buffer = 0) = 0;

    /// Renders a frame of the scene and writes the resulting image
    /// tiles into the user-defined or a built-in \c IIndex_canvas. Each element
    /// of the given viewport list specifies a target area on the
    /// canvas as well as an \c IScope, which enables variations in
    /// the scene between viewport.
    ///
    /// The viewports are rendered in back-to-front order, i.e. when
    /// viewports bounding boxes overlap then a viewport in the list
    /// will be rendered on top of earlier list entries.
    ///
    /// \note This method internally creates and commits a \c
    /// mi::neuraylib::IDice_transaction for each viewport and
    /// therefore all transactions editing the scene should be
    /// committed. Also, in contrast to the non-viewport version of
    /// this method, this one does not require \c
    /// IIndex_session::update() to be called.
    ///
    /// \param[in] session_tag                  The \c ISession tag.
    /// \param[in] canvas                       The user-defined canvas where all
    ///                                         image tiles will be rendered into.
    /// \param[in] viewport_list                List of viewports that should be rendered.
    ///
    /// \return                                 Returns a list of frame results
    ///                                         containing information gathered
    ///                                         during the rendering process (e.g. error
    ///                                         information and performance values) for
    ///                                         each of the viewports.
    virtual IFrame_results_list* render(
        mi::neuraylib::Tag_struct                session_tag,
        IIndex_canvas*                           canvas,
        IViewport_list*                          viewport_list) = 0;

    /// Renders a frame of the scene and writes the resulting image
    /// tiles into multiple user-defined \c IIndex_canvas instances.
    /// Each element of the given canvas/viewport-list pairs specifies a canvas
    /// as well as the associated viewports in that canvas. Each viewport's
    /// \c IScope enables variations in the scene between viewports.
    ///
    /// \note This method must always receive the list of all used canvases, even if some of them
    /// should not be rendered, to ensure proper cache handling. However, the viewports in the
    /// canvases that should be skipped for rendering can be disabled by calling \c
    /// IViewport::set_enable().
    ///
    /// \note This method internally creates and commits a \c
    /// mi::neuraylib::IDice_transaction for each viewport and
    /// therefore all transactions editing the scene should be
    /// committed. Also, in contrast to the non-viewport version of
    /// this method, this one does not require \c
    /// IIndex_session::update() to be called.
    ///
    /// \param[in] session_tag                  The \c ISession tag.
    /// \param[in] canvas_viewport_list         List of user-defined canvases with associated
    ///                                         viewports.
    ///
    /// \return                                 Returns a list of frame results
    ///                                         containing information gathered
    ///                                         during the rendering process (e.g. error
    ///                                         information and performance values) for
    ///                                         each of the viewports.
    virtual IFrame_results_list* render(
        mi::neuraylib::Tag_struct                session_tag,
        ICanvas_viewport_list*                   canvas_viewport_list) = 0;

    /// Cancel the rendering of a frame.
    ///
    /// \param[in] frame_id     The frame identifier uniquely identifies a frame that has been
    ///                         started to be rendered.
    ///
    /// \return                 Returns true if the frame has been canceled and false otherwise,
    ///                         e.g., the frame has already been rendered.
    ///
    virtual bool cancel_rendering(const IFrame_identifier& frame_id) = 0;
};


/// Interface to represent the entry point to the NVIDIA IndeX library and its
/// functionality. Only one instance of this class may exist at the same time.
/// It enables the configuration, start-up and shutdown of the NVIDIA IndeX
/// library and exposes the library's API functionality through API components.
///
/// \ingroup nv_index
///
class IIndex :
    public mi::base::Interface_declare<0x0342c227,0x65f1,0x4465,0xb2,0x79,0x74,0x6a,0x04,0xfa,0x28,0x4b>
{
public:
    /// Authenticates the NVIDIA IndeX library.
    ///
    /// Note: This call checks the argument validity only. The actual
    /// license validation will be done in the nv::index::IIndex::start() call.
    ///
    /// \param[in] vendor_key           Vendor key string.
    /// \param[in] vendor_key_length    Length of the vendor key.
    /// \param[in] secret_key           Secret key string.
    /// \param[in] secret_key_length    Length of the secret key.
    /// \param[in] flexnet_license_path FlexNet license path string.
    ///                                 Can be 0 when no FlexNet license is used.
    /// \param[in] flexnet_license_path_length Length of the FlexNet license path.
    ///                                 Can be 0 when no FlexNet license is used.
    ///
    /// \return Result of the NVIDIA IndeX library authentication
    ///         attempt, 0 means success.
    ///
    virtual mi::Sint32 authenticate(
        const char* vendor_key,
        mi::Sint32  vendor_key_length,
        const char* secret_key,
        mi::Sint32  secret_key_length,
        const char* flexnet_license_path,
        mi::Sint32  flexnet_license_path_length) = 0;

    /// Starts the operation of the IndeX library.
    //
    /// \note All user-defined serializable classes must have been registered before calling this
    /// method.
    ///
    /// \param[in] is_dice_start_block Indicates whether the DiCE start up should be done in blocking
    ///                                mode (compare mi::neuraylib::INeuray::start()). Currently
    ///                                blocking mode is required by NVIDIA IndeX and a warning will
    ///                                be printed if false is specified here.
    ///
    /// \return                       Returns 0 on success.
    ///
    virtual mi::Uint32 start(
        bool is_dice_start_block) = 0;

    /// Shut down the IndeX library.
    /// After this call, all library-related function calls will fail.
    ///
    /// \note           Restarting is not supported.
    ///
    /// \return         Return 0 on success
    ///
    virtual mi::Sint32 shutdown() = 0;

    /// Retrieve an instance of an API component from the NVIDIA
    /// IndeX or DiCE library.
    /// Examples of supported API components:
    ///   - \c IIndex_session.
    ///        The API component for creating a user's session, e.g., scene.
    ///   - \c ICluster_configuration.
    ///        The API component for configuring and querying the NVIDIA IndeX cluster setup.
    ///   - \c IIndex_scene_query
    ///
    /// \param interface_id      The unique id of the interface to be queried.
    ///
    /// \return                  A pointer to the interface or 0 if the interface is not
    ///                          supported. The pointer must be released when it is no longer needed.
    ///
    virtual mi::base::IInterface* get_api_component(const mi::base::Uuid& interface_id) const = 0;

    /// Retrieve an instance of an API component from the IndeX library.
    ///
    /// This templated method is a wrapper around get_api_component(const mi::base::Uuid&) for the
    /// user's convenience. It eliminates the need to call mi::IInterface::get_interface(const
    /// Uuid&) on the returned pointer, since the return type already is a pointer to the type
    /// specified as template parameter.
    ///
    /// \tparam     T The type of the interface to be queried.
    ///
    /// \return     A pointer to the interface or 0 if the interface is not
    ///             supported. The pointer must be released when it is no longer needed.
    ///
    template<class T>
    T* get_api_component() const
    {
        mi::base::IInterface* ptr_iinterface = get_api_component(typename T::IID());
        if (!ptr_iinterface)
        {
            return 0;
        }
        T* ptr_T = static_cast<T*>(ptr_iinterface->get_interface(typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// An instance of the rendering interface \c IIndex_rendering allows rendering
    /// a scene to a user-defined canvas. Multiple rendering interfaces may exists. The rendering
    /// interface instances enabled, for instance, rendering to different multiple views.
    ///
    /// \return         Creates and returns a new instance of the rendering interface
    ///                 \c IIndex_rendering. If the library hasn't been initialized
    ///                 yet, then 0 will be returned.
    ///
    virtual IIndex_rendering* create_rendering_interface() const = 0;

    /// Registers a serializable class with DiCE and NVIDIA IndeX.
    ///
    /// Registering a class for serialization allows communicating class instances through the
    /// serialization mechanism. It enables, for instance, storing database elements
    /// and jobs in the distributed database or communicating job results
    /// (see \c mi::neuraylib::IFragmented_job::execute_fragment_remote()).
    ///
    /// Registering a class for serialization can only be done before DiCE library has been
    /// started.
    ///
    /// \param class_id     The class ID of the class that shall be registered for serialization.
    ///
    /// \param factory      The class factory.
    ///
    /// \return             Returns \c true if the class of was successfully registered
    ///                     for serialization, and \c false otherwise.
    ///
    virtual bool register_serializable_class(
        mi::base::Uuid                      class_id,
        mi::neuraylib::IUser_class_factory* factory) const = 0;

    /// Registers a serializable class with DiCE and NVIDIA IndeX.
    ///
    /// Registering a class for serialization allows communicating class instances through the
    /// serialization mechanism. It enables, for instance, storing database elements
    /// and jobs in the distributed database or communicating job results
    /// (see \c mi::neuraylib::IFragmented_job::execute_fragment_remote()).
    ///
    /// Registering a class for serialization can only be done before DiCE library has been
    /// started.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It uses the default class factory mi::neuraylib::IUser_class_factory
    /// specialized for T.
    ///
    /// \return             Returns \c true if the class of type T was successfully registered
    ///                     for serialization, and \c false otherwise.
    template <class T>
    bool register_serializable_class() const
    {
        mi::base::Handle<mi::neuraylib::IUser_class_factory> factory(
                new mi::neuraylib::User_class_factory<T>());
        return register_serializable_class(typename T::IID(), factory.get());
    }

    /// The NVIDIA IndeX library name or product name.
    ///
    /// \return     Returns the NVIDIA IndeX product name
    ///             as a null-terminated string.
    ///
    virtual const char* get_product_name() const = 0;

    /// The product version of the NVIDIA IndeX library. Please also
    /// refer to the product version in support requests.
    ///
    /// \return     Returns the NVIDIA IndeX product version
    ///             as a null-terminated string.
    ///
    virtual const char* get_version() const = 0;

    /// The NVIDIA IndeX revision number indicates the build. Please also
    /// refer to the product version in support requests.
    ///
    /// \return     Returns NVIDIA IndeX product revision number
    ///             as a null-terminated string.
    ///
    virtual const char* get_revision() const = 0;

    /// Returns the NVIDIA driver version string
    ///
    /// \return     Returns the detected NVIDIA driver version string.
    ///
    virtual const char* get_nvidia_driver_version() const = 0;

    /// Returns the CUDA runtime version number
    ///
    /// \return     Returns the detected CUDA runtime version.
    ///
    virtual mi::Sint32 get_cuda_runtime_version() const = 0;

    /// The NVIDIA IndeX library may come with different API to support various
    /// domain specific interfaces for compute and rendering of large-scale data.
    /// The interface version of the NVIDIA IndeX library indicates the API.
    ///
    /// \return     Returns the NVIDIA IndeX API version.
    ///
    virtual mi::Uint32 get_interface_version() const = 0;

    /// Returns the interface version of the DiCE library.
    ///
    /// \return     Returns the DiCE API version.
    ///
    virtual mi::Uint32 get_dice_interface_version() const = 0;

    /// Returns the product version of the DiCE library.
    ///
    /// \return     Returns the DiCE product version.
    ///
    virtual const char* get_dice_version() const = 0;

    /// Get the IndeX library's built-in logger.
    ///
    /// \return     Returns the built-in logger used in the IndeX library.
    ///             Please refer to the DiCE library API for details.
    ///
    virtual mi::base::ILogger* get_built_in_logger() const = 0;

    /// Get the IndeX library's forwarding logger.
    ///
    /// \return     Returns the forwarding logger used in the IndeX library.
    ///             Please refer to the DiCE library API for details.
    ///
    virtual mi::base::ILogger* get_forwarding_logger() const = 0;

    /// The DiCE interface is available for further use.
    /// Please refer to the DiCE library API for details.
    ///
    /// \return         Exposes the DiCE interface.
    ///
    virtual mi::neuraylib::INeuray* get_dice_interface() = 0;
};

} // namespace index

} // namespace nv

extern "C"
{
/// This factory function is the only public access point to all algorithms and data structures in
/// the NVIDIA IndeX library.
///
/// It returns an instance of the main nv::index::IIndex interface, which can be used to
/// configure, to start up, to operate with, and to shut down the NVIDIA IndeX library.
///
/// \ingroup nv_index
///
/// \note       This function may be called only once in each process.
///
/// \return     Returns an instance of the main nv::index::IIndex interface, which
///             represents the entry point in the NVIDIA IndeX library.
///
MI_DLL_EXPORT
nv::index::IIndex* nv_index_factory();

} // extern "C"

#endif // NVIDIA_INDEX_IINDEX_H
