/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Distributed subsets of particle volume datasets.

#ifndef NVIDIA_INDEX_IPARTICLE_VOLUME_SUBSET_H
#define NVIDIA_INDEX_IPARTICLE_VOLUME_SUBSET_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/math/vector.h>

#include <nv/index/idistributed_data_subset.h>

namespace nv {
namespace index {

/// Voxel format of particle volume attrib data.
///
/// \ingroup nv_index_data_storage
///
enum Particle_volume_attrib_format
{
    PARTICLE_VOLUME_ATTRIB_FORMAT_UINT8          = 0x00,    ///< Scalar attrib format with uint8 precision
    PARTICLE_VOLUME_ATTRIB_FORMAT_UINT8_2,                  ///< Vector attrib format with 2 components and uint8 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_UINT8_4,                  ///< Vector attrib format with 4 components and uint8 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_SINT8,                    ///< Scalar attrib format with sint8 precision
    PARTICLE_VOLUME_ATTRIB_FORMAT_SINT8_2,                  ///< Vector attrib format with 2 components and sint8 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_SINT8_4,                  ///< Vector attrib format with 4 components and sint8 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_UINT16,                   ///< Scalar attrib format with uint16 precision
    PARTICLE_VOLUME_ATTRIB_FORMAT_UINT16_2,                 ///< Vector attrib format with 2 components and uint16 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_UINT16_4,                 ///< Vector attrib format with 4 components and uint16 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_SINT16,                   ///< Scalar attrib format with sint16 precision
    PARTICLE_VOLUME_ATTRIB_FORMAT_SINT16_2,                 ///< Vector attrib format with 2 components and sint16 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_SINT16_4,                 ///< Vector attrib format with 4 components and sint16 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_FLOAT16,                  ///< Scalar attrib format with float16 precision
    PARTICLE_VOLUME_ATTRIB_FORMAT_FLOAT16_2,                ///< Vector attrib format with 2 components and float16 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_FLOAT16_4,                ///< Vector attrib format with 4 components and float16 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_FLOAT32,                  ///< Scalar attrib format with float32 precision
    PARTICLE_VOLUME_ATTRIB_FORMAT_FLOAT32_2,                ///< Vector attrib format with 2 components and float32 precision per component
    PARTICLE_VOLUME_ATTRIB_FORMAT_FLOAT32_4,                ///< Vector attrib format with 4 components and float32 precision per component

    PARTICLE_VOLUME_ATTRIB_FORMAT_COUNT,
    PARTICLE_VOLUME_ATTRIB_FORMAT_INVALID = PARTICLE_VOLUME_ATTRIB_FORMAT_COUNT
};

#if 0
/// Volume device-data accessor interface.
///
/// An instance of this class will hold ownership of the device data. For this reason managing the life-time of an instance
/// is important for when to free up the data.
///
/// \note WORK IN PROGRESS
///  - for now uses the Particle_volume_attrib_format enum to represent volume type
///
/// \ingroup nv_index_data_storage
///
class IVolume_device_data_buffer :
    public mi::base::Interface_declare<0x55df860c,0x7afd,0x4c39,0x8e,0x45,0x2b,0x65,0xba,0x4d,0x3,0x8d>
{
public:
    virtual bool                                is_valid() const = 0;

    /// Voxel format of the volume data.
    virtual Particle_volume_attrib_format          get_data_format() const = 0;

    /// Volume data position in local volume space.
    virtual mi::math::Vector<mi::Sint32, 3>     get_data_position() const = 0;

    /// Volume data extent.
    virtual mi::math::Vector<mi::Uint32, 3>     get_data_extent() const = 0;
    
    /// Raw memory pointer to internal device-buffer data.
    virtual void*                               get_device_data() const = 0;

    /// The size of the buffer in Bytes.
    virtual mi::Size                            get_data_size() const = 0;

    /// GPU device id if the buffer is located on a GPU device,
    virtual mi::Sint32                          get_gpu_device_id() const = 0;
};
#endif

/// Attribute-set descriptor for particle volume subsets. This interface is used to configure a set of
/// attributes for a particle volume subset to input into the NVIDIA IndeX library.
///
/// \ingroup nv_index_data_storage
///
class IParticle_volume_attribute_set_descriptor :
    public mi::base::Interface_declare<0x27f160e3,0x1412,0x4264,0xa2,0x5b,0x89,0x3d,0xe6,0xaa,0xb3,0x37,
                                       IDistributed_data_attribute_set_descriptor>
{
public:
    /// Particle volume attribute parameters.
    ///
    /// This structure defines the basic parameters of a single attribute associated with the volume dataset.
    ///
    struct Attribute_parameters
    {
        Particle_volume_attrib_format  format;             ///< Attribute format. See \c Particle_volume_attrib_format.
    };

    /// Configure the parameters for an attribute for a particle volume subset.
    ///
    /// \param[in]  attrib_index    The storage index of the attribute.
    /// \param[in]  attrib_params   The attribute parameters for the given index.
    ///
    /// \return                     True when the attribute according to the passed index could be set up, false otherwise.
    ///
    virtual bool    setup_attribute(
                        mi::Uint32                  attrib_index,
                        const Attribute_parameters& attrib_params) = 0;

    /// Get the attribute parameters of a currently valid attribute for a given index.
    ///
    /// \param[in]  attrib_index    The storage index of the attribute.
    /// \param[out] attrib_params   The attribute parameters for the given index.
    ///
    /// \return                     True when the attribute according to the passed index could be found, false otherwise.
    ///
    virtual bool    get_attribute_parameters(
                        mi::Uint32                  attrib_index,
                        Attribute_parameters&       attrib_params) const = 0;

};

/// Distributed data storage class for particle volume subsets.
///
/// The data import for particle volume data associated with \c IParticle_volume_scene_element instances using
/// NVIDIA IndeX is performed through instances of this subset class. A subset of a particle volume is defined
/// by all particles intersecting  a rectangular subregion of the entire scene/dataset. This
/// interface class provides methods to input volume data for one or multiple attributes of a dataset.
///
/// \ingroup nv_index_data_storage
///
class IParticle_volume_subset :
    public mi::base::Interface_declare<0x81b198e6,0x8eaf,0x49a8,0xb9,0xa3,0x66,0x62,0xc,0x27,0xb6,0x90,
                                       IDistributed_data_subset>
{
public:
#if 0
    /// Definition of internal buffer information.
    ///
    /// The internal buffer information can be queried using the \c get_internal_buffer_info() method
    /// to gain access to the internal buffer data for direct write operations. This enables zero-copy
    /// optimizations for implementations of, e.g., \c IDistributed_compute_technique where large parts
    /// of the data-subset buffer can be written directly without going through the \c write() methods.
    ///
    /// \note The internal layout of the buffer of a volume-data brick is in a linear IJK/XYZ layout with
    ///       the I/X-component running fastest.
    ///
    struct Data_brick_buffer_info
    {
        void*           data;               ///< Raw memory pointer to internal buffer data.
        mi::Size        size;               ///< The size of the buffer in Bytes.
        mi::Sint32      gpu_device_id;      ///< GPU device id if the buffer is located on a GPU device,
                                            ///< -1 to indicate a host buffer.
        bool            is_pinned_memory;   ///< Flag indicating if a host buffer is a pinned (page-locked)
                                            ///< memory area.
    };
#endif

public:
    // * positions
    // * radii (if not fixed radius)
    // * attributes (per position)

    /// Returns the attribute-set descriptor of the subset.
    ///
    virtual const IParticle_volume_attribute_set_descriptor*  get_attribute_set_descriptor() const = 0;

    /// Generate the subsets particle volume data storage.
    ///
    /// This function initializes the internal storage buffers of this subset to the appropriate sizes
    /// according to the number of particles expected in the subset. Multiple calls to this function will
    /// invalidate previous internal buffers and new buffers are initialized according the newly declared
    /// number of particles.
    ///
    /// When initializing the internal storage with a fixed radius for all particles in the volume there
    /// will be no particle radii storage generated and \c get_radii_buffer() will always return nullptr.
    ///
    /// \param[in]  nb_subset_particles     The number of particles in this subset instance.
    /// \param[in]  fixed_particle_radius   The fixed particle radius to be used for the volume subset.
    ///                                     A value less than or equal to 0.0f indicates the use of
    ///                                     individual particle radii.
    ///
    /// \return                             True if a storage generation succeeded, false otherwise.
    ///
    // #todo limit this to uint32 in order to be able to use 32bit indices in the renderer... or prevent someone to input more?
    virtual bool generate_volume_storage(
        mi::Size    nb_subset_particles,
        mi::Float32 fixed_particle_radius = -1.0f) = 0;

    /// Get the number of particles in the current subset.
    ///
    /// \returns    The number of particles in the current subset
    ///
    virtual mi::Size                                    get_number_of_particles() const = 0;

    /// Get the particle positions buffer.
    ///
    /// After successfully generating the subsets volume data storage a valid pointer to the particle
    /// positions buffer is returned. If a prior \c generate_volume_storage call failed nullptr is returned.
    ///
    /// \returns    A pointer to the internal positions buffer.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 3>*    get_positions_buffer() const = 0;

    /// Get the particle radii buffer.
    ///
    /// After successfully generating the subsets volume data storage a valid pointer to the particle
    /// radii buffer is returned. If a prior \c generate_volume_storage call failed nullptr is returned.
    ///
    /// If the storage was initialized with a fixed radius for all particles in the volume this function
    /// will always return nullptr;
    ///
    /// \returns    A pointer to the internal radii buffer.
    ///
    virtual mi::Float32*                                get_radii_buffer() const = 0;

    /// Get the fixed radius set for the particle volume.
    ///
    /// If the subset storage was generated using a fixed radius this function will return that value.
    /// Otherwise it will return a value less than or equal to 0.0f;
    ///
    /// \returns    The fixed radius set for the particle volume.
    ///
    virtual mi::Float32                                 get_fixed_radius() const = 0;

    /// Get a particle attributes buffer.
    ///
    /// After successfully generating the subsets volume data storage a valid pointer to the particle
    /// attributes buffer according to the requested attribute index is returned. If a prior call to
    /// \c generate_volume_storage failed or an invalid attribute index is specified nullptr is returned.
    ///
    /// \param[in]  attrib_index            The storage index of the requested attribute set.
    ///
    /// \returns    A pointer to the internal attribute set buffer.
    ///
    virtual void*                                       get_attribute_buffer(mi::Uint32 attrib_index) const = 0;

};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IPARTICLE_VOLUME_SUBSET_H
