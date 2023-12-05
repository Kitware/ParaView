/******************************************************************************
 * Copyright 2023 NVIDIA Corporation. All rights reserved.
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

// forward declarations
class IParticle_volume_subset;
class IParticle_volume_subset_device;

/// Attribute-set descriptor for particle volume subsets. This interface is used to configure a set of
/// attributes for a particle volume subset to input into the NVIDIA IndeX library.
///
/// \ingroup nv_index_data_subsets
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
        Distributed_data_attribute_format   format;     ///< Attribute format. See \c Distributed_data_attribute_format.
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

    /// Get maximum number of attributes.
    virtual mi::Uint32  get_attribute_count_limit() const = 0;

};

/// Distributed data storage class for particle volume subsets.
///
/// Data access (e.g., import, editing) for particle volume data associated with \c IParticle_volume_scene_element 
/// instances is performed through instances of this subset class. A subset of a particle volume is defined
/// by all particles intersecting a rectangular subregion of the entire scene/dataset. This interface class
/// provides methods to input volume data for one or multiple attributes of a dataset.
///
/// \ingroup nv_index_data_subsets
///
class IParticle_volume_subset :
    public mi::base::Interface_declare<0x7694224f,0x93e6,0x4d5c,0x91,0x82,0xad,0x90,0x6a,0xb1,0x12,0x70,
                                       IDistributed_data_subset>
{
public:
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
    /// \note When initializing the internal storage with a fixed radius for all particles in the volume
    ///       there will be no particle radii storage generated and \c get_radii_buffer() will always
    ///       return a nullptr.
    ///
    /// \param[in]  nb_subset_particles     The number of particles in this subset instance.
    /// \param[in]  fixed_particle_radius   The fixed particle radius to be used for the volume subset.
    ///                                     A value less than or equal to 0.0f indicates the use of
    ///                                     individual particle radii.
    /// \param[in]  enable_particle_normals If true, then storage for particle normals is created.
    ///
    /// \return                             True if a storage generation succeeded, false otherwise.
    ///
    virtual bool generate_volume_storage(
        mi::Size    nb_subset_particles,
        mi::Float32 fixed_particle_radius = -1.0f,
        bool        enable_particle_normals = false) = 0;

    /// Returns the number of particles in the current subset.
    ///
    /// \return    The number of particles in the current subset
    ///
    virtual mi::Size                                    get_number_of_particles() const = 0;

    /// Get the particle positions buffer.
    ///
    /// After successfully generating the subsets volume data storage a valid pointer to the particle
    /// positions buffer is returned. If a prior \c generate_volume_storage call failed nullptr is returned.
    ///
    /// \return     Pointer to the internal positions buffer.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 3>*    get_positions_buffer() const = 0;

    /// Get the particle radii buffer.
    ///
    /// After successfully generating the subsets volume data storage a valid pointer to the particle
    /// radii buffer is returned. If a prior \c generate_volume_storage call failed nullptr is returned.
    ///
    /// \note If the storage was initialized with a fixed radius for all particles in the volume this
    ///       function will always return nullptr;
    ///
    /// \return     Pointer to the internal radii buffer.
    ///
    virtual mi::Float32*                                get_radii_buffer() const = 0;

    /// Get the fixed radius set for the particle volume.
    ///
    /// If the subset storage was generated using a fixed radius this function will return that value.
    /// Otherwise it will return a value less than or equal to 0.0f;
    ///
    /// \return     Fixed radius set for the particle volume.
    ///
    virtual mi::Float32                                 get_fixed_radius() const = 0;

    /// Get the particle normals buffer.
    ///
    /// After successfully generating the subsets volume data storage a valid pointer to the particle
    /// normals buffer is returned. If a prior \c generate_volume_storage call failed nullptr is returned.
    ///
    /// \return     Pointer to the internal normals buffer.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 3>*    get_normals_buffer() const = 0;


    /// Get a particle attributes buffer.
    ///
    /// After successfully generating the subsets volume data storage a valid pointer to the particle
    /// attributes buffer according to the requested attribute index is returned. If a prior call to
    /// \c generate_volume_storage failed or an invalid attribute index is specified nullptr is returned.
    ///
    /// \param[in]  attrib_index            The storage index of the requested attribute set.
    ///
    /// \return     Pointer to the internal attribute set buffer.
    ///
    virtual void*                                       get_attribute_buffer(mi::Uint32 attrib_index) const = 0;


    /// Access the device particle volume subset for direct access to device resources.
    ///
    /// \return     Device subset interface for direct access of device resources.
    ///             May return \c NULL if data is not on a GPU device.
    /// 
    virtual IParticle_volume_subset_device*             get_device_subset() const = 0;

};


/// Distributed data storage class for particle volume subsets hosted on a GPU device.
///
/// Data access for device particle data associated with \c IParticle_volume_scene_element instances
/// is performed through implementations of this interface.
/// 
/// \ingroup nv_index_data_subsets
/// 
class IParticle_volume_subset_device :
    public mi::base::Interface_declare<0x7259b57b,0x18e8,0x411e,0x8f,0x28,0xfd,0xad,0x94,0xf0,0x1f,0xd6,
                                       IDistributed_data_subset_device>
{
public:
    /// Data format of particle radii.
    enum Particle_radii_format
    {
        PVOL_radii_none,                            ///< no particle radii present
        PVOL_radii_f32                              ///< particle radii in \c float format (\c mi::Float32)
    };

    /// Data format of particle normals.
    enum Particle_normals_format
    {
        PVOL_normals_none,                          ///< no particle normals present
        PVOL_normals_f32_3,                         ///< particle normals in \c float3 format (\c mi::math::Vector_struct< mi::Float32, 3 >)
        PVOL_normals_compressed_1_32                ///< particle normals in compressed format 1 (\c sizeof(uint32), 'compress_normal_uint_sphere')
    };

    /// Organization of particle geometry buffers for positions, radii and normals.
    enum Particle_storage_format
    {
        PVOL_geometry_storage_separate,             ///< geometry buffers are separate
        PVOL_geometry_storage_combined,             ///< geometry buffers are consecutive arrays in order: positions, radii, normals
        PVOL_geometry_storage_interleaved           ///< geometry buffers are interleaved, element components in order: position, radius, normal
    };

    enum Particle_geometry_buffer
    {
        PVOL_buffer_base,                           ///< base buffer, used for combined or interleaved storage formats
        PVOL_buffer_positions,                      ///< buffer for particle positions
        PVOL_buffer_radii,                          ///< buffer for particle radii
        PVOL_buffer_normals                         ///< buffer for particle normals
    };

    /// Parameters of particle geometry.
    struct Geometry_parameters
    {
        mi::Uint32                  nb_particles;                   ///< number of particles in subset
        Particle_radii_format       radii_format;                   ///< format and presence of radii
        Particle_normals_format     normals_format;                 ///< format and presence of normals
        Particle_storage_format     storage_format;                 ///< buffer organization of geometry storage
        mi::Uint32                  element_stride;                 ///< element stride in bytes for interleaved data (4-byte aligned)
        mi::Uint32                  component_offset_positions;     ///< byte offset for positions per element in interleaved data 
        mi::Uint32                  component_offset_radii;         ///< byte offset for radii per element in interleaved data 
        mi::Uint32                  component_offset_normals;       ///< byte offset for normals per element in interleaved data 
    };

    /// Parameters of particle size.
    struct Particle_size_parameters
    {
        mi::Float32                 fixed_radius;                   ///< fixed particle radius
        //mi::Float32                 min_radius;
        //mi::Float32                 max_radius;
    };


    /// Get parameters of particle geometry.
    virtual Geometry_parameters         get_geometry_parameters() const = 0;

    /// Get parameters of particle size.
    virtual Particle_size_parameters    get_particle_size_parameters() const = 0;

    /// Change parameters of particle size.
    ///
    /// \param      params          New parameters of particle geometry.
    /// \return     Returns false if change was invalid.
    ///
    virtual bool                        set_particle_size_parameters(
                                            const Particle_size_parameters& params) = 0;

    /// Get current geometry device buffer.
    ///
    /// Elements of selected buffer are consecutive for format \c PVOL_geometry_storage_separate and \c PVOL_geometry_storage_combined. 
    /// Elements have a stride in format \c PVOL_geometry_storage_interleaved (\see Geometry_parameters).
    ///
    /// \param      buffer_selector Buffer selector. Use \c PVOL_buffer_base for interleaved or combined storage format.
    /// \return     Pointer to geometry device storage (internal or adopted) or \c NULL, if no storage.
    ///
    virtual void*                       get_geometry_storage(
                                            Particle_geometry_buffer buffer_selector) = 0;


    /// Resize internal geometry/attribute storage using \c Geometry_parameters.
    ///
    /// If parameters specify zero particles, then free all internal device storage.
    /// If external storage is referenced, then storage references will be reset.
    /// All particle geometry data will be reset and needs to be updated.
    ///
    /// It might fail to change the storage format, an implementation might not support all formats.
    /// It might fail to change the data format for components (position, radii, normals).
    ///
    /// \param      params          Parameters of geometry storage, including number of particles.
    /// \return     Returns false if allocation failed or parameters are invalid.
    ///
    virtual bool                        resize_storage(
                                            const Geometry_parameters& params) = 0;


    /// Adopt external geometry storage using \c Geometry_parameters.
    ///
    /// If internal geometry storage exists, it will be released.
    /// If parameters specify zero particles, then storage references will be reset.
    ///
    ///
    /// \param      params                      Parameters of geometry storage, including number of particles.
    /// \param      geometry_device_buffers     Pointer to array of device pointers for geometry data, in order
    ///                                         If storage is interleaved or combined, then one pointer is expected (\c PVOL_buffer_base).
    ///                                         if storage is separate, then the array of device pointers should have three elements 
    ///                                         [positions, radii, normals].
    /// \param      byte_sizes_of_buffers       Pointer to array of buffer sizes in bytes (\see geometry_device_buffers).
    ///                                         The real buffer size  is allowed to be larger than the required size.
    /// \param      nb_buffers                  Number of buffers.
    /// \return     Returns false if buffer addresses are incomplete or parameters are invalid.
    ///
    virtual bool                        adopt_geometry_storage(
                                            const Geometry_parameters&  params,
                                            void* const                 geometry_device_buffers [],
                                            mi::Size const              byte_sizes_of_buffers [],
                                            mi::Uint32                  nb_buffers) = 0;


    /// Get particle attribute formats.
    ///
    /// \param      formats     Buffer to receive format codes (\see \c Distributed_data_attribute_format)
    /// \param      nb_formats  Size of buffer \c formats.
    /// \return     Returns false if buffer is invalid.
    ///
    virtual bool                        get_attribute_formats(
                                            mi::Uint32*                 formats,
                                            mi::Uint32                  nb_formats) const = 0;

    /// Set particle attribute formats.
    ///
    /// Changing the format of an attribute invalidates attribute storage.
    /// Using nb_formats == 0 will clear all attributes.
    ///
    /// \param      formats     Buffer of format codes (\see \c Distributed_data_attribute_format)
    /// \param      nb_formats  Size of buffer \c formats.
    /// \return     Returns false if format data is invalid.
    ///
    virtual bool                        set_attribute_formats(
                                            const mi::Uint32*           formats,
                                            mi::Uint32                  nb_formats) = 0;


    /// Get current attribute device buffer for given attribute index.
    ///
    /// The attribute set is fixed at creation time.
    /// It is defined in \c IParticle_volume_attribute_set_descriptor  (\see IParticle_volume_subset::get_attribute_set_descriptor() ).
    ///
    /// \param      attrib_index    The storage index of the attribute set.
    /// \return     Returns \c nullptr if attribute index is invalid.
    ///
    virtual void*                       get_attribute_storage(
                                            mi::Uint32          attrib_index) = 0;

    /// Adopt external attribute device buffer for given attribute index.
    ///
    /// If internal attribute storage exists, it will be released.
    /// The attribute set is fixed at creation time.
    /// It is defined in \c IParticle_volume_attribute_set_descriptor  (\see IParticle_volume_subset::get_attribute_set_descriptor() ).
    ///
    /// \param      attrib_index            The storage index of the attribute set.
    /// \param      attribute_storage       Pointer to device buffer of attribute set.
    /// \param      byte_size_of_storage    Byte size of device buffer, allowed to be larger than required size.
    /// \return     Returns false if attribute index or given pointer is invalid.
    ///
    virtual bool                        adopt_attribute_storage(
                                            mi::Uint32          attrib_index,
                                            void*               attribute_storage,
                                            mi::Size            byte_size_of_storage) = 0;

    /// Get location of geometry storage.
    /// \return     \c true if internal geometry storage, \c false if external storage or no storage 
    virtual bool                        is_internal_geometry_storage() const = 0;

    /// Get location of attribute storage.
    /// \return     \c true if internal attribute storage, \c false if external storage or no storage 
    virtual bool                        is_internal_attribute_storage() const = 0;

    /// Get size in bytes of geometry storage.
    ///
    /// \param[out]     storage_sizes      Pointer to array of sizes. If separate geometry buffers, one element for positions, radii, normals.
    /// \param          nb_storage_sizes   Number of sizes to write to \c storage_sizes.
    ///
    virtual void                        get_geometry_storage_byte_size(
                                            mi::Size            storage_sizes[],
                                            mi::Uint32          nb_storage_sizes) const = 0;

    /// Get size in bytes of attribute storage.
    ///
    /// \param      attrib_index    The storage index of the attribute set.
    ///
    virtual mi::Size                    get_attribute_storage_byte_size(
                                            mi::Uint32          attrib_index) const = 0;


    /// Signal a data change in adopted storage.
    //virtual void                    signal_data_change(bool geometry, bool attributes) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IPARTICLE_VOLUME_SUBSET_H
