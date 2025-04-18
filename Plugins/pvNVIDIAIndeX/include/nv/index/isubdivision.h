/******************************************************************************
* Copyright 2025 NVIDIA Corporation. All rights reserved.
*****************************************************************************/
/// \file
/// \brief  Spatial subdivision information.

#ifndef NVIDIA_INDEX_ISUBDIVISION_H
#define NVIDIA_INDEX_ISUBDIVISION_H

#include <mi/math/bbox.h>
#include <mi/neuraylib/dice.h>

namespace nv {
namespace index {

/// Spatial subdivision.
class ISubdivision :
    public mi::base::Interface_declare<0x470b9032,0x21f5,0x4f4f,0xa7,0xae,0xb8,0xe9,0x48,0xae,0x86,0x50,
                                       mi::neuraylib::ISerializable>
{
public:
    /// Provides the number of spatial regions that span the spatial decomposition scheme.
    virtual mi::Uint32  get_nb_subregions() const = 0;

    /// Returns bounding box of subregion with given index. 
    /// \param[in]  index       The index must be in the range [0,\c get_nb_subregions()).
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_subregion(mi::Uint32 index) const = 0;

    // Type of subregion-id will change to Subregion_identifier, when it was converted to 64 bit.
    // A value of 0 represent an invalid id.
    typedef   mi::Uint64            Subregion_id;
    //typedef Subregion_identifier  Subregion_id; // \see idistributed_data_locality.h

    /// Returns identifier of subregion with given index. 
    /// A value of 0 represent an invalid id.
    ///
    /// \param[in]  index       The index must be in the range [0,\c get_nb_subregions()).
    virtual Subregion_id    get_subregion_id(mi::Uint32 index) const = 0;

    /// Returns weight of subregion with given index. 
    /// A weight of 0 signifies an empty subregion.
    /// A negative value means that the real weight is unknown.
    /// A positive value represents an abstract render/memory cost value of a non-empty subregion.
    /// The weight of a spatial subregion is the sum of all estimated weights of distributed data elements,
    /// \see IDistributed_data_import_callback::estimate().
    /// Adaptive subdivision schemes use the weight to generate balanced subregions.
    ///
    /// \param[in]  index       The index must be in the range [0,\c get_nb_subregions()).
    virtual mi::Float64     get_subregion_weight(mi::Uint32 index) const = 0;
};

/// Subdivision with topology information.
///
/// This interface extends \c ISubdivision by providing topology information
/// for efficient subregion sorting.
///
class ISubdivision_topology :
    public mi::base::Interface_declare<0x1fb9897d,0x1f7a,0x4f47,0xb0,0xea,0xbd,0x2b,0x68,0x67,0x4f,0xd7,
                                       nv::index::ISubdivision>
{
public:
    /// Subdivision schemes can rely on different topologies
    enum Topology_type 
    {
        TOPO_KD_TREE,   ///<! Kd-tree based topology.
        TOPO_OCTREE     ///<! Octree-based topology.
    };

    /// Get type of topology, \see Topology_type. If not supported, topology will be ignored.
    virtual mi::Uint32 get_topology_type() const = 0;

    /// Get the total number of nodes of the topology.
    /// It is assumed that the first node is the root node.
    virtual mi::Uint32 get_nb_nodes() const = 0;

    /// Get the bounding box of the node \c inode.
    /// \param inode    The node's index value.
    /// \return         Returns the bounding box of the node.
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_node_box(mi::Uint32 inode) const = 0;

    /// Get the number of children of node \c inode.
    /// Note that this expresses the number of available child slots, 
    /// but not the number of valid children.
    /// This means for a Kd-tree, you can always return 2, and for an Octree 8.
    /// \param inode    The node's index value.
    virtual mi::Uint32 get_node_child_count(mi::Uint32 inode) const = 0;

    /// Get a child index of the node \c inode. Return -1 if no child at given \c ichild slot.
    /// \param inode Index of node.
    /// \param ichild Index of child slot.
    virtual mi::Sint32 get_node_child(mi::Uint32 inode, mi::Uint32 ichild) const = 0;

    /// Get index of subregion associated with node \c inode (or -1, if no subregion).
    /// The subregions are provided by the IDomain_specific_subdivision interface.
    /// \param inode Index of node.
    virtual mi::Sint32 get_node_subregion_index(mi::Uint32 inode) const = 0;
};

}} // namespace nv::index

#endif
