/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \brief Scene attribute controlling particle volume rendering properties.

#ifndef NVIDIA_INDEX_IPARTICLE_VOLUME_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IPARTICLE_VOLUME_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// Available rendering modes for particle volume elements.
///
/// \ingroup nv_index_scene_description_attribute
///
enum Particle_volume_render_mode
{
    PARTICLE_VOLUME_RENDER_MODE_RBF             = 0x00u,    ///< Direct volume rendering using radial basis functions (RBF) (see \c Particle_volume_RBF_falloff_kernel)
    PARTICLE_VOLUME_RENDER_MODE_SOLID_BOX,                  ///< Render particles as solid geometries (boxes)
    PARTICLE_VOLUME_RENDER_MODE_SOLID_CONE,                 ///< Render particles as solid geometries (cones)
    PARTICLE_VOLUME_RENDER_MODE_SOLID_CYLINDER,             ///< Render particles as solid geometries (cylinders)
    PARTICLE_VOLUME_RENDER_MODE_SOLID_SPHERE,               ///< Render particles as solid geometries (spheres)
    PARTICLE_VOLUME_RENDER_MODE_SOLID_INDIVIDUAL,           ///< Render particles as solid geometries, where the shape is selected by a second uint8-typed
                                                            ///< attribute (index 1). The values stored per-particle need to match the values from
                                                            ///< PARTICLE_VOLUME_RENDER_MODE_SOLID_BOX to PARTICLE_VOLUME_RENDER_MODE_SOLID_SPHERE (1..4)

    PARTICLE_VOLUME_RENDER_MODE_COUNT
};

/// Available radial basis-functions for the direct volume rendering mode of
/// particle volume primitives. These define the falloff of the influence of
/// a particle over its volume of influence defined by its radius.
///
/// \ingroup nv_index_scene_description_attribute
///
enum Particle_volume_RBF_falloff_kernel
{
    PARTICLE_VOLUME_RBF_FALLOFF_CONSTANT        = 0x00,
    PARTICLE_VOLUME_RBF_FALLOFF_LINEAR,
    PARTICLE_VOLUME_RBF_FALLOFF_HERMITE,
    PARTICLE_VOLUME_RBF_FALLOFF_SPLINE,

    PARTICLE_VOLUME_RBF_FALLOFF_COUNT
};

/// The interface class representing rendering properties for particle volume data.
///
/// \ingroup nv_index_scene_description_attribute
///
class IParticle_volume_rendering_properties :
    public mi::base::Interface_declare<0x82869c9e,0x96a,0x4e86,0x9f,0xde,0x40,0xa8,0x62,0xcd,0xa0,0xa1,
                                       nv::index::IAttribute>
{
public:
    /// Set the sampling distance used for a particle volume scene element (\c Iparticle_volume_scene_element)
    /// when applying the direct volume rendering mode (\c PARTICLE_VOLUME_RENDER_MODE_RBF).
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
    /// \param[in]  ref_sample_dist Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_reference_sampling_distance(mi::Float32 ref_sample_dist) = 0;
    /// Returns the reference sampling distance used for a particle volume scene element (\c Iparticle_volume_scene_element).
    virtual mi::Float32                 get_reference_sampling_distance() const = 0;

    /// Set rendering mode used for the visualization of a particle volume scene element (\c Iparticle_volume_scene_element).
    /// 
    /// \param[in]  render_mode     Rendering mode identifier (default value is PARTICLE_VOLUME_RENDER_MODE_RBF).
    /// 
    virtual void                        set_rendering_mode(Particle_volume_render_mode render_mode) = 0;
    /// Returns the rendering mode used for rendering a particle volume scene element (\c Iparticle_volume_scene_element).
    virtual Particle_volume_render_mode get_rendering_mode() const = 0;


    /// Set the radial basis-function falloff kernel applied for the direct volume rendering mode.
    /// 
    /// \param[in]  falloff_kernel  RBF falloff kernel function (default value is PARTICLE_VOLUME_RBF_FALLOFF_LINEAR).
    /// 
    virtual void                        set_rbf_falloff_kernel(Particle_volume_RBF_falloff_kernel falloff_kernel) = 0;
    /// Returns the falloff kernel function applied for the radial basis function rendering (RBF) mode.
    virtual Particle_volume_RBF_falloff_kernel  get_rbf_falloff_kernel() const = 0;

    /// Set a override radius used as a fixed radius for all particles in the dataset. This setting allows to control the 
    /// size of the particles without modifying the particle radii through the data subset. This can be especially
    /// useful for the solid geometry rendering modes.
    ///
    /// \note Setting this override radius larger than the radii or fixed radius specified during data import may result
    ///       in rendering artifacts due to the existing data distribution of the subsets already created.
    ///
    virtual void                        set_override_fixed_radius(mi::Float32 override_radius) = 0;
    /// Returns the override radius applied to a particle volume scene element (\c Iparticle_volume_scene_element).
    virtual mi::Float32                 get_override_fixed_radius() const = 0;

    /// Internal debugging options applied to the visualization.
    /// \param[in] o    Debug option applied to the visualization.   
    virtual void                        set_debug_visualization_option(mi::Uint32 o) = 0;
    /// Internal debugging options applied to the visualization.
    /// \return         Returns the applied debug option.  
    virtual mi::Uint32                  get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IPARTICLE_VOLUME_RENDERING_PROPERTIES_H
