/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling corner-point grid rendering properties.

#ifndef NVIDIA_INDEX_ICORNER_POINT_GRID_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_ICORNER_POINT_GRID_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// The interface class representing rendering properties for corner-point grid data.
///
/// \ingroup nv_index_scene_description_attribute
///
class ICorner_point_grid_rendering_properties :
    public mi::base::Interface_declare<0x4df63b2c,0x7e24,0x4e16,0xa9,0x64,0x2e,0xbd,0x41,0x9e,0xc4,0x51,
                                       nv::index::IAttribute>
{
public:
    /// Set the sampling distance used for a corner-point grid scene element (\c ICorner_point_grid).
    /// The default value used is 1.0f.
    /// 
    /// \param[in]  sample_dist     Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_sampling_distance(mi::Float32 sample_dist) = 0;
    /// Returns the sampling distance used for a corner-point grid scene element (\c ICorner_point_grid).
    virtual mi::Float32                 get_sampling_distance() const = 0;

    /// Set the reference sampling distance used for a corner-point grid scene element (\c ICorner_point_grid).
    /// The default value used is 1.0f. The reference sampling distance is used during the volume rendering to steer
    /// the opacity correction and therefore the appearance of the volume display.
    /// 
    /// \param[in]  s   Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_reference_sampling_distance(mi::Float32 s) = 0;
    /// Returns the reference sampling distance used for a corner-point grid scene element (\c ICorner_point_grid).
    virtual mi::Float32                 get_reference_sampling_distance() const = 0;

    /// Enable or disable pre-integrated volume rendering for a corner-point grid scene element (\c ICorner_point_grid).
    /// Per default this is enabled.
    /// 
    /// \param[in] enable           Enables the pre-integrated volume rendering technique
    ///                             if (\c true) otherwise the technique will be disabled (\c false).
    /// 
    virtual void                        set_preintegrated_volume_rendering(bool enable) = 0;
    /// Returns if the pre-integrated volume rendering is enabled or disabled.
    virtual bool                        get_preintegrated_volume_rendering() const = 0;

    /// Internal debugging options applied to the visualization.
    /// \param[in] o                Debug option applied to the visualization.   
    virtual void                        set_debug_visualization_option(mi::Uint32 o) = 0;
    /// Internal debugging options applied to the visualization.
    /// \return                     Returns the applied debug option.   
    virtual mi::Uint32                  get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ICORNER_POINT_GRID_RENDERING_PROPERTIES_H
