/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Base classes representing light sources in the scene description.

#ifndef NVIDIA_INDEX_ILIGHT_H
#define NVIDIA_INDEX_ILIGHT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description_attribute

/// The light base class. Derived classes define the light sources.
/// Light sources emit light into the scene and enable the rendering system to shade
/// graphics elements such as heightfields or higher-level 3D shapes in a realistic way.
/// The illumination of surfaces using light sources provides visual cues in the scene
/// and enables users of the rendering software to orient themselves in 3D space.
///
/// Every light source is defined in the scene description hierarchy and affects all shapes
/// defined further down in the scene description's hierarchy in traversal order. The interface
/// class ILight represents a light source.
///
/// A light source exposes the interface methods to set and get the light's
/// intensity, i.e., the weighted color that the light emits, and to turn on and
/// off the light source.
///
class ILight : public mi::base::Interface_declare<0xbf099107, 0x7878, 0x4504, 0x94, 0x7e, 0xa8,
                 0x45, 0xa1, 0x93, 0x95, 0xa6, nv::index::IAttribute>
{
public:
  /// Sets the light intensity.
  ///
  /// \param[in] intensity Intensity of the light
  ///
  virtual void set_intensity(const mi::math::Color_struct& intensity) = 0;

  /// Returns the light intensity
  ///
  /// \return Light intensity
  virtual const mi::math::Color_struct& get_intensity() const = 0;
};

/// An interface class representing a directional light.
/// A directional light source illuminates all graphics elements equally from a given direction;
/// it defines an oriented light with an origin at infinity. That is, a directional light source
/// has a directional vector and parallel light rays travel along the direction. The directional
/// light contributes to diffuse and specular reflections but does not contribute to ambient
/// reflections.
///
class IDirectional_light : public mi::base::Interface_declare<0x4296b302, 0x4581, 0x435c, 0xb0,
                             0xf0, 0xfa, 0xdb, 0x32, 0xc2, 0x0c, 0xf4, nv::index::ILight>
{
public:
  /// Sets the direction of the directional light
  ///
  /// \param[in] d Direction vector
  ///
  virtual void set_direction(const mi::math::Vector_struct<mi::Float32, 3>& d) = 0;

  /// Returns the direction of the directional light
  ///
  /// \return Direction vector
  ///
  virtual const mi::math::Vector_struct<mi::Float32, 3>& get_direction() const = 0;
};

/// A directional headlight is just like a normal directional light, just the light direction is
/// specified in eye space instead of object space. This means that the light source will move with
/// the camera, giving the impression of a "headlight" that is fixed to the camera.
///
class IDirectional_headlight
  : public mi::base::Interface_declare<0x93bc91e1, 0xa6a8, 0x42a8, 0xbc, 0x86, 0xbf, 0x5c, 0xa5,
      0x0c, 0xa3, 0x24, nv::index::IDirectional_light>
{
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ILIGHT_H
