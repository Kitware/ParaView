/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Base class declaring the functionality of higher-level shapes.

#ifndef NVIDIA_INDEX_ISHAPE_H
#define NVIDIA_INDEX_ISHAPE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_element.h>

namespace nv
{
namespace index
{

/// The base class of all shapes. A shape can either be defined in object space or in image space.
/// All types of shapes can be pickable.
/// @ingroup nv_index_scene_description_shape
///
class IShape : public mi::base::Interface_declare<0xa0df402e, 0x4e28, 0x4dc4, 0x81, 0x4b, 0x6b,
                 0x68, 0x74, 0xeb, 0xd0, 0xd2, nv::index::IScene_element>
{
public:
  /// Each shape may be pickable, so that the ray cast through the scene can intersect
  /// the shape. The intersection information will be returned by the pick operation.
  ///
  /// \return     The shape may or may not be pickable.
  ///
  virtual bool get_pickable() const = 0;

  /// Sets the pickable state of the shape.
  ///
  /// \param[in] pickable    Shape is pickable
  ///
  virtual void set_pickable(bool pickable) = 0;
};

/// Object space shapes are defined in the 3D scene, i.e., their position as well as their extent is
/// affected by the applied transformations present in the scene description.
/// @ingroup nv_index_scene_description_shape
///
class IObject_space_shape : public mi::base::Interface_declare<0xca3aacf8, 0x99af, 0x46dc, 0xa0,
                              0xa7, 0xe9, 0xb7, 0xf0, 0xed, 0x68, 0xda, nv::index::IShape>
{
public:
  /// An object space shape must define its bounding box. The bounding box is always defined in the
  /// shape's local coordinate system. The rendering traversals require the bounding box
  /// for accurate and efficient rendering.
  ///
  /// \return     The bounding box of the shape in its local coordinate system.
  ///
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_bounding_box() const = 0;
};

/// Image space shapes have an extent defined in two-dimensional image space even though their
/// position may be affected by the applied transformations present in the scene description. For
/// instance, points, lines, and labels may have their vertex position defined in 3D space, but
/// their width is defined in pixels.
/// @ingroup nv_index_scene_description_shape
///
class IImage_space_shape : public mi::base::Interface_declare<0xb98467a6, 0xf317, 0x459d, 0xa9,
                             0x64, 0xc9, 0x8f, 0x4c, 0x37, 0x54, 0x71, nv::index::IShape>
{
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ISHAPE_H
