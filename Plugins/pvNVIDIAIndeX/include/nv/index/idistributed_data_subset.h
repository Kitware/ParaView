/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Distributed subset of a large-scale dataset.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_DATA_SUBSET_H
#define NVIDIA_INDEX_IDISTRIBUTED_DATA_SUBSET_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/neuraylib/iserializer.h>

namespace nv {
namespace index {


// * optional for attribute-bound datasets?
//   - volumes
//   - height-fields do not store attributes for example...
class IDistributed_data_attribute_set_descriptor :
    public mi::base::Interface_declare<0x88b4168a,0xde56,0x4f18,0x96,0xca,0xd6,0x4d,0xef,0x0,0x4b,0x94>
{
public:
    virtual bool is_valid() const = 0;
};

class IDistributed_data_subset_data_descriptor :
    public mi::base::Interface_declare<0x10f2ae8b,0xd1ac,0x446b,0xa2,0xef,0x15,0xb,0xd,0x82,0xaa,0xc8>
{
public:
    /// Returns whether the data-subset data descriptor is valid or not.
    ///
    virtual bool is_valid() const = 0;

    /// Returns the subdivision subregion bounding box of the subset in global scene space.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3>    get_subregion_scene_space() const = 0;
    /// Returns the subdivision subregion bounding box of the subset in the dataset's object space.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3>    get_subregion_object_space() const = 0;
};

/// @ingroup nv_index_data_storage
///
/// The base class for each subset of a large-scale distributed dataset.
///
/// The entire scene is decomposed into smaller
/// sized subregions. Each of these subregions
/// contain only a fraction of the entire dataset uploaded to NVIDIA IndeX.
/// For instance, a subset of a regular volume is called a brick and a subset
/// of a regular heightfield is called a patch.
/// The data import callback (\c IDistributed_data_import_callback) is
/// responsible for creating and loading/generating the contents of the
/// data subset.
///
class IDistributed_data_subset :
    public mi::base::Interface_declare<0x3fbeb822,0xffd0,0x4520,0x83,0x21,0xbd,0x62,0x89,0x69,0x01,0x69,
                                       mi::neuraylib::ISerializable>
{
public:
    /// Check if the data is valid for the use by NVIDIA IndeX.
    ///
    /// The application can verify whether the dataset is ready for
    /// use using this method anytime. In particular, the call is independent
    /// from calling \c finalize(). That is, the application can continue to
    /// modify the dataset before calling finally calling
    /// \c finalize(), even if is_valid() returns \c true.
    ///
    /// \return true  Returns \c true if the data subset is ready for use
    ///               by NVIDIA IndeX.
    ///
    virtual bool is_valid() const = 0;

    /// Finalize this dataset for use by NVIDIA IndeX.
    ///
    /// This can be called only once to finalize the data subset and submitting
    /// it to the NVIDIA IndeX library. Once the method is
    /// called, the application may not invoke any of the non-const
    /// methods of this class anymore.
    ///
    /// \return     Returns \c true if the finalization was successful.
    ///
    virtual bool finalize() = 0;
};

/// @ingroup nv_index_data_storage
///
/// The interface classes derived from \c IDistributed_data_subset cannot be
/// instantiated beyond the library
/// boundary, e.g., by an application. But the data import callback
/// (\c IDistributed_data_import_callback) requires
/// an application to crate instances of subset of large-scale distributed
/// datasets. The factory interface class \c IData_subset_factory creates these
/// classes for the user based on the given UUID.
/// An implementation of this class is passed as argument to the import callback
/// method \c IDistributed_data_import_callback::create().
///
class IData_subset_factory :
    public mi::base::Interface_declare<0x79c2f676,0x194d,0x4915,0xb0,0xce,0x66,0xbe,0x68,0x7c,0xbd,0xc7>
{
public:
    /// Creates an instance that represents a subset of a large-scale dataset.
    /// The instance is an implementation of the interface classes derived from
    /// \c IDistributed_data_subset. Common interface classes include a brick of
    /// a regular volume (\c IRegular_volume_brick) or a patch of a regular
    /// heightfield (\c IRegular_heightfield_patch). The factory creates these
    /// instances based on just the UUID of these interface classes.
    ///
    /// \param[in] dataset_type The UUID of the interface class that represents
    ///                         a subset of a large-scale dataset.
    /// \param[in] parameter    Custom parameter string.
    ///
    /// \return Created instance of an implementation of the interface class
    ///         that represents the subset of the large-scale dataset.
    ///
    virtual IDistributed_data_subset*   create_data_subset(
                                            const mi::base::Uuid&   dataset_type,
                                            const char*             parameter) const = 0;

    virtual IDistributed_data_subset*   create_data_subset(
                                            const mi::base::Uuid&                               dataset_type,
                                            const IDistributed_data_attribute_set_descriptor*   dataset_attrib_set_desc,
                                            const char*                                         parameter) const = 0;

    // Convenience functions for creating typed data subsets.
    template <class T>
    const T* create_data_subset(const char* parameter = 0) const
    {
        const T* t = static_cast<const T*>(create_data_subset(typename T::IID(), parameter));
        return t;
    }

    template <class T>
    T* create_data_subset(const char* parameter = 0)
    {
        T* t = static_cast<T*>(create_data_subset(typename T::IID(), parameter));
        return t;
    }

    template <class T>
    const T* create_data_subset(
        const IDistributed_data_attribute_set_descriptor*   dataset_attrib_set_desc,
        const char*                                         parameter = 0) const
    {
        const T* t = static_cast<const T*>(create_data_subset(typename T::IID(), dataset_attrib_set_desc, parameter));
        return t;
    }

    template <class T>
    T* create_data_subset(
        const IDistributed_data_attribute_set_descriptor*   dataset_attrib_set_desc,
        const char*                                         parameter = 0)
    {
        T* t = static_cast<T*>(create_data_subset(typename T::IID(), dataset_attrib_set_desc, parameter));
        return t;
    }

    // Creates a new attribute-set descriptor
    virtual IDistributed_data_attribute_set_descriptor* create_attribute_set_descriptor(const mi::base::Uuid& class_id) const = 0;

    // Convenience functions for creating typed attribute-set descriptors.
    template <class T>
    T* create_attribute_set_descriptor()
    {
        return static_cast<T*>(create_attribute_set_descriptor(typename T::IID()));
    }
};


} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_DATA_SUBSET_H
