/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling regular volume rendering properties.

#ifndef NVIDIA_INDEX_IREGULAR_VOLUME_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IREGULAR_VOLUME_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// @ingroup nv_index_scene_description_attribute

/// The interface class representing rendering properties for regular volume data.
///
class IRegular_volume_rendering_properties :
    public mi::base::Interface_declare<0x89f2350b,0xf1d9,0x4924,0x92,0x92,0x64,0x74,0xea,0xb5,0x12,0xd0,
                                       nv::index::IAttribute>
{
public:
    /// Shading modes for direct volume rendering of regular volume data.
    enum Shading_mode
    {
        NO_LIGHTING             = 0x00, ///< No lighting calculated for volume samples
        PHONG_MATERIAL_LIGHTING         ///< Basic Lambertian diffuse reflectance with Blinn-Phong specular lighting
                                        ///< using the values of an additional \c IPhong_gl material attribute to the
                                        ///< regular volume scene element this attribute is associated with.
    };

    virtual void            set_shading_mode(Shading_mode m) = 0;
    virtual Shading_mode    get_shading_mode() const = 0;

    virtual void            set_shading_gradient_threshold(mi::Float32 t) = 0;
    virtual mi::Float32     get_shading_gradient_threshold() const = 0;

    virtual void            set_reference_step_size(mi::Float32 s) = 0;
    virtual mi::Float32     get_reference_step_size() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IREGULAR_VOLUME_RENDERING_PROPERTIES_H
