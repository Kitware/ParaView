/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling regular height-field rendering properties.

#ifndef NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// @ingroup nv_index_scene_description_attribute

/// The interface class representing rendering properties for regular height-field data.
///
class IRegular_heightfield_rendering_properties :
    public mi::base::Interface_declare<0xaf1a0bad,0x1c56,0x495d,0x9f,0xb8,0x71,0x54,0x2d,0x25,0xb0,0x8e,
                                       nv::index::IAttribute>
{
public:
    virtual void            set_lod_rendering_enabled(bool enable_lod_rendering) = 0;
    virtual bool            get_lod_rendering_enabled() const = 0;

    virtual void            set_lod_pixel_threshold(mi::Float32 pixel_threshold) = 0;
    virtual mi::Float32     get_lod_pixel_threshold() const = 0;

    // internal use only
    virtual void            set_debug_visualization_option(mi::Uint32 o) = 0;
    virtual mi::Uint32      get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IREGULAR_HEIGHTFIELD_RENDERING_PROPERTIES_H
