/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element representing a sphere higher-level shape.

#ifndef NVIDIA_INDEX_ISPHERE_H
#define NVIDIA_INDEX_ISPHERE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description_shape
///
/// A simple sphere.
///
class ISphere : public mi::base::Interface_declare<0x9b0af03a, 0x177c, 0x4127, 0x9c, 0x47, 0x8b,
                  0x5c, 0xc8, 0x36, 0x92, 0xaf, nv::index::IObject_space_shape>
{
public:
  /// Returns the position of the center of the sphere.
  ///
  /// \return Center of the sphere
  ///
  virtual mi::math::Vector_struct<mi::Float32, 3> get_center() const = 0;

  /// Sets the position of the center of the sphere
  ///
  /// \param[in] center Center position of the sphere
  ///
  virtual void set_center(const mi::math::Vector_struct<mi::Float32, 3>& center) = 0;

  /// Sets the radius of the sphere.
  ///
  /// \return     Radius of the sphere.
  ///
  virtual mi::Float32 get_radius() const = 0;

  /// Returns the radius of the sphere.
  ///
  /// \param[in] radius Radius of the sphere
  ///
  virtual void set_radius(mi::Float32 radius) = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ISPHERE_H
