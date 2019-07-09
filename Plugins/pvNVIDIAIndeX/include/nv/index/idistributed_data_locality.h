/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interfaces for exposing the data distribution scheme.

#ifndef NVIDIA_INDEX_IDATA_DISTRIBUTION_H
#define NVIDIA_INDEX_IDATA_DISTRIBUTION_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/neuraylib/iserializer.h>

#include <nv/index/idistributed_data_edit.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_data_access
/// The interface class exposes data locality information.
///
/// In general, the data representation that corresponds to a large-scale
/// dataset is distributed in the cluster environment to enable
/// scalable rendering and computing (based on a sort-last approach).
/// The distribution scheme relies on a spatial subdivision
/// of the entire scene space.
/// Using the subdivision scheme, the data representation is partitioned
/// into subsets that are distributed to and stored on the nodes in the
/// cluster. Each subset is contained in its local space bounding box.
///
/// The data locality provides the application the means to query where, i.e., on
/// which node in the cluster, a subset of the data representation of the entire
/// distributed dataset is stored. A node stores either none or a set of subsets.
/// Furthermore, the data locality information provides the number of
/// those 3D bounding boxes that correspond to the number of subsets
/// stored on a given cluster node. Each of the bounding boxes can be
/// accessed. The respective index then also corresponds to the distributed
/// data subset.
///
/// A common use case that requires the data locality is the invocation of
/// parallel and distributed compute tasks applied to the distributed data.
/// DiCE provides a fragmented job infrastructure (\c IFragmented_job) that allows
/// invoking operations in the cluster.
/// Each job invocation usually requires information about
/// the number of operations (a.k.a. fragments) that shall be launched and the
/// target cluster nodes to which the launched fragments shall be sent to
/// process or analyse the distributed data.
///
///
/// \deprecated This interface class is subject to change!
///

class IDistributed_data_locality :
    public mi::base::Interface_declare<0x64624ed0,0x6e2a,0x48c9,0xb9,0x73,0x61,0x8d,0x32,0xd0,0x5e,0xf5,
                                       mi::neuraylib::ISerializable>
{
public:
    /// The data representation is distributed to a number of machines in the cluster.
    /// A cluster node hosts none or a subset of the distributed data.
    /// The number of nodes that host a subset of the entire
    /// dataset allows, for instance, setting up the fragmented job, i.e.,
    /// the number of fragments a fragmented job shall be split into.
    ///
    /// \return             The number of cluster nodes that host a subset of
    ///                     the entire data representation.
    ///
    virtual mi::Uint32 get_nb_cluster_nodes() const = 0;

    /// The data representation is distributed to the given cluster machines.
    /// Each of the cluster machines has a unique id. The method returns the unique
    /// ids of the cluster machines that store a subset of the data locally.
    ///
    /// \param[in] index    The index used to access one of the cluster nodes.
    ///                     The index must be given in the range from 0 to
    ///                     \c get_nb_cluster_nodes().
    ///
    /// \return             The indexed unique cluster machines id.
    ///
    virtual mi::Uint32 get_cluster_node(mi::Uint32 index) const = 0;

    /// Each cluster node hosts none or a subset of the large-scale data representation.
    /// Each subset has its own bounding box in the scene element's local space.
    /// The number of subsets stored locally, i.e., the number
    /// of bounding boxes per cluster node, allows, for instance, a user-defined
    /// compute algorithm to iterate over all subsets on a cluster machines.
    ///
    /// \param[in] cluster_node_id      The id that references a cluster machine.
    ///
    /// \return                         The number of data subsets
    ///                                 stored locally on the cluster node.
    ///
    virtual mi::Size get_nb_bounding_box(mi::Uint32 cluster_node_id) const = 0;

    /// The data subset stored locally on a cluster node is defined inside its bounding box.
    /// The method allows iterating over all the bounding boxes
    /// on a cluster machine, e.g., to implement compute techniques.
    ///
    /// \param[in] cluster_node_id      The id that references a cluster node.
    ///                                 The index must be given in the range
    ///                                 from 0 to \c get_nb_bounding_box().
    ///
    /// \param[in] index                The index of the bounding box that references
    ///                                 the actual subset of the large-scale data
    ///                                 representation.
    ///
    /// \return                         The data subset's bounding
    ///                                 box in the scene element's local space.
    ///
    virtual const mi::math::Bbox_struct<mi::Sint32, 3> get_bounding_box(
        mi::Uint32 cluster_node_id,
        mi::Uint32 index) const = 0;
};

/// @ingroup nv_index_data_access
/// The interface class exposes the locality information of a distributed regular volume dataset.
///
/// The interface method \c IData_distribution::retrieve_data_locality()
/// returns the volume data locality.
///
///
/// \deprecated This interface class is subject to change!
///
class IRegular_volume_data_locality :
    public mi::base::Interface_declare<0x64624ed9,0x6e2a,0x48c9,0xa9,0x82,0x61,0x8d,0x32,0xd0,0x5e,0xf9,
                                       IDistributed_data_locality>
{
public:
    /// Creates means to edit the volume brick stored locally on a cluster node.
    /// The method may only be called on the cluster machine that stores the data.
    ///
    /// \param[in] dice_transaction     The DiCE transaction used that the tasks operates in.
    ///
    /// \param[in] cluster_node_id      The index of the cluster machine that stores
    ///                                 a certain brick data.
    ///
    /// \param[in] index                The index of the bounding box that references
    ///                                 the actual brick of the subset of bricks.
    ///
    /// \return                         Returns an NVIDIA IndeX instance of
    ///                                 the interface class \c IRegular_volume_data_edit
    ///                                 to edit the brick data contents.
    ///
    ///
    virtual IRegular_volume_data_edit* create_data_edit(
        mi::neuraylib::IDice_transaction*   dice_transaction,
        mi::Uint32                          cluster_node_id,
        mi::Uint32                          index) const = 0;

    /// Creates means to access the volume brick stored locally on a cluster machine.
    /// While the interface class \c IRegular_volume_data_access creates of copy of the
    /// present interface method exposes direct access to the brick data stored locally
    /// on a machine avoiding any data copies.
    /// The method may only be called on the cluster node that stores the data.
    ///
    /// \deprecated This call is supposed to be removed soon as
    ///             it used to be a workaround in the past.
    ///
    /// \param[in] dice_transaction     The DiCE transaction used for this operation.
    ///
    /// \param[in] cluster_node_id      The index of the cluster machine that stores
    ///                                 a brick data.
    ///
    /// \param[in] index                The index of the bounding box that references
    ///                                 the brick of the subset of bricks.
    ///
    /// \param[out] brick_bounding_box  The bounding box of the brick data stored
    ///                                 locally. The bounding box may be larger than
    ///                                 the bounding box returned by \c get_bounding_box
    ///                                 because the raw brick data may be extended by
    ///                                 an additional voxel boundary.
    ///
    /// \return                         Returns a pointer to the amplitude values of the
    ///                                 local brick data.
    ///
    virtual mi::Uint8* access_local_data(
        mi::neuraylib::IDice_transaction*       dice_transaction,
        mi::Uint32                              cluster_node_id,
        mi::Uint32                              index,
        mi::math::Bbox_struct<mi::Uint32, 3>&   brick_bounding_box) const = 0;
};

/// @ingroup nv_index_data_access
/// The interface class exposes the locality information of a distributed sparse-volume dataset.
///
/// The interface method \c IData_distribution::retrieve_data_locality()
/// returns the volume data locality.
///
/// \deprecated This interface class is subject to change!
///
class ISparse_volume_data_locality :
    public mi::base::Interface_declare<0x860724fe,0x5c07,0x43b3,0xaa,0xc8,0xbc,0x3,0x72,0xc8,0x44,0xf8,
                                       IDistributed_data_locality>
{
public:
    /// Creates means to edit the sparse-volume subset stored locally on a cluster node.
    /// The method may only be called on the cluster machine that stores the data.
    ///
    /// \param[in] dice_transaction     The DiCE transaction used that the tasks operates in.
    ///
    /// \param[in] cluster_node_id      The index of the cluster machine that stores
    ///                                 a certain subset data.
    ///
    /// \param[in] index                The index of the bounding box that references
    ///                                 the actual sparse-volume subset
    ///
    /// \return                         Returns an NVIDIA IndeX instance of
    ///                                 the interface class \c ISparse_volume_data_edit
    ///                                 to edit the sparse-volume subset data contents.
    ///
    virtual ISparse_volume_data_edit* create_data_edit(
        mi::neuraylib::IDice_transaction*   dice_transaction,
        mi::Uint32                          cluster_node_id,
        mi::Uint32                          index) const = 0;
};

/// @ingroup nv_index_data_access
/// The interface class exposes the locality information of a distributed height-field dataset.
///
/// The interface method \c IData_distribution::retrieve_data_locality()
/// returns the height-field data locality.
///
/// \deprecated This interface class is subject to change!
///
class IHeight_field_data_locality :
    public mi::base::Interface_declare<0xb7e4caa4,0x36d7,0x4712,0xba,0x91,0x89,0x9d,0x70,0x3e,0x4c,0x7a,
                                       IDistributed_data_locality>
{
public:
};

/// @ingroup nv_index_data_access
/// The interface class exposes the locality information of a distributed regular heightfield dataset.
///
/// The interface method \c IData_distribution::retrieve_data_locality()
/// returns the volume data locality.
///
///
/// \deprecated This interface class is subject to change!
///
class IRegular_heightfield_data_locality :
    public mi::base::Interface_declare<0xe40bd2c9,0x0d82,0x4e03,0x9c,0x46,0x42,0x87,0x86,0xbb,0xee,0xdf,
                                       IDistributed_data_locality>
{
public:
    /// Creates means to edit the heightfield data stored locally on a cluster machine.
    /// The method may only be called on the cluster node that stores the data.
    ///
    /// \param[in] dice_transaction     The DiCE transaction used for this operation.
    /// \param[in] cluster_node_id      The index of the cluster node that stores
    ///                                 a certain patch data.
    /// \param[in] index                The index of the bounding box that references
    ///                                 the actual patch of the sub set of patches.
    ///
    /// \return                         Returns an instance of an implementation of
    ///                                 the interface class \c IRegular_heightfield_data_edit.
    ///
    ///
    virtual IRegular_heightfield_data_edit* create_data_edit(
        mi::neuraylib::IDice_transaction*   dice_transaction,
        mi::Uint32                          cluster_node_id,
        mi::Uint32                          index) const = 0;
};

/// @ingroup nv_index_data_access
/// The interface class exposes the locality information of a distributed regular volume dataset.
///
/// The interface method \c IData_distribution::retrieve_data_locality()
/// returns the volume data locality.
///
///
/// \deprecated This interface class is subject to change!
///
class IIrregular_volume_data_locality :
    public mi::base::Interface_declare<0x1a783273,0xea49,0x4f86,0x97,0x44,0x24,0x5b,0xb5,0xef,0xe1,0x9d,
                                       IDistributed_data_locality>
{
public:
    /// Creates means to edit the volume brick stored locally on a cluster node.
    /// The method may only be called on the cluster machine that stores the data.
    ///
    /// \param[in] dice_transaction     The DiCE transaction used that the tasks operates in.
    ///
    /// \param[in] cluster_node_id      The index of the cluster machine that stores
    ///                                 a certain brick data.
    ///
    /// \param[in] index                The index of the bounding box that references
    ///                                 the actual brick of the subset of bricks.
    ///
    /// \return                         Returns an NVIDIA IndeX instance of
    ///                                 the interface class \c IRegular_volume_data_edit
    ///                                 to edit the brick data contents.
    ///
    ///
    virtual IIrregular_volume_data_edit* create_data_edit(
        mi::neuraylib::IDice_transaction*   dice_transaction,
        mi::Uint32                          cluster_node_id,
        mi::Uint32                          index) const = 0;

    /// The irregular volume data subset stored locally on a cluster node is
    /// defined inside its bounding box.
    /// The method allows iterating over all the bounding boxes
    /// on a cluster machine, e.g., to implement compute techniques.
    ///
    /// \param[in] cluster_node_id      The id that references a cluster node.
    ///                                 The index must be given in the range
    ///                                 from 0 to \c get_nb_bounding_box().
    ///
    /// \param[in] index                The index of the bounding box that references
    ///                                 the actual subset of the large-scale data
    ///                                 representation.
    ///
    /// \return                         The data subset's bounding
    ///                                 box in the scene element's local space.
    ///
    virtual const mi::math::Bbox_struct<mi::Float32, 3> get_data_subset_bounding_box(
        mi::Uint32 cluster_node_id,
        mi::Uint32 index) const = 0;
};

/// @ingroup nv_index_data_access
/// Interface class that exposes the data distribution in the cluster environment.
///
/// \deprecated This interface class is subject to change!
///
class IData_distribution :
    public mi::base::Interface_declare<0xc37caf77,0xe632,0x46ad,0x8c,0x19,0x6d,0x57,0x56,0x04,0xda,0x68,
                                       mi::neuraylib::IElement>
{
public:
    /// Query the distribution of volume data in the cluster. The returned
    /// instance of the class that implements interface \c IRegular_volume_data_locality
    /// provides the cluster nodes where parts of the queried data including their
    /// respective bounding boxes (brick bounding box) are stored.
    ///
    /// \param[in] scene_element_tag    The volume scene element tag that refers to
    ///                                 data representation whose distribution shall
    ///                                 be returned.
    /// \param[in] query_bbox           The bounding box in scene element's local space.
    ///                                 The data distribution will be exposed for the
    ///                                 user-defined bounding box passed to the query.
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns the data distribution scheme that
    ///                                 corresponds to the query parameters.
    ///
    virtual IRegular_volume_data_locality* retrieve_data_locality(
        mi::neuraylib::Tag_struct                       scene_element_tag,
        const mi::math::Bbox_struct<mi::Uint32, 3>&     query_bbox,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Query the distribution of sparse-volume data in the cluster. The returned
    /// instance of the class that implements interface \c ISparse_volume_data_locality
    /// provides the cluster nodes where parts of the queried data including their
    /// respective bounding boxes (subset bounding box) are stored.
    ///
    /// \param[in] scene_element_tag    The sparse-volume scene element tag that refers to
    ///                                 data representation whose distribution shall
    ///                                 be returned.
    /// \param[in] query_bbox           The bounding box in scene element's local space.
    ///                                 The data distribution will be exposed for the
    ///                                 user-defined bounding box passed to the query.
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns the data distribution scheme that
    ///                                 corresponds to the query parameters.
    ///
    virtual ISparse_volume_data_locality* retrieve_sparse_volume_data_locality(
        mi::neuraylib::Tag_struct                       scene_element_tag,
        const mi::math::Bbox_struct<mi::Sint32, 3>&     query_bbox,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Height-field data locality query mode.
    ///
    /// This mode allows to query the height-field data locality of the data subsets
    /// which are either globally unique or unique per host. Globally unique mode will
    /// reference a single subset subset only once, while in per-host unique mode the
    /// locality potentially references a subset multiple times, but only once per host.
    ///
    enum Height_field_locality_query_mode {
        HFLOCALITY_SUBSETS_UNIQUE_GLOBAL    = 0x01u,
        HFLOCALITY_SUBSETS_UNIQUE_PER_HOST  = 0x02u
    };
    /// Query the distribution of sparse-volume data in the cluster. The returned
    /// instance of the class that implements interface \c ISparse_volume_data_locality
    /// provides the cluster nodes where parts of the queried data including their
    /// respective bounding boxes (subset bounding box) are stored.
    ///
    /// \param[in] scene_element_tag    The sparse-volume scene element tag that refers to
    ///                                 data representation whose distribution shall
    ///                                 be returned.
    /// \param[in] query_bbox           The bounding box in scene element's local space.
    ///                                 The data distribution will be exposed for the
    ///                                 user-defined bounding box passed to the query.
    /// \param[in] query_mode           Height field data locality query mode. 
    ///                                 See Height_field_locality_query_mode.
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns the data distribution scheme that
    ///                                 corresponds to the query parameters.
    ///
    virtual IHeight_field_data_locality* retrieve_height_field_data_locality(
        mi::neuraylib::Tag_struct                       scene_element_tag,
        const mi::math::Bbox_struct<mi::Sint32, 2>&     query_bbox,
        Height_field_locality_query_mode                query_mode,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Query the distribution of heightfield data in the cluster. The returned
    /// instance of the class that implements interface \c IRegular_heightfield_data_locality
    /// provides the cluster nodes where parts of the queried data including their
    /// respective 2D bounding boxes (patch bounding box) are stored.
    ///
    /// \param[in] scene_element_tag    The heightfield scene element tag that refers to
    ///                                 data representation whose distribution shall be
    ///                                 returned.
    /// \param[in] query_patch          The bounding box in scene element's local space.
    ///                                 The data distribution will be exposed for the
    ///                                 user-defined bounding box passed to the query.
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns the data distribution scheme that
    ///                                 corresponds to the query parameters.
    ///
    virtual IRegular_heightfield_data_locality* retrieve_data_locality(
        mi::neuraylib::Tag_struct                       scene_element_tag,
        const mi::math::Bbox_struct<mi::Uint32, 2>&     query_patch,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Query the distribution of heightfield data in the cluster. The returned
    /// instance of the class that implements interface \c IRegular_heightfield_data_locality
    /// provides the cluster nodes where parts of the queried data including their
    /// respective 2D bounding boxes (patch bounding box) are stored.
    /// Distributed heightfield patches may cover the same heightfield data area and
    /// stored on multiple subregions along the z-direction (depth) in the scene.
    /// That is, this query in contrast to the previous one returns all heightfield patches
    /// in all intersected subregions, e.g., for applying cluster-wide editing
    /// operations.
    ///
    /// \param[in] scene_element_tag    The heightfield scene element tag that refers to
    ///                                 data representation whose distribution shall be
    ///                                 returned.
    /// \param[in] query_bbox           The bounding box in scene element's local space.
    ///                                 The data distribution will be exposed for the
    ///                                 user-defined bounding box passed to the query.
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns the data distribution scheme that
    ///                                 corresponds to the query parameters.
    ///
    virtual IRegular_heightfield_data_locality* retrieve_data_locality_for_editing(
        mi::neuraylib::Tag_struct                       scene_element_tag,
        const mi::math::Bbox_struct<mi::Uint32, 2>&     query_bbox,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Query the distribution of irregular volume data in the cluster. The returned
    /// instance of the class that implements interface \c IRegular_volume_data_locality
    /// provides the cluster nodes where parts of the queried data including their
    /// respective bounding boxes (brick bounding box) are stored.
    ///
    /// \param[in] scene_element_tag    The volume scene element tag that refers to
    ///                                 data representation whose distribution shall
    ///                                 be returned.
    /// \param[in] query_bbox           The bounding box in scene element's local space.
    ///                                 The data distribution will be exposed for the
    ///                                 user-defined bounding box passed to the query.
    /// \param[in] dice_transaction     The DiCE transaction that the operation runs in.
    ///
    /// \return                         Returns the data distribution scheme that
    ///                                 corresponds to the query parameters.
    ///
    virtual IIrregular_volume_data_locality* retrieve_data_locality(
        mi::neuraylib::Tag_struct                       scene_element_tag,
        const mi::math::Bbox_struct<mi::Float32, 3>&    query_bbox,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IDATA_DISTRIBUTION_H
