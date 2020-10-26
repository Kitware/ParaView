/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file   idistributed_data_subset.h
/// \brief  Distributed subset interaces for a large-scale distributed datasets.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_DATA_SUBSET_H
#define NVIDIA_INDEX_IDISTRIBUTED_DATA_SUBSET_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/neuraylib/iserializer.h>

namespace nv {
namespace index {

/// Attribute formats for distributed dataset types.
///
/// \note   A distributed dataset type may support support only a subset of the types below. 
///         Please review the specific datatypes for information regarding the 
///         supported attribute types (e.g., see \c ISparse_volume_subset).
///
/// \ingroup nv_index_data_subsets
///
enum Distributed_data_attribute_format
{
    ATTRIB_FORMAT_UINT8          = 0x00u,   ///< Scalar attribute format with uint8 precision
    ATTRIB_FORMAT_UINT8_2,                  ///< Vector attribute format with 2 components and uint8 precision per component
    ATTRIB_FORMAT_UINT8_3,                  ///< Vector attribute format with 3 components and uint8 precision per component
    ATTRIB_FORMAT_UINT8_4,                  ///< Vector attribute format with 4 components and uint8 precision per component
    ATTRIB_FORMAT_SINT8,                    ///< Scalar attribute format with sint8 precision
    ATTRIB_FORMAT_SINT8_2,                  ///< Vector attribute format with 2 components and sint8 precision per component
    ATTRIB_FORMAT_SINT8_3,                  ///< Vector attribute format with 3 components and sint8 precision per component
    ATTRIB_FORMAT_SINT8_4,                  ///< Vector attribute format with 4 components and sint8 precision per component
    ATTRIB_FORMAT_UINT16,                   ///< Scalar attribute format with uint16 precision
    ATTRIB_FORMAT_UINT16_2,                 ///< Vector attribute format with 2 components and uint16 precision per component
    ATTRIB_FORMAT_UINT16_3,                 ///< Vector attribute format with 3 components and uint16 precision per component
    ATTRIB_FORMAT_UINT16_4,                 ///< Vector attribute format with 4 components and uint16 precision per component
    ATTRIB_FORMAT_SINT16,                   ///< Scalar attribute format with sint16 precision
    ATTRIB_FORMAT_SINT16_2,                 ///< Vector attribute format with 2 components and sint16 precision per component
    ATTRIB_FORMAT_SINT16_3,                 ///< Vector attribute format with 3 components and sint16 precision per component
    ATTRIB_FORMAT_SINT16_4,                 ///< Vector attribute format with 4 components and sint16 precision per component
    ATTRIB_FORMAT_UINT32,                   ///< Scalar attribute format with uint32 precision
    ATTRIB_FORMAT_UINT32_2,                 ///< Vector attribute format with 2 components and uint32 precision per component
    ATTRIB_FORMAT_UINT32_3,                 ///< Vector attribute format with 3 components and uint32 precision per component
    ATTRIB_FORMAT_UINT32_4,                 ///< Vector attribute format with 4 components and uint32 precision per component
    ATTRIB_FORMAT_SINT32,                   ///< Scalar attribute format with sint32 precision
    ATTRIB_FORMAT_SINT32_2,                 ///< Vector attribute format with 2 components and sint32 precision per component
    ATTRIB_FORMAT_SINT32_3,                 ///< Vector attribute format with 3 components and sint32 precision per component
    ATTRIB_FORMAT_SINT32_4,                 ///< Vector attribute format with 4 components and sint32 precision per component
    ATTRIB_FORMAT_FLOAT16,                  ///< Scalar attribute format with float16 precision
    ATTRIB_FORMAT_FLOAT16_2,                ///< Vector attribute format with 2 components and float16 precision per component
    ATTRIB_FORMAT_FLOAT16_3,                ///< Vector attribute format with 3 components and float16 precision per component
    ATTRIB_FORMAT_FLOAT16_4,                ///< Vector attribute format with 4 components and float16 precision per component
    ATTRIB_FORMAT_FLOAT32,                  ///< Scalar attribute format with float32 precision
    ATTRIB_FORMAT_FLOAT32_2,                ///< Vector attribute format with 2 components and float32 precision per component
    ATTRIB_FORMAT_FLOAT32_3,                ///< Vector attribute format with 3 components and float32 precision per component
    ATTRIB_FORMAT_FLOAT32_4,                ///< Vector attribute format with 4 components and float32 precision per component

    ATTRIB_FORMAT_COUNT,
    ATTRIB_FORMAT_INVALID = ATTRIB_FORMAT_COUNT
};

/// A data attribute set descriptor configures and communicates the attributes for a data subset.
/// 
/// This interface class can be used by an application to define the attribute set of a subset data
/// or by the NVIDIA IndeX library to communicate the information the attributes set
/// aggregated by a data subset. Typically, a dataset can have multiple attributes.
///
/// \note   Typically, an instance of \c IDistributed_data_attribute_set_descriptor implementation is exposed by 
///         data subset interfaces. Please note that an attribute set descriptor mandatory.
///
class IDistributed_data_attribute_set_descriptor :
    public mi::base::Interface_declare<0x88b4168a,0xde56,0x4f18,0x96,0xca,0xd6,0x4d,0xef,0x0,0x4b,0x94>
{
public:
    /// Verifying the correctness of a data subset descriptor.
    /// 
    /// \return     Returns \c true if the attribute set descriptor is valid
    ///             Otherwise it returns \c false.
    ///
    virtual bool is_valid() const = 0;
};

/// Subset data descriptors communicate the structural data representations about a subset data to an application.
/// 
/// This interface class is used to communicate information such as the layout of the internal data representions of a
/// data subset to the application. For instance, a \c ISparse_volume_subset_data_descriptor shares details 
/// about the number of bricks and the depth level-of-detail structure that compose a single \c ISparse_volume_subset
/// representation in a subregion.
///
/// Typically, an instance of \c IDistributed_data_subset_data_descriptor implementation is exposed by 
/// data subset interfaces.
///
class IDistributed_data_subset_data_descriptor :
    public mi::base::Interface_declare<0x10f2ae8b,0xd1ac,0x446b,0xa2,0xef,0x15,0xb,0xd,0x82,0xaa,0xc8>
{
public:
    /// Verifying the integrity of a data subset descriptor.
    ///
    /// \return     Returns \c true if the data subset descriptor is valid, i.e., 
    ///             the subset data representation is valid. Otherwise it
    ///             returns \c false.
    ///
    virtual bool is_valid() const = 0;

    /// The scene space bounding box of the subdivision's subregion, which hosts the given data subset. 
    ///
    /// \return     Returns the bounding box of the subregion in the 
    ///             global scene space.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_subregion_scene_space() const = 0;

    /// The object space bounding box of the subdivision's subregion, which hosts the given data subset. 
    ///
    /// \return     Returns the bounding box of the subregion in the subset's 
    ///             object space, i.e., in the distributed data's object space .
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_subregion_object_space() const = 0;
};

/// A data subset represent the unique entity representing a distributed dataset inside a single subregion.
///
/// The NVIDIA IndeX system decomposes space (including the scene) into disjoint subregions.
/// These subregions contain smaller-sized portions of an entire large-scale dataset;  
/// these portions are called data subsets in NVIDIA IndeX and represented by the 
/// \c IDistributed_data_subset interface. 
///
/// A data subset represents the major building-blocks for distributed rendering of 
/// large-scale data in NVIDIA IndeX but also for applying all kinds 
/// of processing and analysis operations. The derived distributed dataset type 
/// specific interfaces such as the \c ISparse_volume_subset expose interface methods
/// for querying the internal data representation and for manipulating the internal 
/// data representation.
/// Typically, the data representation is hosted in both the main memory but, more
/// importantly, also in the CUDA device memory for fast rendering and processing.
/// Whenever possible, the specific data subset representations are always  
/// supposed to provide interfaces for direct CUDA device memory access to the
/// subset data (see \c IIrregular_volume_subset).
/// Such direct access to the CUDA device representations enables application-side
/// CUDA-based processing of subset data or instant data updates, e.g., using 
/// NVIDIA's <a href="https://developer.nvidia.com/blog/gpudirect-storage/">GPUDirect Storage</a> or
/// <a href="https://developer.nvidia.com/gpudirect">GPUDirect RDMA</a>.
///
/// NVIDIA IndeX's distributed data import infrastructure relies on the 
/// subregion and dat subset concept. 
/// An importer callback (see \c IDistributed_data_import_callback) receives 
/// an instance of a \c IData_subset_factory interface implementation to 
/// enables an application to create a empty data subset and is then responsible
/// for generating the desired valid contents of a data subset.
///
/// \ingroup nv_index_data_subsets
///
class IDistributed_data_subset :
    public mi::base::Interface_declare<0x3fbeb822,0xffd0,0x4520,0x83,0x21,0xbd,0x62,0x89,0x69,0x01,0x69,
                                       mi::neuraylib::ISerializable>
{
public:
    /// Verifying the integrity of a data subset descriptor for the use by NVIDIA IndeX.
    ///
    /// If the data subset descriptor is in a valid state, then the
    /// provided data is sufficient for NVIDIA IndeX to create an  
    /// internal data representation for a subset. 
    ///
    /// The application, e.g., an importer, can verify whether the dataset is 
    /// valid using this method anytime. In particular, the call is independent
    /// from calling \c finalize(). That is, the application can continue to
    /// modify the dataset before calling \c finalize().
    ///
    /// \return     Returns \c true if the data subset is ready for use
    ///             by NVIDIA IndeX. Even if the subset is already in a valid state
    ///             an application can continue to extend the subsets data contents.
    ///             If the provided data is insufficient for NVIDIA IndeX 
    ///             to produce a valid data representation for the subset, then 
    ///             \c false is returned.
    ///
    virtual bool is_valid() const = 0;

    /// Finalize the given subset's data representation.
    ///
    /// When called this method submits the data subset to the NVIDIA
    /// IndeX system and finalizes the data representation of the subset,
    /// which may include the generation of internal data structures 
    /// for fast data access and rendering. Usually NVIDIA IndeX also
    /// tries to repair the integrity of the subset data, e.g., an
    /// unstructured mesh might require additional treatment and cleanups.
    /// The #finalize method may only be called once. Once called
    /// successfully, the data subset is locked for use by NVIDIA IndeX.
    /// In particular, the application may not invoke any of the
    /// non-const methods of the data subset interface class anymore.
    ///
    /// \return     Returns \c true if the finalization was successful, i.e.,
    ///             the integrity of the data subset has been verified and
    ///             all internal data structures built successfully.
    ///             If \c false is returned, then the finalize was obviously
    ///             not successful and the NVIDIA IndeX cannot operate with
    ///             the set up data subset representation. 
    ///
    virtual bool finalize() = 0;
};

/// Factory for creating an empty data subset for a specific distributed dataset type.
/// 
/// Instances of the distributed data subset interface (\c IDistributed_data_subset)
/// can not be created by an application, the instantiation requires a factory that
/// is provided by NVIDIA IndeX. The present interface \c IData_subset_factory allows
/// an application to instantiate a distributed data subset by means of additional
/// parameter. 
/// a  
/// But the data import callback
/// (\c IDistributed_data_import_callback) requires
/// an application to create instances of subset of large-scale distributed
/// datasets. The factory interface class \c IData_subset_factory creates these
/// classes for the user based on the given UUID.
/// An implementation of this class is passed as argument to the import callback
/// method \c IDistributed_data_import_callback::create().
///
/// \note   The interface classes derived from \c IDistributed_data_subset cannot be
///         instantiated beyond the library boundary, e.g., by an application. 
///
/// \ingroup nv_index_data_subsets
///
class IData_subset_factory :
    public mi::base::Interface_declare<0x79c2f676,0x194d,0x4915,0xb0,0xce,0x66,0xbe,0x68,0x7c,0xbd,0xc7>
{
public:
    /// Creates an instance that represents a subset of a large-scale dataset.
    /// The instance is an implementation of the interface classes derived from
    /// \c IDistributed_data_subset. Common interface classes include a subset of
    /// a sparse volume (\c ISparse_volume_subset) or a patch of a regular
    /// heightfield (\c IRegular_heightfield_patch). The factory creates these
    /// instances based on just the UUID of these interface classes.
    ///
    /// \param[in] dataset_type     The UUID of the interface class that represents
    ///                             a subset of a large-scale dataset.
    ///
    /// \param[in] parameter        Custom parameter string.
    ///
    /// \return Created instance of an implementation of the interface class
    ///         that represents the subset of the large-scale dataset.
    ///
    virtual IDistributed_data_subset*   create_data_subset(
                                            const mi::base::Uuid&   dataset_type,
                                            const char*             parameter) const = 0;

    /// Creates an instance that represents a subset of a large-scale dataset.
    /// The instance is an implementation of the interface classes derived from
    /// \c IDistributed_data_subset. Common interface classes include a subset of
    /// a sparse volume (\c ISparse_volume_subset) or a patch of a regular
    /// heightfield (\c IRegular_heightfield_patch). The factory creates these
    /// instances based on just the UUID of these interface classes.
    ///
    /// \param[in] dataset_type             The UUID of the interface class that represents
    ///                                     a subset of a large-scale dataset.
    ///
    /// \param[in] dataset_attrib_set_desc  Custom parameter string.
    ///
    /// \param[in] parameter                Custom parameter string.
    ///
    /// \return Created instance of an implementation of the interface class
    ///         that represents the subset of the large-scale dataset.
    ///
    virtual IDistributed_data_subset*   create_data_subset(
                                            const mi::base::Uuid&                               dataset_type,
                                            const IDistributed_data_attribute_set_descriptor*   dataset_attrib_set_desc,
                                            const char*                                         parameter) const = 0;

    /// Convenience template function for creating and empty typed data subset instance.
    /// 
    /// \code
    /// //
    /// // Usage:
    /// //
    /// mi::base::Handle<const nv::index::ISparse_volume_subset> attribute_set(
    ///     factory->create_data_subset<nv::index::ISparse_volume_subset>());
    /// \endcode
    ///
    /// \return                 Returns a typed data subset.
    ///
    template <class T>
    const T* create_data_subset(const char* parameter = 0) const
    {
        const T* t = static_cast<const T*>(create_data_subset(typename T::IID(), parameter));
        return t;
    }

    /// Convenience template function for creating and empty typed data subset instance.
    /// 
    /// \code
    /// //
    /// // Usage:
    /// //
    /// mi::base::Handle<nv::index::ISparse_volume_subset> attribute_set(
    ///     factory->create_data_subset<nv::index::ISparse_volume_subset>());
    /// \endcode
    ///
    /// \return                 Returns a typed data subset.
    ///
    template <class T>
    T* create_data_subset(const char* parameter = 0)
    {
        T* t = static_cast<T*>(create_data_subset(typename T::IID(), parameter));
        return t;
    }

    /// Convenience template function for creating and empty typed data subset instance for a given attribute set descriptor.
    /// 
    /// \code
    /// //
    /// // Usage:
    /// //
    /// mi::base::Handle<const nv::index::ISparse_volume_subset> attribute_set(
    ///     factory->create_data_subset<nv::index::ISparse_volume_subset>(descriptor, parameter));
    /// \endcode
    ///
    /// \param[in] dataset_attrib_set_desc  A attribute set descriptor.
    ///
    /// \param[in] parameter                Optional parameter string typically used to customize the data subset creations.  
    ///
    /// \return Returns a typed data subset.
    ///
    template <class T>
    const T* create_data_subset(
        const IDistributed_data_attribute_set_descriptor*   dataset_attrib_set_desc,
        const char*                                         parameter = 0) const
    {
        const T* t = static_cast<const T*>(create_data_subset(typename T::IID(), dataset_attrib_set_desc, parameter));
        return t;
    }

    /// Convenience template function for creating and empty typed data subset instance for a given attribute set descriptor.
    /// 
    /// \code
    /// //
    /// // Usage:
    /// //
    /// mi::base::Handle<nv::index::ISparse_volume_subset> attribute_set(
    ///     factory->create_data_subset<nv::index::ISparse_volume_subset>(descriptor, parameter));
    /// \endcode
    ///
    /// \param[in] dataset_attrib_set_desc  A attribute set descriptor.
    ///
    /// \param[in] parameter                Optional parameter string typically used to customizer the data subset creations.  
    ///
    /// \return Returns a typed data subset.
    ///
    template <class T>
    T* create_data_subset(
        const IDistributed_data_attribute_set_descriptor*   dataset_attrib_set_desc,
        const char*                                         parameter = 0)
    {
        T* t = static_cast<T*>(create_data_subset(typename T::IID(), dataset_attrib_set_desc, parameter));
        return t;
    }

    /// Creates a specific attribute set descriptor instance.
    ///
    /// \param[in] class_id     The uuid specifies the attribute set descriptor type, e.g., 
    ///                         \c ISparse_volume_attribute_set_descriptor, that shall be 
    ///                         instantiated and returned.
    ///
    /// \return                 Returns a specific attribute set descriptor. If the 
    ///                         type is not support, then the method retuns a \c nullptr.
    ///
    virtual IDistributed_data_attribute_set_descriptor* create_attribute_set_descriptor(
        const mi::base::Uuid& class_id) const = 0;

    /// Convenience template function for creating a typed attribute-set descriptor instance.
    /// 
    /// \code
    /// //
    /// // Usage:
    /// //
    /// mi::base::Handle<nv::index::ISparse_volume_attribute_set_descriptor> attribute_set(
    ///     factory->create_attribute_set_descriptor<nv::index::ISparse_volume_attribute_set_descriptor>());
    /// \endcode
    ///
    /// \return                 Returns a typed attribute set descriptor.
    ///
    template <class T>
    T* create_attribute_set_descriptor()
    {
        return static_cast<T*>(create_attribute_set_descriptor(typename T::IID()));
    }
};


} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_DATA_SUBSET_H
