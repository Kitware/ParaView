/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Asynchronous, distributed data generation techniques.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_TECHNIQUE_H
#define NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_TECHNIQUE_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/iattribute.h>
#include <nv/index/idistributed_compute_destination_buffer.h>

namespace nv {
namespace index {

/// Interface class for user-defined techniques for computing data (2D textures
/// on plane and heightfield shapes and 3D volume data) on demand.
///
/// An implementation of this class can be assigned as a scene description
/// attribute to an compute-enabled scene element, such as \c IPlane, 
/// \c IRegular_heightfield or \c ISparse_volume_scene_element. When rendering a certain area
/// of the scene primitive (defined by the renderer), the compute technique is
/// called and an instance of an \c IDistributed_compute_destination_buffer is passed. The
/// compute technique fills this buffer for the required subregions of the
/// distributed dataset. The renderer then maps either uses the generated data
/// directly (\c ISparse_volume_scene_element) or uses it as a texture mapped onto the defined
/// 2D area (c IPlane and \c IRegular_heightfield).
///
/// Typical use cases for the compute technique are to visualize data that result
/// from complex, possibly distributed, computing processes process either as 2D
/// textures or 3D volumes.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_technique :
    public mi::base::Interface_declare<0xfbde8f4c,0x421e,0x4a6e,0x98,0x68,0x6a,0xe2,0x9b,0xac,0x30,0x4d,
                                       nv::index::IAttribute>
{
public:
    /// Launches the user-defined compute technique. This method is called by the
    /// rendering system in a separate thread to trigger the computation of the buffer
    /// data asynchronously and in parallel to the rendering of subregion data.
    ///
    /// \note This method must only return upon finishing the compute tasks.
    ///
    /// \param[in]     dice_transaction  The current DiCE transaction.
    ///
    /// \param[in,out] dst_buffer
    ///                An instance of a \c IDistributed_compute_destination_buffer representing
    ///                either a \c IDistributed_compute_destination_buffer_2d_texture for generating
    ///                2D texture data or a specialization of \c IDistributed_compute_destination_buffer_3d_volume
    ///                for generating 3D volume data. Part of destination buffers
    ///                attributes are set by the rendering system. These include,
    ///                for instance, the bounding box of the subregion or the 2D
    ///                screen space area covered by the subregion. The destination
    ///                buffers are created by and will be cached inside the rendering
    ///                system.
    ///
    virtual void launch_compute(
        mi::neuraylib::IDice_transaction*        dice_transaction,
        IDistributed_compute_destination_buffer* dst_buffer) const = 0;

    /// Returns optional configuration settings that may be used by the library
    /// for the session export mechanism provided by \c ISession::export_session().
    ///
    /// \see IDistributed_data_import_strategy::get_configuration()
    ///
    /// \return             String representation of the configuration settings, the
    ///                     instance of the interface class keeps ownership of the returned pointer.
    ///
    virtual const char* get_configuration() const
    {
        return 0;
    }
};

/// Interface class for user-defined techniques for computing 2D LOD-texture data on demand.
///
/// An implementation of this class can be assigned as a scene description attribute to an compute-enabled \c IPlane
/// scene element.
/// 
/// When rendering a certain area of the scene primitive (defined by the renderer), the compute technique is called and
/// an instance of an \c IDistributed_compute_destination_buffer_2d_texture_LOD is passed the \c launch_compute() method.
/// The compute technique fills this buffer for the required subregions and mipmap-layers of the distributed dataset.
/// The renderer then uses the generated data as a texture mapped onto the defined 2D area (\c IPlane).
///
/// Typical use cases for the compute technique are to visualize data that result from complex, possibly distributed,
/// computing processes process as 2D textures.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_technique_LOD :
    public mi::base::Interface_declare<0x7c8909d5,0x3cc2,0x415b,0x80,0xef,0x65,0x74,0x99,0x38,0xd6,0xff,
                                       nv::index::IDistributed_compute_technique>
{
};

/// Mixin class for implementing the IDistributed_compute_technique interface.
///
/// This mixin class provides a default implementation of some of the pure
/// virtual methods of the IDistributed_compute_technique interface.
///
/// \ingroup nv_index_data_computing
///
template <mi::Uint32 i_id1, mi::Uint16 i_id2, mi::Uint16 i_id3,
          mi::Uint8  i_id4, mi::Uint8  i_id5, mi::Uint8  i_id6,  mi::Uint8 i_id7,
          mi::Uint8  i_id8, mi::Uint8  i_id9, mi::Uint8  i_id10, mi::Uint8 i_id11,
          class I = nv::index::IDistributed_compute_technique>
class Distributed_compute_technique :
    public mi::neuraylib::Element<i_id1,i_id2,i_id3,i_id4,i_id5,i_id6,i_id7,i_id8,i_id9,i_id10,i_id11,I>
{
public:
    /// Return IID as attribute class.
    /// \return attribute class IID
    virtual mi::base::Uuid get_attribute_class() const
    {
        return nv::index::IDistributed_compute_technique::IID();
    }

    /// Only a single instance of this attribute can be active at the same time.
    /// \return true if multiple active instances
    virtual bool multiple_active_instances() const
    {
        return false;
    }

    /// Attribute is always enabled
    ///
    /// \param[in] enable unused argument
    ///
    virtual void set_enabled(bool enable)
    {
        // avoid warnings
        (void) enable;
    }

    /// Attribute is always enabled
    ///
    /// \return true always
    ///
    virtual bool get_enabled() const
    {
        return true;
    }

    /// Each scene element can store additional user-defined meta data. Meta
    /// data, for instance, may include a string representing the scene
    /// element's name or domain specific attributes.  A class that represents
    /// meta data has to be a database element and the scene element then refers
    /// to the database element by means of a tag.
    ///
    /// \param[in] tag  The tag that refers to the user-defined meta
    ///                 data associated with the scene element.
    ///
    virtual void set_meta_data(mi::neuraylib::Tag_struct tag)
    {
      (void) tag;
    }

    /// Retrieve the scene element's reference to the user-defined meta data.
    ///
    /// \return  Returns the tag that refers to the user-defined meta
    ///          data associated with the scene element.
    ///
    virtual mi::neuraylib::Tag_struct get_meta_data() const { return mi::neuraylib::NULL_TAG; }
};


/// Mixin class for implementing the IDistributed_compute_technique_LOD interface.
///
/// \ingroup nv_index_data_computing
///
template <mi::Uint32 i_id1, mi::Uint16 i_id2, mi::Uint16 i_id3,
          mi::Uint8  i_id4, mi::Uint8  i_id5, mi::Uint8  i_id6,  mi::Uint8 i_id7,
          mi::Uint8  i_id8, mi::Uint8  i_id9, mi::Uint8  i_id10, mi::Uint8 i_id11,
          class I = nv::index::IDistributed_compute_technique_LOD>
class Distributed_compute_technique_LOD :
    public nv::index::Distributed_compute_technique<i_id1,i_id2,i_id3,i_id4,i_id5,i_id6,i_id7,i_id8,i_id9,i_id10,i_id11,I>
{
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_TECHNIQUE_H
