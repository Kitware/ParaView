/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling particle volume rendering properties.

#ifndef NVIDIA_INDEX_IPARTICLE_VOLUME_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IPARTICLE_VOLUME_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// The interface class representing rendering properties for particle volume data.
///
/// \ingroup nv_index_scene_description_attribute
///
class IParticle_volume_rendering_properties :
    public mi::base::Interface_declare<0x82869c9e,0x96a,0x4e86,0x9f,0xde,0x40,0xa8,0x62,0xcd,0xa0,0xa1,
                                       nv::index::IAttribute>
{
public:
#if 0
    SAMPLING DISTANCE
    * hard one... depending on the particle size we need to sample inside their RBF footprint

    /// Set the sampling distance used for a particle volume scene element (\c Iparticle_volume_scene_element).
    /// The default value used is 1.0f.
    /// 
    /// \param[in]  sample_dist     Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_sampling_distance(mi::Float32 sample_dist) = 0;
    /// Returns the sampling distance used for a particle volume scene element (\c Iparticle_volume_scene_element).
    virtual mi::Float32                 get_sampling_distance() const = 0;

    /// Set the reference sampling distance used for a particle volume scene element (\c Iparticle_volume_scene_element).
    /// The default value used is 1.0f. The reference sampling distance is used during the volume rendering to steer
    /// the opacity correction and therefore the appearance of the volume display.
    /// 
    /// \param[in]  s   Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_reference_sampling_distance(mi::Float32 s) = 0;
    /// Returns the reference sampling distance used for a particle volume scene element (\c Iparticle_volume_scene_element).
    virtual mi::Float32                 get_reference_sampling_distance() const = 0;

    RENDERING MODE
    * RBF
    * Splat
    * Sphere

    /// Set the volume filter mode for a particle volume scene element (\c Iparticle_volume_scene_element).
    /// The default filter used is \c particle_VOLUME_FILTER_NEAREST.
    /// 
    /// \param[in]  filter_mode     Filter mode (default value is \c particle_VOLUME_FILTER_NEAREST).
    /// 
    virtual void                        set_filter_mode(particle_volume_filter_mode filter_mode) = 0;
    /// Returns the filter mode used for a particle volume scene element (\c Iparticle_volume_scene_element).
    virtual particle_volume_filter_mode   get_filter_mode() const = 0;
#endif
    /// Internal debugging options.
    virtual void                        set_debug_visualization_option(mi::Uint32 o) = 0;
    virtual mi::Uint32                  get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IPARTICLE_VOLUME_RENDERING_PROPERTIES_H
