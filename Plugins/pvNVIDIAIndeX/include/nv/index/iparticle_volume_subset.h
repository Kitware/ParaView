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

};

/// Distributed data storage class for particle volume subsets.
///
/// Data access (e.g., import, editing) for particle volume data associated with \c IParticle_volume_scene_element 
/// instances is performed through instances of this subset class. A subset of a particle volume is defined
/// by all particles intersecting a rectangular subregion of the entire scene/dataset. This interface class
/// provides methods to input volume data for one or multiple attributes of a dataset.
///
/// \ingroup nv_index_data_storage
///
class IParticle_volume_subset :
    public mi::base::Interface_declare<0x81b198e6,0x8eaf,0x49a8,0xb9,0xa3,0x66,0x62,0xc,0x27,0xb6,0x90,
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
    ///
    /// \return                             True if a storage generation succeeded, false otherwise.
    ///
    virtual bool generate_volume_storage(
        mi::Size    nb_subset_particles,
        mi::Float32 fixed_particle_radius = -1.0f) = 0;

    /// Returns the number of particles in the current subset.
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
    /// \note If the storage was initialized with a fixed radius for all particles in the volume this
    ///       function will always return nullptr;
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
