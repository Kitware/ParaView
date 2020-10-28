/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Base class representing the surface appearance of shapes in the scene description.

#ifndef NVIDIA_INDEX_IMATERIAL_H
#define NVIDIA_INDEX_IMATERIAL_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// The material base class. Derived classes define the material properties of shapes. The material
/// properties are applied to shapes together with the direct illumination using light sources. The
/// material properties, e.g., the ambient, diffuse, and specular surface reflectivity, the shininess
/// of the surface and its transparency shall be defined by a material attribute as part of the 
/// scene description. Only one material property can be active at the same time. All graphics
/// elements that are affected by the material shall then be shaded in accordance to the material
/// properties.
///
/// \ingroup nv_index_scene_description_attribute
///
class IMaterial :
        public mi::base::Interface_declare<0xaaf6c278,0xf36f,0x4d00,0x83,0xd6,0xfc,0x9a,0x4c,0xb3,0x2d,0x57,
                                           nv::index::IAttribute>
{
};

/// Simple OpenGL Phong material property.
///
/// An implementation of the material base class provides the material properties for the standard
/// Phong lighting model as implemented in OpenGL. The following terms define the material
/// properties:
///
/// - Ambient term. An ambient term accounts for light bounced around in the scene as if
/// it comes from everywhere. The ambient light does not appear to come from any particular
/// direction but from all directions. Therefore, the ambient lighting term does not depend
/// on a light's position.
/// Let the parameter KA represent the material's ambient reflectance and the parameter
/// global_ambient represent the color of the incoming ambient light then the ambient term is:
/// ambient = KA x global_ambient
///
/// - Diffuse term. The diffuse term accounts for directed light reflected off the surface
/// equally in all directions. In a microscopic scale, diffuse materials are rough and reflect
/// light bounces off in all directions. The amount of light reflected is proportional to the
/// angle of incidence of the incoming light (Lambertian reflectance).
/// Let the parameter KD represent the material's diffuse reflectance, the parameter
/// light_diffuse represent the color of the diffuse incoming light contribution, the
/// parameter N represent the normal at a surface position, the parameter L represent the
/// normalized incident direction of the incoming light, and P the surface position to be shaded,
/// then the diffuse term is:
/// diffuse = KD x global_diffuse x max(dot(L, N), 0.0)
///
/// - Specular term. The specular term represents light scattered mostly around the mirroring
/// direction at the surface position. The specular term is most striking on shiny surfaces and
/// its contribution depends on the surface's orientation towards the viewer/camera. The specular
/// term is affected by the specular color contribution of the light sources and the material as
/// well as by the surface's shininess. Shinier surfaces have smaller, tighter highlights and less
/// shiny materials have larger highlights.
/// Let the parameter KS represent the material's specular reflectance, the parameter
/// light_specular represent the color of the specular incoming light contribution, the parameter
/// shininess represent the material's shininess, the parameter N represent the normal at a surface
/// position, the parameter L represent the normalized incident direction of the incoming light,
/// the parameter V represent the normalized incident vector towards the camera, the parameter H
/// represent the normalized vector halfway between V and L, and P the surface position to be shaded,
/// then the diffuse term is:
/// specular = KS x global_specular x pow(max(dot(N, H), 0.0), shininess)
///
/// The term that describes the surface's light reflectance then is:
/// surface_color = ambient + diffuse + specular
///
/// \ingroup nv_index_scene_description_attribute
///
class IPhong_gl :
        public mi::base::Interface_declare<0x9202fbb8,0x4c0f,0x442a,0xad,0x31,0x4f,0x0b,0x30,0x3d,0xc6,0xad,
                                           nv::index::IMaterial>
{
public:
    /// Sets the ambient term of the material.
    /// \param[in] ka   Ambient term, the alpha component is ignored.
    virtual void set_ambient(const mi::math::Color_struct& ka) = 0;

    /// Returns the ambient term of the material.
    /// \return color of ambient term
    virtual const mi::math::Color_struct& get_ambient() const = 0;

    /// Sets the diffuse term of the material.
    /// \param[in] kd   Diffuse term, the alpha component is ignored.
    virtual void set_diffuse(const mi::math::Color_struct& kd) = 0;

    /// Returns the diffuse term of the material.
    /// \return color of diffuse term
    virtual const mi::math::Color_struct& get_diffuse() const = 0;

    /// Sets the specular term of the material.
    /// \param[in] ks   Specular term, the alpha component is ignored.
    virtual void set_specular(const mi::math::Color_struct& ks) = 0;

    /// Returns the specular term of the material.
    /// \return color of specular term
    virtual const mi::math::Color_struct& get_specular() const = 0;

    /// Sets the specular shininess.
    /// \param[in] shininess    Shininess, typical values are between 5 and 100
    virtual void set_shininess(mi::Float32 shininess) = 0;

    /// Returns the specular shininess.
    /// \return shininess value
    virtual mi::Float32 get_shininess() const = 0;

    /// Sets the opacity of the material.
    /// \param[in] opacity    Fully opaque at 1.0, fully transparent at 0.0.
    virtual void set_opacity(mi::Float32 opacity) = 0;

    /// Returns the opacity of the material.
    /// \return opacity value
    virtual mi::Float32 get_opacity() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IMATERIAL_H
