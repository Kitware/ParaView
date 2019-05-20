/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Class representing wireframe rendering for geometries in the scene description.

#ifndef NVIDIA_INDEX_IWIREFRAME_RENDERING_STYLE_H
#define NVIDIA_INDEX_IWIREFRAME_RENDERING_STYLE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute

/// The interface class representing an attribute to render triangle based
/// geometries in wireframe style.  The class provides two different styles:
/// <em>outline</em> and <em>wireframe</em>.
///
/// In outline style, the visible edges of the geometry will be rendered as a
/// thin outline on top of the geometry with a given width and color.
///
/// In wireframe style, only the visible edges will be rendered as a thin
/// wireframe model, skipping completely the triangles of the geometry
/// model. The wireframe will use a given width and color.
///
/// By default, all edges of the geometry are visible. However, edge visibility
/// can be specified per triangle for geometric types that support triangle flag
/// data. (See, for example, the ITriangle_mesh class for details.)
///
class IWireframe_rendering_style
  : public mi::base::Interface_declare<0x3150e667, 0xf850, 0x4bfc, 0x84, 0xdd, 0x98, 0xa6, 0x1d,
      0x78, 0x28, 0x10, nv::index::IAttribute>
{
public:
  /// Wireframe rendering styles
  enum Wireframe_style
  {
    /// outlines rendered on top of the geometry
    WIREFRAME_STYLE_OUTLINE = 1,
    /// pure wireframe rendering (triangles are not rendered)
    WIREFRAME_STYLE_WIREFRAME = 2
  };

  /// Set the wireframe style to be used.
  ///
  /// \param[in]  style           The wireframe style.
  ///
  virtual void set_wireframe_style(Wireframe_style style) = 0;

  /// Get the current wireframe style.
  ///
  /// \return                     Returns the wireframe style. Default is WIREFRAME_STYLE_OUTLINE.
  ///
  virtual Wireframe_style get_wireframe_style() const = 0;

  /// Set the outline/wireframe width to be used, specified in object units.
  ///
  /// \param[in]  width           The outline/wireframe width.
  ///
  virtual void set_wireframe_width(mi::Float32 width) = 0;

  /// Get the current outline/wireframe width, specified in object units.
  ///
  /// \return                     Returns the outline/wireframe width. Default is 1.0.
  ///
  virtual mi::Float32 get_wireframe_width() const = 0;

  /// Set the outline/wireframe color to be used.
  ///
  /// \param[in]  color           The outline/wireframe color.
  ///
  virtual void set_wireframe_color(const mi::math::Color_struct& color) = 0;

  /// Get the current outline/wireframe color.
  ///
  /// \return                     Returns the outline/wireframe color. Default is (0, 0, 0, 1).
  ///
  virtual mi::math::Color_struct get_wireframe_color() const = 0;
};

/// The interface class representing the wireframe style specific to
/// heightfields.
///
/// Two different styles of outlining techniques are supported:
/// <em>triangles</em> and <em>patches</em>.
///
/// In <em>triangle outline style</EM>, the edges of all triangles that define
/// the heightfield surface are highlighted using the width and color of the
/// super class.
///
/// In <em>patch outline style</em>, the edges of all patches that are defined
/// by four consecutive elevation values are highlighted using the width and
/// color of the super class.
///
/// By default, the edges of all triangle are highlighted.
///
class IHeightfield_wireframe_style
  : public mi::base::Interface_declare<0x9837b2d4, 0x8347, 0x4e56, 0x97, 0x42, 0x0a, 0xfc, 0x9f,
      0x54, 0xae, 0x45, nv::index::IWireframe_rendering_style>
{
public:
  /// Wireframe topologies
  enum Wireframe_topology
  {
    /// wireframe of the triangles that setup the heightfield's surface
    WIREFRAME_TOPOLOGY_TRIANGLES = 1,
    /// wireframe of the quads that setup the heightfield's surface (similar
    /// to HF_WIREFRAME_STYLE_TRIANGLES but does not include diagonal edges)
    WIREFRAME_TOPOLOGY_QUADS = 2
  };

  /// Set the wireframe topology to be used.
  ///
  /// \param[in]  topology           The wireframe topology.
  ///
  virtual void set_wireframe_topology(Wireframe_topology topology) = 0;

  /// Get the current wireframe topology.
  ///
  /// \return                     Returns the wireframe topology. The default is
  /// WIREFRAME_TOPOLOGY_QUADS.
  ///
  virtual Wireframe_topology get_wireframe_topology() const = 0;

  /// Set the wireframe resolution. N times the resolution of the heightfield mesh resolution
  ///
  /// \param[in]  resolution           The wireframe resolution.
  ///
  virtual void set_wireframe_resolution(mi::Uint32 resolution) = 0;

  /// Get the current wireframe resolution. N times the resolution of the heightfield mesh
  /// resolution
  ///
  /// \return                     Returns the wireframe resolution. The default is 1.
  ///
  virtual mi::Uint32 get_wireframe_resolution() const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IWIREFRAME_RENDERING_STYLE_H
