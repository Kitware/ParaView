/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Base class representing the surface appearance of shapes in the scene description.

#ifndef NVIDIA_INDEX_ISHADING_MODEL_H
#define NVIDIA_INDEX_ISHADING_MODEL_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute

/// The interface class representing a shading model.
///
/// A shading model defines the normal interpolation for shading that results in
/// the illumination of a surface.  The shading model is considered in
/// combination with the material properties and the illumination model applied
/// to the surface. Only one shading model can be active at a time.  Derived
/// classes define the shading model to use when rendering a surface.
///
class IShading_model : public mi::base::Interface_declare<0x8f5bc60f, 0x654f, 0x46dd, 0x85, 0x76,
                         0xeb, 0x10, 0xbf, 0xab, 0x06, 0xb8, nv::index::IAttribute>
{
public:
  /// This mode specifies the orientation of front-facing polygons in, for example, triangle meshes.
  /// The
  /// order in which vertices are defined specifies if a polygon is front or back-facing. This
  /// ordering
  /// is important for backface culling and double-sided lighting to generate correct visual
  /// results.
  enum Front_face_vertex_order
  {
    FRONT_FACE_CCW = 0, ///< Counter-clockwise vertex-ordering in front faces.
    FRONT_FACE_CW = 1   ///< Clockwise order vertex-ordering in front faces.
  };

  /// Sets the vertex-ordering of front-facing polygons. By default counter-clockwise
  /// vertex ordering is used (FRONT_FACE_CCW).
  ///
  /// \param[in]  order           The new vertex-ordering used for front-facing polygons.
  ///
  virtual void set_front_face_vertex_order(Front_face_vertex_order order) = 0;

  /// Query the current vertex-ordering of front-facing polygons.
  ///
  /// \return                     Returns \c current vertex-ordering mode of front-facing polygons.
  ///
  virtual Front_face_vertex_order get_front_face_vertex_order() const = 0;

  /// By default, double-sided lighting is disabled but can be enabled using
  /// set_double_sided_lighting().
  ///
  /// \param[in]  double_sided    If \c true, then double-sided lighting will be enabled.
  ///
  virtual void set_double_sided_lighting(bool double_sided) = 0;

  /// Query if double-sided lighting is enabled.
  ///
  /// \return                     Returns \c true if the double-sided lighting is enabled.
  ///
  virtual bool get_double_sided_lighting() const = 0;

  /// By default, back-face culling is disabled but can be enabled using
  /// set_backface_culling().
  ///
  /// \param[in]  backface_cull   If \c true, then back-face culling will be enabled.
  ///
  virtual void set_backface_culling(bool backface_cull) = 0;

  /// Query if back-face culling is enabled.
  ///
  /// \return                     Returns \c true if back-face culling is enabled.
  ///
  virtual bool get_backface_culling() const = 0;
};

/// An interface class representing flat shading.
///
/// For flat shading, only the perpendicular normal of a triangle or polygon is
/// considered for lighting which defines the shading of the entire face.  As a
/// result all the face's vertices are colored using a single color, which
/// allows differentiation between adjacent faces.  Specular highlights are
/// particularly poor when using flat shading.  For example, a large specular
/// component determines the brightness rendered uniformly on an entire
/// face. Similarly, a specular highlight that is not considered at a shading
/// position will be ignored completely for shading the face.
///
/// The interface class serves as a flag considered in the scene description and
/// therefore no parameters need to be exposed.
///
class IFlat_shading : public mi::base::Interface_declare<0x9af9c55e, 0xd0d2, 0x4dee, 0x93, 0x3e,
                        0xa5, 0x5f, 0x96, 0x30, 0xe3, 0x16, nv::index::IShading_model>
{
};

/// An interface class representing Phong shading.
///
/// For Phong shading, the interpolated normal is considered at each shading
/// position for lighting which defines the shading of the face.  Phong shading
/// is used by default.
///
/// The interface class serves as a flag considered in the scene description
/// and, thus, no parameters need to be exposed.
///
class IPhong_shading : public mi::base::Interface_declare<0x65b99243, 0x4dd8, 0x41aa, 0xa4, 0x5d,
                         0x45, 0x0a, 0x79, 0xf5, 0xd7, 0xee, nv::index::IShading_model>
{
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ISHADING_MODEL_H
