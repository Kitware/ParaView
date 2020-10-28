/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interfaces for accessing distributed data.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_DATA_ACCESS_H
#define NVIDIA_INDEX_IDISTRIBUTED_DATA_ACCESS_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/iheight_field_subset.h> // required by deprecated interface classes

#include <nv/index/idistributed_data_subset.h>

namespace nv
{
namespace index
{

/// Interface class enabling custom distributed data accesses.
///
/// The access functionality facilitates an application to 
/// operate and make use of the data distributed and uploaded  
/// by NVIDIA IndeX to GPUs and nodes in the cluster environment. 
/// User-defined computing algorithms, analyis techniques,
/// data export strategies of
/// todays or future workflows can make use of the distributed data 
/// through the distributed data access interfaces.
///
/// Distributed data can be queried from the
/// cluster environment. A query bounding box provided by
/// the application developer defines the extent used for 
/// accessing distributed data.
/// The NVIDIA IndeX library then manages the cluster-wide
/// data access and returns a local copy of the data to the caller.
/// The size of the bounding box defines the amount of
/// data that is routed through the network and the amount of
/// memory allocated for the local data copy.
/// Typically, the available system memory on the calling node constrains 
/// the amount of data that an application may request by means of the
/// access functionality. The network bandwidth has an effect on the 
/// the performance the 
/// Distributed data export functionalities rely on
/// the access functionality. An export strategy may, for instance,
/// choose to either query all distributed data of a dataset at once
/// or multiple smaller sized data subsets in sequential order
/// depending on the network and system memory contraints.
///
/// The interface class \c IDistributed_data_access_factory returns
/// an interface specific to a distributed data scene element type
/// and referred to by the element's tag.
///
/// \ingroup nv_index_data_access
///
class IDistributed_data_access :
    public mi::base::Interface_declare<0xd9ff4bf,0xa458,0x44f4,0x8f,0x44,0xd4,0x16,0x4c,0x14,0x4d,0x99>
{
public:
    /// Accessing a subset of a distributed dataset.
    ///
    /// The access query relies on the user-defined bounding box
    /// and creates a local copy of the data. The query
    /// bounding box may be larger than the dataset uploaded
    /// to the cluster.
    ///
    /// \param[in] query_bbox           Specifies the query bounding box to perform
    ///                                 the distributed data access. The
    ///                                 bounding box is defined in the dataset's
    ///                                 local space.
    /// \param[in] dice_transaction     The DiCE transaction active when 
    ///                                 launching the distributed data access.
    ///
    /// \return                         Returns a status code. The access operation succeeded, 
    ///                                 if the returned values is larger-equal 0, otherwise
    ///                                 the access procedure failed.
    ///
    virtual mi::Sint32 access(
        const mi::math::Bbox_struct<mi::Float32, 3>&  query_bbox,
        mi::neuraylib::IDice_transaction*             dice_transaction) = 0;

    /// Get the determined bounding box that bounds the accessed distributed data.
    ///
    /// The determined bounding box may differ from the one used for 
    /// querying the distributed data using the /c access call,
    /// e.g., if all or part of the distributed data resides outside the
    /// user-defined bounding box.
    ///
    /// \returns        The bounding box of the accessed distributed data subset.
    ///                 The bounding box is defined in the local space of the 
    ///                 scene element.
    ///
    virtual const mi::math::Bbox_struct<mi::Float32, 3>& get_bounding_box() const = 0;

    /// The volume scene element that corresponds to the accessed distributed dataset.
    ///
    /// \returns        The unique tag that references the distributed data
    ///                 scene element.
    ///
    virtual mi::neuraylib::Tag_struct get_scene_element() const = 0;


    /// Returns the accessed and assembled distributed data subset.
    ///
    /// The extent of the stored data corresponds to the determined bounding box.
    /// The method returns a pointer to an \c IDistributed_data_subset
    /// interface. 
    /// For accessing the datasets specific data, such as voxel values, 
    /// the interface class needs to be cast to a concrete data subset, e.g., 
    /// \c ISparse_volume_subset.
    /// A \c IDistributed_data_subset interface class owns the
    /// data and shall only grant access to the data.
    /// In particular, the destructor of the implemented interface
    /// class deletes the local data. 
    ///
    /// \returns        Interface pointer to an \c IDistributed_data_subset instance
    ///                 giving access a datasets data.
    ///
    virtual const IDistributed_data_subset* get_distributed_data_subset() const = 0;
};

/// Interface class for accessing the distributed heightfield data.
///
/// The access functionality, for instance, allows implementing
/// user-defined computing algorithms operating on the distributed
/// elevation data to facilitate todays and future workflow
/// functionalities.
///
/// The amount of elevation values queried from the
/// cluster environment relies on the 2D bounding box or patch given
/// by the user. The NVIDIA IndeX library then manages the
/// cluster-wide data access and returns a local copy of the
/// heightfield's elevation values inside the requested extent.
/// The size of the bounding box affects the amount of
/// elevation values that needs to be routed through the network
/// and the size of the memory allocated for the local data copy.
/// Since network bandwidth is limited and main memory is a
/// scarce resource care needs to be taken when using the
/// access functionality.
/// The elevation export functionalities, which also rely on
/// the access functionality, query multiple smaller sized data
/// chucks in sequential order rather than accessing large
/// amounts of data at once.
///
/// The interface class \c IDistributed_data_access_factory returns
/// an interface specific to a regular heightfield referred to by
/// the scene element's tag.
///
/// \deprecated This class shall be removed as the deprecated regular
///             height field data shall be removed as well.
///
/// \ingroup nv_index_data_access
///
class IRegular_heightfield_data_access :
    public mi::base::Interface_declare<0x5f7ac66c,0x5af7,0x4af9,0xa3,0xe8,0x8f,0xac,0xf0,0x08,0x9f,0x4c>
{
public:
    /// Querying the elevation values of a heightfield dataset.
    /// The query relies on the user-defined 2D bounding box
    /// and creates a local copy of the height data. The
    /// bounding box may be larger than the height data uploaded
    /// to the cluster. In such a case, the access returns the
    /// data contained in the both the user-defined
    /// bounding box and extent that bounds the uploaded heightfield.
    ///
    /// \param[in] query_bbox           Defines the 2D bounding box to perform
    ///                                 the heightfield data access query. The
    ///                                 bounding box is defined in the
    ///                                 scene element's local 3D space.
    /// \param[in] dice_transaction     The DiCE transaction that the
    ///                                 data access runs in.
    ///
    /// \return a negative value on error.
    ///
    virtual mi::Sint32 access(
        const mi::math::Bbox_struct<mi::Uint32, 2>&   query_bbox,
        mi::neuraylib::IDice_transaction*             dice_transaction) = 0;

    /// Getting the computed 2D bounding box in which the accessed elevation
    /// data is defined. The computed bounding box may be different
    /// from the bounding box used to query the heightfield data if,
    /// for instance, all or part uploaded data lies outside the
    /// user-defined bounding box.
    ///
    /// \return         The bounding box of the accessed
    ///                 data. The bounding box is defined in the
    ///                 scene element's local 3D space.
    ///
    virtual const mi::math::Bbox_struct<mi::Uint32, 2>& get_patch_bounding_box() const = 0;

    /// The scene element that corresponds to the accessed data.
    ///
    /// \return         The unique tag that references the heightfield
    ///                 scene element.
    ///
    virtual mi::neuraylib::Tag_struct get_scene_element() const = 0;

    /// The accessed heightfield elevation values stored locally. The
    /// extent of the stored values is defined by the computed 2D
    /// bounding box.  The method returns the pointer to the elevation
    /// value array.  The interface class 'owns' the elevation values,
    /// i.e., the destructor of the implemented interface class
    /// deletes the local data.
    ///
    /// \return         The pointer to the first element of the accessed
    ///                 elevation values. The height values are defined
    ///                 in J-first and I-last order.
    ///
    virtual mi::Float32* get_elevation_values() const = 0;

    /// The accessed heightfield normal vector values that correspond
    /// to the heightfield's elevation values. The normal vector
    /// values are stored locally. The extent of the stored values is
    /// defined by the computed 2D bounding box.  The method returns
    /// the pointer to the normal vector value array.  The interface
    /// class 'owns' the normal values, i.e., the destructor of the
    /// implemented interface class deletes the local data.
    ///
    /// \return         The pointer to the first element of the accessed
    ///                 normal values. Each normal corresponds to a height
    ///                 value, i.e., the normal values are defined
    ///                 in J-first and I-last order.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 3>* get_normal_values() const = 0;
};
//
//
// -------------------------------------------------------------------

/// Interface class that exposes distributed data access interfaces
/// for specific scene element.
///
/// This interface class is exposed through the \c ISession.
///
/// \ingroup nv_index_data_access
///
class IDistributed_data_access_factory :
    public mi::base::Interface_declare<0xc77ead83,0xf351,0x4160,0x8c,0x15,0x40,0xbc,0x86,0x21,0x4e,0xf0,
    mi::neuraylib::IElement>
{
public:
    /// Exposes an interface class that allows accessing distributed heightfield data.
    ///
    /// \param[in] scene_element_tag            The unique tag that references the
    ///                                         heightfield scene element for which the
    ///                                         access interface shall be exposed.
    ///
    /// \return                                 The interface that enables accessing
    ///                                         heightfield data distributed in the
    ///                                         cluster environment.
    ///
    /// \deprecated                             The \c IRegular_heightfield interface
    ///                                         will be removed in the future
    ///
    virtual IRegular_heightfield_data_access* create_regular_heightfield_data_access(
        mi::neuraylib::Tag_struct scene_element_tag) const = 0; 

    /// Creates an data access functionality specific for a distributed data type.
    ///
    /// The instance is an implementation of the interface classes derived from
    /// \c IDistributed_data_access. The factory creates these
    /// instances based on just the UUID of distributed data class, such as
    /// \c nv::index::ISparse_volume_scene_element.
    ///
    /// \param[in] dataset_type             The UUID of the interface class that represents
    ///                                     a distributed dataset.
    /// 
    /// \param[in] scene_element_tag        The unique tag that references the
    ///                                     scene element of the distributed data type.
    ///
    /// \return                             Returns the created distribute data access instance 
    ///                                     for a specific distribute dataset type and element.
    ///
    virtual IDistributed_data_access* create_distributed_data_access(
        const mi::base::Uuid&               dataset_type,
        mi::neuraylib::Tag_struct           scene_element_tag) const = 0;

    /// Convenience function for creating data access for a given distributed dataset type.
    ///
    /// The instance is an implementation of the interface classes derived from
    /// \c IDistributed_data_access. The factory creates these
    /// instances based on just the UUID of distributed data class, such as
    /// \c nv::index::ISparse_volume_scene_element.
    ///
    /// The template parameter defines the distributed dataset type of a large-scale dataset.
    /// \code
    /// //
    /// // Usage:
    /// //
    /// mi::base::Handle<nv::index::IDistributed_data_access> volume_data_access(
    ///     access_factory->create_distributed_data_access<nv::index::ISparse_volume_scene_element>(volume_tag));
    /// \endcode
    /// 
    /// \param[in] scene_element_tag        The unique tag that references the
    ///                                     scene element of the distributed data type.
    ///
    /// \return                             Returns the created distribute data access instance 
    ///                                     for a specific distribute dataset type and element.
    ///
    template <class T>
    IDistributed_data_access* create_distributed_data_access(
        mi::neuraylib::Tag_struct           scene_element_tag) const
    {
        IDistributed_data_access* t = static_cast<IDistributed_data_access*>(create_distributed_data_access(typename T::IID(), scene_element_tag));
        return t;
    }
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_DATA_ACCESS_H
