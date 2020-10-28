/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file  idistributed_data_locality.h
/// \brief Interfaces for exposing the data distribution scheme.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_DATA_LOCALITY_H
#define NVIDIA_INDEX_IDISTRIBUTED_DATA_LOCALITY_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/neuraylib/iserializer.h>

#include <nv/index/idistributed_data_edit.h>

namespace nv
{
namespace index
{

/// Unique identifier refering to a subregion.
///
/// NVIDIA IndeX decomposes the space/scene into smaller
/// sized disjoined portions for distributed and parallel 
/// processing and rendering. These smaller sized portions
/// or regions are called \c subregions. Each subregion is
/// represented by a bounding box and can be uniquely 
/// identivied by an identifier. 
///
/// \ingroup nv_index_data_locality
///
struct Subregion_identifier
{
    /// The actual identifier of the subregion refers to a unique 
    /// spatial area of NVIDIA IndeX's spatial subdivision.
    /// A valid identifier is positive integer greater zero.
    ///
    /// \note   Might need to be bumped to a mi::Uint64 type in the future.
    ///
    mi::Uint32      id; //<! Numerical value that serves as unique identifier.
};

/// Query modes enable the purposeful selection of data subsets for data localities.
///
/// NVIDIA IndeX manages the assignments of data subsets bound inside a 3D bounding box
/// to hosts and GPUs in the cluster environments. A distributed data job (see \c IDistributed_data_job)
/// runs against data subsets (see \c IData_subset). The job may be invoked for the
/// entire set of data subsets that belong to a distributed dataset (see \c IDistributed_data) 
/// or invoked for a subset of data subsets. The interface class 
/// \c IDistributed_data_locality_query_mode enables an application to specify the set 
/// of data subset that shall be consindered for distributed processing and analysis.
/// Derived classes of the interface class implement different query methods. For instance,
/// a the data subset selection for job execution commonly relies on the spatial selection, i.e., 
/// a region of interest, in 3D space.
///
/// When implementing an \c IDistributed_data_job, the method \c IDistributed_data_job::get_scheduling_mode()
/// has to expose a query method so that the internal NVIDIA IndeX infrastructure can schdule 
/// the job to the user-specified subset data selection. 
///
/// \ingroup nv_index_data_locality
///
class IDistributed_data_locality_query_mode :
    public mi::base::Interface_declare<0x841224fe, 0x3c17, 0x43b3, 0xa1, 0xc1, 0xba, 0x30, 0x72, 0xc8, 0x44, 0xf1>
{
public:
    /// Query methods apply to a distributed dataset that is refered to by an unique identifier.
    ///
    /// Each query method applies to a single instance of a distributed dataset. The method 
    /// #get_distributed_data_tag is expected to identify the distributed dataset by 
    /// means of the unique identifier \c mi::neuraylib::Tag_struct. Each derived class
    /// is expected to provide the identifier respectively.
    ///
    /// \return     Returns the \c mi::neuraylib::Tag_struct that uniquely identifiers 
    ///             a distribute dataset in the scene description or DiCE's distributed
    ///             data store.
    ///
    virtual const mi::neuraylib::Tag_struct& get_distributed_data_tag() const = 0;
};

/// Implements a query method that selection all data subsets of a distributed dataset for distributed job execution.
///
/// The class \c Distributed_data_locality_query_mode implements the base class 
/// \c IDistributed_data_locality_query_mode. The purpose of the implementation is 
/// to conveniently select the entire set of data subsets of a distributed dataset 
/// for a job execution.
///
/// \ingroup nv_index_data_locality
///
class Distributed_data_locality_query_mode :
    public mi::base::Interface_implement<nv::index::IDistributed_data_locality_query_mode>
{
public:
    /// Instantiating a query method using the distributed dataset unique identifier.
    ///
    /// \param[in] tag      The \c mi::neuraylib::Tag_struct identifies the distributed dataset.
    ///
    Distributed_data_locality_query_mode(
        const mi::neuraylib::Tag_struct& tag) : m_distribute_data_tag(tag) {}
    /// Implements an empty default destructor.
    virtual ~Distributed_data_locality_query_mode() {}

    /// Implements query methods apply to a distributed dataset that is refered to by an unique identifier.
    ///
    /// \return     Return the \c mi::neuraylib::Tag_struct that uniquely identifiers 
    ///             the entire distribute dataset.
    /// 
    virtual const mi::neuraylib::Tag_struct& get_distributed_data_tag() const { return m_distribute_data_tag; };

private:
    mi::neuraylib::Tag_struct m_distribute_data_tag;  ///<! Unique distributed dataset identifier.
};

/// Query modes to determine the data localities of data subsets inside a region of interest.
///
/// Deriving a data locality from the data distribution for a specific distributed dataset type 
/// based on the bounding box, i.e., a region of interest, represents a fundamental operation.
/// The present interface class \c IDistributed_data_subset_locality_query_mode
/// allows for a specifying a bounding box along with the distributed dataset. 
/// A data locality determined using this query method will include all data subsets that 
/// are contained inside or just intersect the region of interest.
///
/// The interface method #get_spatial_coverage returns the bounding box that covers a spatial 
/// area. All data subsets that are covered at least partially, will be considered in the 
/// data locality.
///
/// \ingroup nv_index_data_locality
///
class IDistributed_data_subset_locality_query_mode :
    public mi::base::Interface_declare<0x841224fe, 0x3c17, 0x43b3, 0xa2, 0xc4, 0xbc, 0x22, 0xa2, 0xa8, 0xcc, 0xa7,
    nv::index::IDistributed_data_locality_query_mode>
{
public:
    /// Spatial query methods let determine data localities based on a spatial coverage.
    ///
    /// The bounding box represented by \c mi::math::Bbox_struct<mi::Float32, 3> specify a spatial 
    /// region of interest. All data subsets that are covered at least partially, will be
    /// considered in a data locality.
    ///
    /// \return     Returns the a bounding box that represents a region of interest for
    ///             determining the data locality. An invalid bounding box will be ingnored
    ///             and the space that the entire distributed dataset covers will be
    ///             considered instead. 
    ///
    virtual const mi::math::Bbox_struct<mi::Float32, 3>& get_spatial_coverage() const = 0;
};

/// Implements a query method that selection all data subsets of a distributed dataset covered by a region of interest.
///
/// The class \c Distributed_data_subset_locality_query_mode implements the base class 
/// \c IDistributed_data_subset_locality_query_mode. The purpose of the implementation is 
/// to conveniently determine all data subsets of a distributed dataset 
/// that are inside or intersect a given spatial area.
///
/// \ingroup nv_index_data_locality
///
class Distributed_data_subset_locality_query_mode :
    public mi::base::Interface_implement<nv::index::IDistributed_data_subset_locality_query_mode>
{
public:
    /// Instantiating a query method using the distributed dataset unique identifier and a given region of interest.
    ///
    /// The spatial query is defined in continous space which requires a bounding box defined 
    /// using floating-point min and max values. 
    ///
    /// \param[in] tag      The \c mi::neuraylib::Tag_struct identifies the distributed dataset.
    ///
    /// \param[in] coverage The bounding box specifies the region of interest for the locality query.
    ///
    Distributed_data_subset_locality_query_mode(
        const mi::neuraylib::Tag_struct&                tag,
        const mi::math::Bbox_struct<mi::Float32, 3>&    coverage)
        : m_distribute_data_tag(tag),
        m_spatial_coverage(coverage) {}

    /// Instantiating a query method using the distributed dataset unique identifier and a given region of interest.
    ///
    /// The spatial query is defined in continous space but the region of interst here is defined 
    /// using integert min and max values. The bounding box is converted to a floating-point
    /// bounding box first.  
    ///
    /// \param[in] tag      The \c mi::neuraylib::Tag_struct identifies the distributed dataset.
    ///
    /// \param[in] coverage The bounding box specifies the region of interest for the locality query.
    ///
    Distributed_data_subset_locality_query_mode(
        const mi::neuraylib::Tag_struct&                tag,
        const mi::math::Bbox_struct<mi::Sint32, 3>&     coverage)
        : m_distribute_data_tag(tag)
    {
        // Following is still unfortunate, i.e., the need to convert to float typed bbox.
        m_spatial_coverage.min.x = static_cast<mi::Float32>(coverage.min.x);
        m_spatial_coverage.min.y = static_cast<mi::Float32>(coverage.min.y);
        m_spatial_coverage.min.z = static_cast<mi::Float32>(coverage.min.z);
        m_spatial_coverage.max.x = static_cast<mi::Float32>(coverage.max.x);
        m_spatial_coverage.max.y = static_cast<mi::Float32>(coverage.max.y);
        m_spatial_coverage.max.z = static_cast<mi::Float32>(coverage.max.z);
    }

    /// Implements an empty default destructor.
    virtual ~Distributed_data_subset_locality_query_mode() {}

    /// Implements the query method that applies to a distributed dataset that is refered to by an unique identifier.
    ///
    /// \return     Return the \c mi::neuraylib::Tag_struct that uniquely identifies 
    ///             the entire distributed dataset.
    /// 
    virtual const mi::neuraylib::Tag_struct& get_distributed_data_tag()           const { return m_distribute_data_tag; };

    /// Implements the spatial query method let determine data localities based on a spatial coverage.
    ///
    /// The bounding box represented by \c mi::math::Bbox_struct<mi::Float32, 3> specify a spatial 
    /// region of interest. All data subsets that are covered at least partially, will be
    /// considered in a data locality.
    ///
    /// \return     Returns the a bounding box that represents a region of interest for
    ///             determining the data locality. An invalid bounding box will be ingnored
    ///             and the space that the entire distributed dataset covers will be
    ///             considered instead. 
    ///
    virtual const mi::math::Bbox_struct<mi::Float32, 3>& get_spatial_coverage()   const { return m_spatial_coverage; };

private:
    mi::neuraylib::Tag_struct               m_distribute_data_tag;
    mi::math::Bbox_struct<mi::Float32, 3>   m_spatial_coverage;
};

/// Height field specific query modes to determine the data localities of height field patches.
///
/// Deriving a data locality from the data distribution for a height field dataset 
/// based on the 2D bounding box.
/// A data locality determined using this query method will include all data subsets that 
/// are contained inside or just intersect the region of interest.
///
/// In contrast to a sparse volume dataset (see \c ISparse_volume_scene_element) a height field
/// is defined in 2.5D and its 2D support may intersect multiple subregion in 3D space along the
/// height fields heiht-axis. 
/// Analysis techniques that merly inspect height field data, e.g., to calculate an average
/// height value only need to determine a single patch. For processing techniques that change the 
/// elevation values, all patches that along the z axis need to be considered and updated after 
/// the operation.
/// To consider both, a height field query exposes to modes: globally unique or locally unique 
/// height field data subsets.  
///
/// \ingroup nv_index_data_locality
///
class IDistributed_height_field_locality_query_mode :
    public mi::base::Interface_declare<0xc985b49c,0xdb6a,0x4808,0x94,0x19,0xe8,0x3c,0xbb,0x69,0x77,0x59,
        nv::index::IDistributed_data_locality_query_mode>
{
public:
    /// Height field data locality query modes.
    ///
    /// This mode allows to query the height-field data locality of the data subsets
    /// which are either globally unique or unique per host. Globally unique mode will
    /// reference a single subset only once, while in per-host unique mode the
    /// locality potentially references a subset multiple times, but only once per host.
    ///
    enum Height_field_locality_query_mode {
        HEIGHT_FIELD_LOCALITY_QUERY_SUBSETS_UNIQUE_CLUSTER  = 0x01u, ///<! Data locality considers globally unique data subsets only.
        HEIGHT_FIELD_LOCALITY_QUERY_SUBSETS_UNIQUE_PER_HOST = 0x02u  ///<! Data locality considers locally unique data subsets only.
    };

    /// The height field query mode defines if globally or locally unique data subsets shall be considered for the data locality.
    ///
    /// \return Returns if locally or globally unique data subsets shall be considered.
    ///  
    virtual Height_field_locality_query_mode get_query_mode()                   const = 0;
    
    /// Spatial query methods let determine data localities based on a 2D spatial coverage.
    ///
    /// The bounding box represented by \c mi::math::Bbox_struct<mi::Float32, 2> specifies 
    /// a 2D area of interest. Only data subsets that are covered at least partially, will be
    /// considered in a data locality.
    ///
    /// \return     Returns the a bounding box that represents a area of interest for
    ///             determining the data locality.
    ///
    virtual const mi::math::Bbox_struct<mi::Float32, 2>& get_spatial_coverage() const = 0;
};

/// Implements the height field specific query mode height field patches.
///
/// \ingroup nv_index_data_locality
///
class Distributed_height_field_locality_query_mode :
    public mi::base::Interface_implement<nv::index::IDistributed_height_field_locality_query_mode>
{
public:
    /// Instantiating a query method using the distributed dataset unique identifier and a given region of interest.
    ///
    /// The spatial query is defined in continous space which requires a bounding box defined 
    /// using floating-point min and max values. 
    ///
    /// \param[in] query_mode   Defines if either locally unique or cluster-wise unique height
    ///                         field data shall be considered.
    ///
    /// \param[in] tag          The \c mi::neuraylib::Tag_struct identifies the distributed dataset.
    ///
    /// \param[in] coverage     The bounding box specifies the region of interest for the locality query.
    ///
    Distributed_height_field_locality_query_mode(
        Height_field_locality_query_mode                query_mode,
        const mi::neuraylib::Tag_struct&                tag,
        const mi::math::Bbox_struct<mi::Float32, 2>&    coverage)
        : m_query_mode(query_mode),
          m_distribute_data_tag(tag),
          m_spatial_coverage(coverage) {}

    /// Instantiating a query method using the distributed dataset unique identifier and a given area of interest.
    ///
    /// The 2D spatial query is defined in continous space but the 2D area of interst here is defined 
    /// using integert extents. The bounding area is converted to a floating-point
    /// bounding area first.  
    ///
    /// \param[in] query_mode   Defines if either locally unique or cluster-wise unique height
    ///                         field data shall be considered.
    ///
    /// \param[in] tag          The \c mi::neuraylib::Tag_struct identifies the distributed dataset.
    ///
    /// \param[in] coverage     The bounding box specifies the region of interest for the locality query.
    ///
    Distributed_height_field_locality_query_mode(
        Height_field_locality_query_mode                query_mode,
        const mi::neuraylib::Tag_struct&                tag,
        const mi::math::Bbox_struct<mi::Sint32, 2>&     coverage)
        : m_query_mode(query_mode), 
          m_distribute_data_tag(tag)
    {
        // Following is still unfortunate, i.e., the need to convert to float typed bbox.
        m_spatial_coverage.min.x = static_cast<mi::Float32>(coverage.min.x);
        m_spatial_coverage.min.y = static_cast<mi::Float32>(coverage.min.y);
        m_spatial_coverage.max.x = static_cast<mi::Float32>(coverage.max.x);
        m_spatial_coverage.max.y = static_cast<mi::Float32>(coverage.max.y);
    }

    /// Implements an empty default destructor.
    virtual ~Distributed_height_field_locality_query_mode() {}

    /// implement the height field mode defines if globally or locally unique data subsets shall be considered for the data locality.
    ///
    /// \return     Returns if locally or globally unique data subsets shall be 
    ///             considered in combination with the 2D area of interest.
    /// 
    virtual Height_field_locality_query_mode get_query_mode()                     const { return m_query_mode;          }

    /// Implements the query method that applies to a height field dataset that is refered to by an unique identifier.
    ///
    /// \return     Return the \c mi::neuraylib::Tag_struct that uniquely identifies 
    ///             the height field dataset.
    /// 
    virtual const mi::neuraylib::Tag_struct& get_distributed_data_tag()           const { return m_distribute_data_tag; }

    /// Implements the spatial query method to determine data localities based on a 2D coverage.
    ///
    /// The 2D area represented by \c mi::math::Bbox_struct<mi::Float32, 2> specifies a spatial 
    /// area of interest. All data subsets that are covered at least partially, will be
    /// considered in a data locality.
    ///
    /// \return     Returns the a bounding box that represents a region of interest for
    ///             determining the data locality. An invalid bounding box will be ingnored
    ///             and the space that the entire distributed dataset covers will be
    ///             considered instead. 
    ///
    virtual const mi::math::Bbox_struct<mi::Float32, 2>& get_spatial_coverage()   const { return m_spatial_coverage;    }

private:
    Height_field_locality_query_mode        m_query_mode;
    mi::neuraylib::Tag_struct               m_distribute_data_tag;
    mi::math::Bbox_struct<mi::Float32, 2>   m_spatial_coverage;
};

/// Data locality query mode for deprecated heightfield datasets.
///
/// \deprecated The \c IRegular_heightfield has been deprecated and will be removed.
///
/// \ingroup nv_index_data_locality
///
class IRegular_heightfield_locality_query_mode :
    public mi::base::Interface_declare<0xde972a15, 0x1593, 0x4db3, 0xa0, 0x2f, 0xa, 0x3e, 0x46, 0xe0, 0x90, 0xc,
        nv::index::IDistributed_data_locality_query_mode>
{
public:
    /// Editing and a non-editing inspection mode produce different data localities.
    /// \return     Returns \c true if the data locality query shall consider all heightfield 
    ///             patches, e.g., for data editing and elevation changes operations and \c false
    ///             if only single unique patches are required for data inspection (such as 
    ///             the detection of a average height value of all elevation values).  
    virtual bool is_editing_mode() const = 0;
    /// The locality query only considers patches that cover or intersect a given 2D spatial area.
    /// \return     Returns the 2D spatial area.
    virtual const mi::math::Bbox_struct<mi::Uint32, 2>& get_spatial_coverage() const = 0;
};

/// Implements a data locality query mode for deprecated heightfield datasets.
///
/// \deprecated The \c IRegular_heightfield has been deprecated and will be removed.
///
/// \ingroup nv_index_data_locality
///
class Regular_heightfield_locality_query_mode :
    public mi::base::Interface_implement<nv::index::IRegular_heightfield_locality_query_mode>
{
public:
    /// Instantiating a query method for regular heightfield datasets.
    ///
    /// \param[in] tag          The \c mi::neuraylib::Tag_struct identifies the heighfield dataset.
    ///
    /// \param[in] coverage     The bounding box specifies the region of interest for the locality query.
    ///
    /// \param[in] editing_mode Defines if the locality mode shall consider heightfield patches 
    ///                         for editing only or for non-editing data analysis.
    ///
    Regular_heightfield_locality_query_mode(
        const mi::neuraylib::Tag_struct&                tag,
        const mi::math::Bbox_struct<mi::Uint32, 2>&     coverage,
        bool                                            editing_mode = false)
        : m_editing_mode(editing_mode),
          m_distribute_data_tag(tag),
          m_spatial_coverage(coverage) {}
    /// Implements an empty default destructor.
    virtual ~Regular_heightfield_locality_query_mode() {}

    /// Editing and a non-editing inspection mode produce different data localities.
    /// \return     Returns \c true if the data locality query shall consider all heightfield 
    ///             patches, e.g., for data editing and elevation changes operations and \c false
    ///             if only single unique patches are required for data inspection (such as 
    ///             the detection of a average height value of all elevation values).  
    virtual bool                                         is_editing_mode()          const { return m_editing_mode; }
    /// The distributed heightfield dataset
    /// \return     Returns the unique identifier that refers to the heightfield datset.
    virtual const mi::neuraylib::Tag_struct&             get_distributed_data_tag() const { return m_distribute_data_tag; }
    /// The locality query only considers patches that cover or intersect a given 2D spatial area.
    /// \return     Returns the 2D spatial area.
    virtual const mi::math::Bbox_struct<mi::Uint32, 2>&  get_spatial_coverage()     const { return m_spatial_coverage; }

private:
    bool                                    m_editing_mode;
    mi::neuraylib::Tag_struct               m_distribute_data_tag;
    mi::math::Bbox_struct<mi::Uint32, 2>    m_spatial_coverage;
};

/// Data locality information for distributed datasets.
///
/// In general, the data representation of large-scale
/// datasets is distributed in the cluster environment for efficient and 
/// parallel scalable data processing and analysis and data rendering.
/// NVIDIA IndeX's distribution scheme relies on a spatial subdivision
/// of the entire scene space. 
/// Using the subdivision scheme, the scene space is partitioned into subregion,
/// i.e., smaller-sized spatial 3D areas. Such subregion directly 
/// partition the dataset representation into 
/// data subsets. Data subsets are stored on the nodes and GPUs distributed
/// in the cluster environment. Each data subset is contained in its local
/// space bounding box inside a subregion, wich is defined in the global
/// subdivision space.
///
/// The data locality provides an application with the means to query where, i.e., on
/// which node in the cluster, a data subset of the entire
/// distributed dataset is stored. A node stores either none or a set of data 
/// subsets. Furthermore, the data locality information provides the 
/// bounding boxes that correspond each of to the data subsets
/// stored on a given cluster node or GPU. Each of the bounding boxes can be
/// accessed. The respective index then also corresponds to the distributed
/// data subset.
///
/// A common use case that requires the data locality is the invocation of
/// parallel and distributed data job that apply processing and analysis techniques
/// to the distributed data subset in parallel (see \c IDistributed_data_job).
///
/// \note   Additional more explicit convenience functionalities and methods
///         will be added to the \c IDistributed_data_locality interface class
///         soon.
///
/// \ingroup nv_index_data_locality
///
class IDistributed_data_locality :
    public mi::base::Interface_declare<0x64624ed0,0x6e2a,0x48c9,0xb9,0x73,0x61,0x8d,0x32,0xd0,0x5e,0xf5,
                                       mi::neuraylib::ISerializable>
{
public:
    /// Data subsets are distributed to a number of hosts in a cluster.
    ///
    /// A cluster node hosts subset or none of the distributed data subsets.
    /// The number of nodes that manage the data subsets of the entire
    /// distributed dataset allows, for instance, setting up compute jobs and 
    /// passing the compute executions explicitly the to nodes that store the
    /// data subsets locally.
    /// 
    /// \note Please use the number of cluster nodes to iterate over all nodes
    ///       that store data subsets.
    ///
    /// \return             The number of cluster nodes that manage a data subsets of
    ///                     the entire distributed data representation.
    ///
    virtual mi::Uint32 get_nb_cluster_nodes() const = 0;

    /// A set of data subsets on a specific cluster node with explicit host identifier.
    ///
    /// Each of the cluster node has a unique identifier. The method allows for
    /// interating over all node identiviert that that store at least one data subset.
    /// The node identifier can be used to send a job execution to the node.
    /// For instance, the \c mi::neuraylib::Fragmented_job interface allow for 
    /// explicit scheduling of job executions to nodes, i.e., node ids.
    ///
    /// \param[in] index    The index used to access one of the cluster nodes.
    ///                     The index must be given in the range from 0 to
    ///                     #get_nb_cluster_nodes()-1.
    ///
    /// \code
    /// const mi::Uint32 nb_nodes = locality->get_nb_cluster_nodes();
    /// for(mi::Uint32 node_index=0; node_index<nb_nodes; ++node_index)
    /// {
    ///     const mi::Uint32 node_id = locality->get_cluster_node();
    /// }
    /// \endcode
    ///
    /// \return             Returns the indexed unique identifier of
    ///                     a cluster node.
    ///
    virtual mi::Uint32 get_cluster_node(mi::Uint32 index) const = 0;

    /// A cluster node hosts a set of data subsets bound inside their local-space bounding box.
    ///
    /// Each subset has its own bounding box in the scene element's local space.
    /// This method returns number of buonding boxes which each representing a single 
    /// data subset stored on a the cluster node. The number of bounding box, thus,
    /// indicates the number of data subsets and allows, for instance, to schedule 
    /// and appropriate number of executions to the node. 
    /// That is, knowing the number of data subsets per cluster node 
    /// enables an application to direct and appropriate number of executions to the 
    /// nodes to implement a compute algorithms that operate on a data subset
    /// granularity. 
    ///
    /// The method is typically used to iterate over all data subset bounding box
    /// (i.e., data subsets) assigned to a a cluster node.
    ///
    /// \param[in] cluster_node_id      The unique identifier that references 
    ///                                 a node in the cluster environment.
    ///
    /// \return                         The number of bounding boxes or data subsets
    ///                                 repectively stored locally on the
    ///                                 given cluster node.
    ///
    virtual mi::Size get_nb_bounding_box(mi::Uint32 cluster_node_id) const = 0;

    /// Each data subsets is stored on a cluster node are defined inside its local-space bounding box.
    ///
    /// Use this method to iterating over all the bounding boxes or data subsets
    /// that are stored on the cluster node, e.g., to implement tailor-made 
    /// job scheduling (see also \c \c mi::neuraylib::Fragmented_job).
    ///
    /// \param[in] cluster_node_id      The id that references a cluster node.
    ///                                 The index must be given in the range
    ///                                 from 0 to #get_nb_cluster_nodes()-1.
    ///
    /// \param[in] bounding_box_index   The index of the bounding box that references
    ///                                 the actual subset of the large-scale data
    ///                                 representation.
    ///                                 The index must be given in the range
    ///                                 from 0 to #get_nb_bounding_box()-1.
    ///
    /// \code
    /// // Iterater over cluster host ids:
    /// //
    /// const mi::Uint32 nb_nodes = locality->get_nb_cluster_nodes();
    /// for(mi::Uint32 node_index=0; node_index<nb_nodes; ++node_index)
    /// {
    ///     // Iterater over data subset bounding boxes on the given host (node_index):
    ///     //
    ///     const mi::Uint32 nb_boxes = locality->get_nb_bounding_box(node_index);
    ///     for(mi::Uint32 bbox_index=0; bbox_index<nb_boxes; ++bbox_index)
    ///     {
    ///         const mi::math::Bbox_struct<mi::Sint32, 3> = locality->get_bounding_box(node_index, bbox_index);
    ///     }
    /// }
    /// \endcode
    ///
    /// \return     Returns the data subset's bounding
    ///             box in the scene element's local space.
    ///
    virtual const mi::math::Bbox_struct<mi::Sint32, 3> get_bounding_box(
        mi::Uint32 cluster_node_id,
        mi::Uint32 bounding_box_index) const = 0;

    /// Each data subsets is stored on a cluster node are defined inside its local-space bounding box.
    ///
    /// The data subset stored on a cluster node is defined inside its bounding box.
    /// The method allows iterating over all the bounding boxes
    /// on a cluster machine, e.g., to implement compute techniques.
    ///
    /// \param[in] cluster_node_id      The id that references a cluster node.
    ///                                 The index must be given in the range
    ///                                 from 0 to #get_nb_cluster_nodes()-1.
    ///
    /// \param[in] bounding_box_index   The index of the bounding box that references
    ///                                 the actual subset of the large-scale data
    ///                                 representation.
    ///                                 The index must be given in the range
    ///                                 from 0 to #get_nb_bounding_box()-1.
    ///
    /// \return                         The subregion identifier that the data subset is assigned
    ///                                 to, i.e., the spatial area that the data subset intersects and 
    ///                                 is contained in.
    ///
    virtual Subregion_identifier get_subregion(
        mi::Uint32 cluster_node_id,
        mi::Uint32 bounding_box_index) const = 0;
};

class IDistributed_data_job_scheduler;

/// Retrieving information about the data distribution and scheduling tasks against distributed data. 
/// 
/// The \c ISession exposes access to the \c IData_distribution interface class.
/// The interface class grants general overview on the data distribution, i.e., 
/// exposes the data locality for a distributed dataset or enables the application 
/// to issue distribute jobs in the cluster environment.
///
/// \note   This interface class is going to expose additional details on the data distribution
///         in the future.
///
/// \ingroup nv_index_data_distribution
///
class IData_distribution :
    public mi::base::Interface_declare<0xc37caf77,0xe632,0x46ad,0x8c,0x19,0x6d,0x57,0x56,0x04,0xda,0x68,
                                       mi::neuraylib::IElement>
{
public:
    /// Scheduler that directs distributed data jobs to nodes and GPUs for analyzing and processing large-scale datasets.
    ///
    /// \return     Returns an instance of the job scheduler interface 
    ///             \c IDistributed_data_job_scheduler that issues distributed data 
    ///             analysis and processing techniques towards cluster nodes on a 
    ///             data subset granularity.
    ///
    virtual IDistributed_data_job_scheduler* create_scheduler() const = 0;

    /// Creating a data locality for the given query method and a given distributed dataset type.
    ///
    /// Query the distribution of irregular volume data in the cluster. The returned
    /// instance of the class that implements interface \c IDistributed_data_locality
    /// provides the cluster nodes where parts of the queried data including their
    /// respective bounding boxes (brick bounding box) are stored.
    ///
    /// \param[in] class_id             Identifier of the distributed dataset type.
    ///
    /// \param[in] query_method         The query method used for determining a 
    ///                                 data locality.
    ///
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns the data locality that
    ///                                 corresponds to the query method.
    ///
    virtual IDistributed_data_locality* get_data_locality(
        const mi::base::Uuid&                           class_id,
        IDistributed_data_locality_query_mode*          query_method,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;


    /// Convenience template functions for creating a typed data locality.
    ///
    /// The template parameter T defines distributed dataset type for which the 
    /// data locality shall be determined. 
    ///
    /// \code
    /// //
    /// // Usage:
    /// //
    /// mi::base::Handle<const nv::index::IDistributed_data_locality> data_locality(
    ///     data_distribution->get_data_locality<nv::index::ISparse_volume_scene_element>(query_method, dice_transaction));
    /// \endcode
    ///
    /// \param[in] query_method         The query method used for determining a 
    ///                                 data locality.
    ///
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns a typed data locality that
    ///                                 corresponds to the query method.
    ///
    template <class T>
    IDistributed_data_locality* get_data_locality(
        IDistributed_data_locality_query_mode*          query_method,
        mi::neuraylib::IDice_transaction*               dice_transaction) const
    {
        return get_data_locality(typename T::IID(), query_method, dice_transaction);
    }

    /// Convenience template functions for creating a typed data locality.
    ///
    /// Creates a data locality for a distributed dataset and a spatial query.
    /// The template parameter T defines distributed dataset type for which the 
    /// data locality shall be determined. 
    ///
    /// \code
    /// //
    /// // Usage:
    /// //
    /// mi::base::Handle<const nv::index::IDistributed_data_locality> data_locality(
    ///     data_distribution->get_data_locality<nv::index::ISparse_volume_scene_element>(tag, bbox, dice_transaction));
    /// \endcode
    ///
    /// \param[in] tag                  Specifies the distributed dataset for which the 
    ///                                 data locality shall be determined.
    ///
    /// \param[in] query_bbox           Bounding box for specifiying a spatial query method 
    ///                                 for determining a data locality.
    ///
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns a typed data locality that
    ///                                 corresponds to the query method.
    ///
    template <class T>
    IDistributed_data_locality* get_data_locality(
        mi::neuraylib::Tag_struct                       tag,
        const mi::math::Bbox_struct<mi::Float32, 3>&    query_bbox,
        mi::neuraylib::IDice_transaction*               dice_transaction) const
    {
        mi::base::Handle<IDistributed_data_subset_locality_query_mode> selection(
            new Distributed_data_subset_locality_query_mode(tag, query_bbox));
        return get_data_locality(typename T::IID(), selection.get(), dice_transaction);
    }
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_DATA_LOCALITY_H
