/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling regular height-field rendering properties.

#ifndef NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// Rendering properties for regular height-field data.
///
/// \ingroup nv_index_scene_description_attribute
///
class IRegular_heightfield_rendering_properties :
    public mi::base::Interface_declare<0xaf1a0bad,0x1c56,0x495d,0x9f,0xb8,0x71,0x54,0x2d,0x25,0xb0,0x8e,
                                       nv::index::IAttribute>
{
public:
    /// LOD for height field rendering can be enabled or disabled.
    /// \param[in] enable_lod_rendering     Enable or disable LOD rendering.
    virtual void            set_lod_rendering_enabled(bool enable_lod_rendering) = 0;
    /// LOD for height field rendering can be enabled or disabled.
    /// \return             Returns \c true if LOD rendering is enabled and \c false otherwise.
    virtual bool            get_lod_rendering_enabled() const = 0;

    /// A pixel threshold defines the transition between LODs.
    /// \param[in] pixel_threshold  The threshold in screen/pixel space. 
    virtual void            set_lod_pixel_threshold(mi::Float32 pixel_threshold) = 0;
    /// A pixel threshold defines the transition between LODs.
    /// \return                     Returns the threshold in screen/pixel space. 
    virtual mi::Float32     get_lod_pixel_threshold() const = 0;

    /// Internal debugging options applied to the visualization.
    /// \param[in] o    Debug option applied to the visualization.   
    virtual void            set_debug_visualization_option(mi::Uint32 o) = 0;
    /// Internal debugging options applied to the visualization.
    /// \return         Returns the applied debug option. 
    virtual mi::Uint32      get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_RENDERING_PROPERTIES_H
