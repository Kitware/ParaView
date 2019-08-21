/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief The interface class for implementing import callbacks used for distributed large-scale data chunk loading.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_DATA_IMPORT_CALLBACK_H
#define NVIDIA_INDEX_IDISTRIBUTED_DATA_IMPORT_CALLBACK_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include "idistributed_data_subset.h"

namespace nv
{
namespace index
{

/// The interface class enables the distributed cluster-wide uploading of
/// large-scale data, such as 3D volumetric data or 2.5D height field surface data.
/// In general, the distributed, parallel rendering and computing algorithms are based
/// on a spatial decomposition scheme that subdivides the 3D space into subregions.
/// Each of these subregions contains a subset of the entire dataset.
/// The distributed data import callback implement the upload of the subset data 
/// contained in subregions.
///
/// Derived classes typically declare additional member variables. 
/// For instance, importer that read the data from a file server
/// require the file and directory names where a dataset is
/// stored for uploading.
///
/// Consequently, a derived class has to implement the methods of the
/// (\c mi::neuraylib::ISerializable) interface class properly, i.e.,
/// care needs to be taken to correctly
/// serialize and deserialize the additional member variables to ensure
/// correct network transfer of the class's representation.
///
/// Whenever the subregion that contains part of the entire data is processed by
/// the distributed rendering and computing solution, the NVIDIA IndeX library
/// calls the interface method \c create() and passes a bounding box, The bounding
/// box is given in the dataset's local space and defines the 3D area for which 
/// the data of the dataset subset need to be imported.
///
/// @ingroup nv_index_data_storage
class IDistributed_data_import_callback :
    public mi::base::Interface_declare<0x61a87e48,0x910a,0x4f48,0xba,0xca,0x04,0x3b,0x9b,0xe2,0x62,0xec,
                                       mi::neuraylib::ISerializable>
{
public:
    /// Shall provide the size of the dataset that is contained in the given bounding box.
    /// The size should approximate the number of primary items in the bounding box (e.g. voxels, cells, pixels).
    /// If the size is zero, the bounding box region will be seen as empty, and not considered for further processing.
    ///
    /// \param[in] bounding_box             The bounding box of the subset in the dataset's local space.
    /// \param[in] dice_transaction         The DiCE database transaction that the
    ///                                     operation runs in.
    ///
    /// \return                             Returns the approximate number of items contained in the given 3D area.
    ///
    virtual mi::Size estimate(
        const mi::math::Bbox_struct<mi::Float32, 3>&    bounding_box,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Shall create and provide the dataset subset contained in the given bounding box.
    ///
    /// \param[in] bounding_box             The 3D area for which data shall be imported. The bounding
    ///                                     box is given in the dataset's local space.
    /// \param[in] factory                  A factory class that creates a dataset subset of a given
    ///                                     kind on request (see also \c subset_id()).
    /// \param[in] dice_transaction         The DiCE database transaction that the operation runs in.
    ///                                     The transaction may be invalid indicating an asynchronous 
    ///                                     data import operation.
    ///
    /// \return                             Returns the portion of the dataset contained in the
    ///                                     3D area.
    ///
    virtual IDistributed_data_subset* create(
        const mi::math::Bbox_struct<mi::Float32, 3>&    bounding_box,
        IData_subset_factory*                           factory,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Shall create and provide the dataset subset contained in the given bounding box for a given time step.
    ///
    /// \param[in] bounding_box             The 3D area for which data shall be imported. The bounding
    ///                                     box is given in the dataset's local space.
    /// \param[in] time_step                The time step for which data shall be imported.
    /// \param[in] factory                  A factory class that creates a dataset subset of a given
    ///                                     kind on request (see also \c subset_id()).
    /// \param[in] dice_transaction         The DiCE database transaction that the operation runs in.
    ///                                     The transaction may be invalid indicating an asynchronous 
    ///                                     data import operation.
    ///
    /// \return                             Returns the portion of the dataset contained in the
    ///                                     3D area.
    ///
    virtual IDistributed_data_subset* create(
        const mi::math::Bbox_struct<mi::Float32, 3>&    bounding_box,
        mi::Uint32                                      time_step,
        IData_subset_factory*                           factory,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Shall provide the interface id that tells the NVIDIA IndeX library the kind of
    /// \c IDistributed_data_subset that the importer supports.
    /// For instance, voxel data of a volume brick (\c IRegular_volume_brick) can represent unsigned int 8-bit values
    /// (\c IRegular_volume_brick_8bit) 32-bit float values (\c IRegular_volume_brick_float32), or RGBA color values.
    /// The library has to be aware of the kind of data to manage the internal data representation
    /// and memory allocations. 
    /// The factory class is able to create the appropriate instance of the data subset representation. 
    ///
    /// \return             The UUID represent the type of the distributed data subset.
    ///
    virtual mi::base::Uuid subset_id() const = 0;

    /// Returns optional configuration settings that may be used by the library
    /// for the session export mechanism provided by \c
    /// ISession::export_session().
    ///
    /// The returned string shall contain individual setting entries separated by
    /// newlines ('\n'). Each entry is a key value pair separated by an equal
    /// sign ('='). Entries without an equal sign are considered as comments.
    ///
    /// Example: "key1=value1\ncomment\nkey2=value2\n"
    ///
    /// \return             String representation of the configuration settings,
    ///                     the callee keeps ownerships.
    ///
    virtual const char* get_configuration() const = 0;
};

/// This mixin class can be used to implement the IDistributed_data_import_callback interface.
///
/// This mixin class provides a default implementation of some of the pure virtual methods of the
/// IDistributed_data_import_callbacks. The documentation here just lists the behavior of the
/// default implementation, see IDistributed_data_import_callback for the documentation of the
/// methods themselves.
/// @ingroup nv_index_data_storage
template <mi::Uint32 id1, mi::Uint16 id2, mi::Uint16 id3,
          mi::Uint8 id4, mi::Uint8 id5, mi::Uint8 id6, mi::Uint8 id7,
          mi::Uint8 id8, mi::Uint8 id9, mi::Uint8 id10, mi::Uint8 id11,
          class I = IDistributed_data_import_callback>
class Distributed_discrete_data_import_callback :
    public mi::neuraylib::Base<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11,I>
{
public:
    /// Shall provide the size of the dataset that is contained in the given bounding box.
    /// The size should approximate the number of primary items in the bounding box (e.g. voxels, cells, pixels).
    /// If the size is zero, the bounding box region will be seen as empty, and not considered for further processing.
    ///
    /// \param[in] bounding_box             The bounding box of the subset in the dataset's local space.
    /// \param[in] dice_transaction         The DiCE database transaction that the
    ///                                     operation runs in.
    ///
    /// \return                             Returns the approximate number of items contained in the given 3D area.
    ///
    virtual mi::Size estimate(
        const mi::math::Bbox_struct<mi::Sint32, 3>&     bounding_box,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Implementation maps floating-point bounding box to a signed integer bounding box and call respective variant.
    ///
    /// \param[in] bounding_box             The bounding box of the subset in the dataset's local space.
    ///
    /// \param[in] dice_transaction         he DiCE database transaction that the
    ///                                     operation runs in.
    ///
    /// \return                             Returns the approximate number of items contained in the given 3D area.
    ///
    virtual mi::Size estimate(
        const mi::math::Bbox_struct<mi::Float32, 3>&    bounding_box,
        mi::neuraylib::IDice_transaction*               dice_transaction) const /*override final*/
    {
        mi::math::Bbox_struct<mi::Sint32, 3> bbox;
        bbox.min.x = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.x));
        bbox.min.y = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.y));
        bbox.min.z = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.z));
        bbox.max.x = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.x));
        bbox.max.y = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.y));
        bbox.max.z = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.z));
        return estimate(bbox, dice_transaction);
    }

    using mi::neuraylib::Base<id1, id2, id3, id4, id5, id6, id7, id8, id9, id10, id11, I>::create;

    /// Shall create and provide the dataset subset contained in the given bounding box.
    ///
    /// \param[in] bounding_box             The 3D area for which data shall be imported. The bounding
    ///                                     box is given in the dataset's local space.
    /// \param[in] factory                  A factory class that creates a dataset subset of a given
    ///                                     kind on request (see also \c subset_id()).
    /// \param[in] dice_transaction         The DiCE database transaction that the operation runs in.
    ///                                     The transaction may be invalid indicating an asynchronous 
    ///                                     data import operation.
    ///
    /// \return                             Returns the portion of the dataset contained in the
    ///                                     3D area.
    ///
    virtual IDistributed_data_subset* create(
        const mi::math::Bbox_struct<mi::Sint32, 3>&     bounding_box,
        IData_subset_factory*                           factory,
        mi::neuraylib::IDice_transaction*               dice_transaction) const = 0;

    /// Implementation maps floating-point bounding box to a signed integer bounding box and call respective variant.
    ///
    /// \param[in] bounding_box             The 3D area for which data shall be imported. The bounding
    ///                                     box is given in the dataset's local space.
    /// \param[in] factory                  A factory class that creates a dataset subset of a given
    ///                                     kind on request (see also \c subset_id()).
    /// \param[in] dice_transaction         The DiCE database transaction that the operation runs in.
    ///                                     The transaction may be invalid indicating an asynchronous 
    ///                                     data import operation.
    ///
    /// \return                             Returns the portion of the dataset contained in the
    ///                                     3D area.
    ///
    virtual IDistributed_data_subset* create(
        const mi::math::Bbox_struct<mi::Float32, 3>&    bounding_box,
        IData_subset_factory*                           factory,
        mi::neuraylib::IDice_transaction*               dice_transaction) const /*override final*/
    {
        mi::math::Bbox_struct<mi::Sint32, 3> bbox;
        bbox.min.x = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.x));
        bbox.min.y = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.y));
        bbox.min.z = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.z));
        bbox.max.x = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.x));
        bbox.max.y = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.y));
        bbox.max.z = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.z));
        return create(bbox, factory, dice_transaction);
    }

    /// Shall create and provide the dataset subset contained in the given bounding box for a given time step.
    ///
    /// \param[in] bounding_box             The 3D area for which data shall be imported. The bounding
    ///                                     box is given in the dataset's local space.
    /// \param[in] time_step                The time step for which data shall be imported.
    /// \param[in] factory                  A factory class that creates a dataset subset of a given
    ///                                     kind on request (see also \c subset_id()).
    /// \param[in] dice_transaction         The DiCE database transaction that the operation runs in.
    ///                                     The transaction may be invalid indicating an asynchronous 
    ///                                     data import operation.
    ///
    /// \return                             Returns the portion of the dataset contained in the
    ///                                     3D area.
    ///
    virtual IDistributed_data_subset* create(
        const mi::math::Bbox_struct<mi::Sint32, 3>&     bounding_box,
        mi::Uint32                                      time_step,
        IData_subset_factory*                           factory,
        mi::neuraylib::IDice_transaction*               dice_transaction) const
    {
        (void) bounding_box;     // avoid unused warnings
        (void) time_step;        // avoid unused warnings
        (void) factory;          // avoid unused warnings
        (void) dice_transaction; // avoid unused warnings

        return 0;
    }

    /// Implementation maps floating-point bounding box to a signed integer bounding box and call respective variant.
    ///
    /// \param[in] bounding_box             The 3D area for which data shall be imported. The bounding
    ///                                     box is given in the dataset's local space.
    /// \param[in] time_step                The time step for which data shall be imported.
    /// \param[in] factory                  A factory class that creates a dataset subset of a given
    ///                                     kind on request (see also \c subset_id()).
    /// \param[in] dice_transaction         The DiCE database transaction that the operation runs in.
    ///                                     The transaction may be invalid indicating an asynchronous 
    ///                                     data import operation.
    ///
    /// \return                             Returns the portion of the dataset contained in the
    ///                                     3D area.
    ///
    virtual IDistributed_data_subset* create(
        const mi::math::Bbox_struct<mi::Float32, 3>&    bounding_box,
        mi::Uint32                                      time_step,
        IData_subset_factory*                           factory,
        mi::neuraylib::IDice_transaction*               dice_transaction) const /*override final*/
    {
        mi::math::Bbox_struct<mi::Sint32, 3> bbox;
        bbox.min.x = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.x));
        bbox.min.y = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.y));
        bbox.min.z = static_cast<mi::Sint32>(mi::math::floor(bounding_box.min.z));
        bbox.max.x = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.x));
        bbox.max.y = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.y));
        bbox.max.z = static_cast<mi::Sint32>(mi::math::ceil(bounding_box.max.z));
        return create(bbox, time_step, factory, dice_transaction);
    }

    /// Empty body, i.e., no member data is serialized.
    /// \param[in] serializer unused.
    virtual void serialize( mi::neuraylib::ISerializer* serializer) const
    {
        // avoid warnings
        (void) serializer;
    }

    /// Empty body, i.e., no member data is deserialized.
    /// \param[in] deserializer unused
    virtual void deserialize( mi::neuraylib::IDeserializer* deserializer)
    {
        // avoid warnings
        (void) deserializer;
    }

    /// Empty body, i.e., no configuration for the session exporter given.
    virtual const char* get_configuration() const { return 0; }
};


/// This mixin class can be used to implement the IDistributed_data_import_callback interface.
///
/// This mixin class provides a default implementation of some of the pure virtual methods of the
/// IDistributed_data_import_callback. The documentation here just lists the behavior of the
/// default implementation, see IDistributed_data_import_callback for the documentation of the
/// methods themselves.
/// @ingroup nv_index_data_storage
template <mi::Uint32 id1, mi::Uint16 id2, mi::Uint16 id3,
          mi::Uint8 id4, mi::Uint8 id5, mi::Uint8 id6, mi::Uint8 id7,
          mi::Uint8 id8, mi::Uint8 id9, mi::Uint8 id10, mi::Uint8 id11,
          class I = IDistributed_data_import_callback>
class Distributed_continuous_data_import_callback :
    public mi::neuraylib::Base<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11,I>
{
public:
    using mi::neuraylib::Base<id1, id2, id3, id4, id5, id6, id7, id8, id9, id10, id11, I>::create;

    /// Empty body, i.e., no data import required based on floating-point bounding boxes.
    /// \param[in] bbox                     unused.
    /// \param[in] time_step                unused.
    /// \param[in] factory                  unused.
    /// \param[in] dice_transaction         unused.
    /// \return                             0.
    virtual IDistributed_data_subset* create(
        const mi::math::Bbox_struct<mi::Float32, 3>&    bbox,
        mi::Uint32                                      time_step,
        IData_subset_factory*                           factory,
        mi::neuraylib::IDice_transaction*               dice_transaction) const
    {
        (void)bbox;             // avoid unused warnings
        (void)time_step;        // avoid unused warnings
        (void)factory;          // avoid unused warnings
        (void)dice_transaction; // avoid unused warnings

        return 0;
    }

    /// Empty body, i.e., no member data is serialized.
    /// \param[in] serializer unused
    virtual void serialize(mi::neuraylib::ISerializer* serializer) const
    {
        // avoid warnings
        (void)serializer;
    }

    /// Empty body, i.e., no member data is deserialized.
    /// \param[in] deserializer unused
    virtual void deserialize(mi::neuraylib::IDeserializer* deserializer)
    {
        // avoid warnings
        (void)deserializer;
    }

    /// Empty body, i.e., no configuration for the session exporter given.
    virtual const char* get_configuration() const { return 0; }
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_DATA_IMPORT_CALLBACK_H
