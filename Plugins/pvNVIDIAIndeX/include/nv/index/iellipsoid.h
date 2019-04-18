/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element representing a ellipsoid higher-level shape.

#ifndef NVIDIA_INDEX_IELLIPSOID_H
#define NVIDIA_INDEX_IELLIPSOID_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_shape
///
/// The set of higher-level 3D shapes part of the NVIDIA IndeX library include a 3D ellipsoid.
/// A 3D ellipsoid is defined by its center and three orthogonal radii.
/// The center and radii are defined in the ellipsoid's local coordinate system. The surface of
/// the ellipsoid is shaded using the material and light defined in the scene description.
///
class IEllipsoid : public mi::base::Interface_declare<0xa22f3595, 0xe462, 0x44ef, 0xa9, 0x23, 0x9b,
                     0x1d, 0xdc, 0x7a, 0xda, 0x50, nv::index::IObject_space_shape>
{
public:
  /// The center of the ellipsoid.
  ///
  /// \return     The center of the ellipsoid.
  ///
  virtual const mi::math::Vector_struct<mi::Float32, 3>& get_center() const = 0;
  /// Set the center of the ellipsoid
  ///
  /// \param      center          The center of the ellipsoid.
  ///
  virtual void set_center(const mi::math::Vector_struct<mi::Float32, 3>& center) = 0;

  /// Returns the semi-axes of the ellipsoid.
  /// The three semi-axes must be orthogonal. Therefore, for the c-axis only the length needs to
  /// be specified here, while the direction will be computed internally.
  ///
  /// \param[out]     a       The axis of the ellipsoid in x direction.
  ///
  /// \param[out]     b       The axis of the ellipsoid in y direction.
  ///
  /// \param[out]     c       The axis of the ellipsoid in z direction.
  ///
  virtual void get_semi_axes(mi::math::Vector_struct<mi::Float32, 3>& a,
    mi::math::Vector_struct<mi::Float32, 3>& b,
    mi::math::Vector_struct<mi::Float32, 3>& c) const = 0;

  /// Sets the semi-axes of the ellipsoid.
  /// The three semi-axes must be orthogonal. Therefore, for the c-axis only the length needs to
  /// be specified here, while the direction will be computed internally. A radius of zero for
  /// either the x-, y-, and z-direction is invalid and will be ignored..
  ///
  /// \param[in]      a       The axis of the ellipsoid in x direction.
  ///
  /// \param[in]      b       The axis of the ellipsoid in y direction.
  ///
  /// \param[in]      c       The axis of the ellipsoid in z direction.
  ///
  virtual void set_semi_axes(const mi::math::Vector_struct<mi::Float32, 3>& a,
    const mi::math::Vector_struct<mi::Float32, 3>& b, mi::Float32 c) = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IELLIPSOID_H
