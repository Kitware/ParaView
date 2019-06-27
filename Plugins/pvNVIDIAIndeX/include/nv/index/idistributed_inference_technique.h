/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#ifndef NVIDIA_INDEX_IDISTRIBUTED_INFERENCE_TECHNIQUE_H
#define NVIDIA_INDEX_IDISTRIBUTED_INFERENCE_TECHNIQUE_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/iattribute.h>
#include <nv/index/idistributed_data_access.h>
#include <nv/index/idistributed_data_subset.h>
#include <nv/index/iinference_result.h>
#include <nv/index/iinference_source_data.h>
#include <nv/index/imemory_manager.h>

namespace nv {
namespace index {

/// @ingroup nv_index_data_computing
///
/// Interface class for user-defined AI/DL inference techniques.
///
/// An implementation of this class can be assigned as a scene description. 
///
class IDistributed_inference_technique :
    public mi::base::Interface_declare<0xfc8e88e7,0x7d00,0x41ae,0x89,0xa1,0x10,0x7f,0x8c,0x83,0xff,0x7a,
                                       nv::index::IAttribute>
{
public:
    /// Launches the user-defined inference technique. This method is called by the
    /// rendering system in a separate thread to trigger the inference operation
    /// asynchronously and in parallel to the processing of subregion.
    ///
    /// \note This method must only return upon finishing the compute tasks.
    ///
    /// \param[in]      dice_transaction
    ///                 The current DiCE transaction.
    ///
    /// \param[in]      source_data
    ///                 An instance of a \c IInference_source allows accessing
    ///                 the data stored locally on the machine/GPU.
    ///                 \note
    ///                 Future version shall allow accessing data beyond the local data extent.
    ///
    /// \param[in]      memory_manager
    ///                 An instance of a \c IMemory_manager allows allocating library-side memory
    ///                 that shall be filled with application-side data.
    ///
    /// \param[in,out]  result_data_assignment
    ///                 Receives the inference result data from the external inference operations and maps them
    ///                 to a unique identifier. The identifier or slot shall be know by subsequent XAC shader or
    ///                 compute programs to make use of the results inside. Making the slot available to the 
    ///                 XAC shader represents the responsibility of the user or application writer.
    ///
    virtual void inference(
        mi::neuraylib::IDice_transaction*       dice_transaction,
        const IInference_source_data*           source_data,
        IMemory_manager*                        memory_manager,
        IInference_result*                      result_data_assignment) const = 0;

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

/// @ingroup nv_index_data_computing
///
/// Mixin class for implementing the IDistributed_inference_technique interface.
///
/// This mixin class provides a default implementation of some of the pure
/// virtual methods of the IDistributed_inference_technique interface.
///
template <mi::Uint32 i_id1, mi::Uint16 i_id2, mi::Uint16 i_id3,
          mi::Uint8  i_id4, mi::Uint8  i_id5, mi::Uint8  i_id6,  mi::Uint8 i_id7,
          mi::Uint8  i_id8, mi::Uint8  i_id9, mi::Uint8  i_id10, mi::Uint8 i_id11,
          class I = nv::index::IDistributed_inference_technique>
class Distributed_inference_technique :
    public mi::neuraylib::Element<i_id1,i_id2,i_id3,i_id4,i_id5,i_id6,i_id7,i_id8,i_id9,i_id10,i_id11,I>
{
public:
    /// Return IID as attribute class.
    /// \return attribute class IID
    virtual mi::base::Uuid get_attribute_class() const
    {
        return nv::index::IDistributed_inference_technique::IID();
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
        // avoid warnings
        (void) tag;
    }

    /// Retrieve the scene element's reference to the user-defined meta data.
    ///
    /// \return  Returns the tag that refers to the user-defined meta
    ///          data associated with the scene element.
    ///
    virtual mi::neuraylib::Tag_struct get_meta_data() const { return mi::neuraylib::NULL_TAG; }
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_INFERENCE_TECHNIQUE_H
