/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element for line geometry.

#ifndef NVIDIA_INDEX_ILINE_SET_H
#define NVIDIA_INDEX_ILINE_SET_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description_shape
///
/// Interface class for line geometry, which is a scene element and
/// can be added to the scene description.
/// A line set is a set of line segments. Each segment has its own color
/// and width/radius.
///
/// Applications can derive from the interface class to implement user-defined
/// line geometry that may have arbitrary (per-segment) attributes that impact
/// the rendering attributes (such as 3D position, color and width/radius).
///
class ILine_set : public mi::base::Interface_declare<0xdd11bb4e, 0x46ff, 0x405b, 0xbe, 0x0f, 0xa4,
                    0x16, 0x4f, 0x10, 0xbd, 0x94, nv::index::IImage_space_shape>
{
public:
  /// Available line types for the line rendering
  enum Line_type
  {
    /// separated line segments
    LINE_TYPE_SEGMENTS = 0,
    /// one connected line path
    LINE_TYPE_PATH = 1,
    /// one connected and closed line path
    LINE_TYPE_LOOP = 2
  };

  /// Available line styles for the line rendering
  enum Line_style
  {
    ///
    LINE_STYLE_SOLID = 0,
    ///
    LINE_STYLE_DASHED = 1,
    ///
    LINE_STYLE_DOTTED = 2,
    ///
    LINE_STYLE_CENTER = 3,
    ///
    LINE_STYLE_HIDDEN = 4,
    ///
    LINE_STYLE_PHANTOM = 5,
    ///
    LINE_STYLE_DASHDOT = 6,
    ///
    LINE_STYLE_BORDER = 7,
    ///
    LINE_STYLE_DIVIDE = 8
  };

  /// Available cap styles for the line rendering
  enum Cap_style
  {
    ///
    CAP_STYLE_FLAT = 0,
    ///
    CAP_STYLE_SQUARE = 1,
    ///
    CAP_STYLE_ROUND = 2
  };

  /// Get the line type. look Line_style enum for details.
  ///
  /// \return     The line type.
  ///
  virtual Line_type get_line_type() const = 0;

  /// Set the line type. look Line_style enum for details.
  ///
  /// \param[in] type  The line type.
  ///
  virtual void set_line_type(Line_type type) = 0;

  /// Get the line style. The line styles can be solid or a dashed or dotted style.
  ///
  /// \return     The line style used by stylized lines.
  ///
  virtual Line_style get_line_style() const = 0;

  /// Set the line style. The line styles can be solid or a dashed or dotted style.
  ///
  /// \param[in] style The line style used by stylized lines.
  ///
  virtual void set_line_style(Line_style style) = 0;

  /// Get the line cap style.
  ///
  /// \return     The cap style used by stylized lines.
  ///
  virtual Cap_style get_cap_style() const = 0;

  /// Set the line cap style.
  ///
  /// \param[in] style The line cap style used by stylized lines.
  ///
  virtual void set_cap_style(Cap_style style) = 0;

  /// Get number of line segments.
  ///
  /// \deprecated
  /// \return The number of line.
  ///
  virtual mi::Size get_nb_lines() const = 0;

  /// Get number of line vertices. This is the size of the line vertices buffer.
  ///
  /// \return The number of vertices of the line.
  ///
  virtual mi::Size get_nb_vertices() const = 0;

  /// Get the pointer to the array of line vertices (points). The number of vertices
  /// depend on the line type:
  ///
  /// if N = number of line segments, then
  ///
  /// number of vertices =
  ///     - 2 * N ( LINE_TYPE_SEGMENTS )
  ///     - N + 1 ( LINE_TYPE_PATH )
  ///     - N     ( LINE_TYPE_LOOP )
  ///
  /// \return     The pointer to the array of line vertices.
  ///
  virtual const mi::math::Vector_struct<mi::Float32, 3>* get_lines() const = 0;

  /// Set the line vertices (points). The number of vertices depend on the line type:
  ///
  /// if N = number of line segments, then
  ///
  /// number of vertices =
  ///     - 2 * N ( LINE_TYPE_SEGMENTS )
  ///     - N + 1 ( LINE_TYPE_PATH )
  ///     - N     ( LINE_TYPE_LOOP )
  ///
  /// \param[in] line_vertices    The pointer to the array of line vertices.
  /// \param[in] nb_line_vertices The number of line vertices.
  virtual void set_lines(
    mi::math::Vector_struct<mi::Float32, 3>* line_vertices, mi::Size nb_line_vertices) = 0;

  /// Get the pointer to the array of colors per segment/vertex.
  /// It depends on the line type:
  ///
  /// if N = number of line segments, then
  ///
  /// number of colors =
  ///     - N     ( LINE_TYPE_SEGMENTS, color per line segment )
  ///     - N + 1 ( LINE_TYPE_PATH, color per vertex )
  ///     - N     ( LINE_TYPE_LOOP, color per vertex )
  ///
  /// \return     The pointer to the array of colors per segment/vertex.
  ///
  virtual const mi::math::Color_struct* get_colors() const = 0;

  /// Get number of color values.
  ///
  /// \return     The number of color values.
  ///
  virtual mi::Size get_nb_colors() const = 0;

  /// Set the pointer to the array of colors per segment/vertex.
  /// It depends on the line type:
  ///
  /// if N = number of line segments, then
  ///
  /// number of colors =
  ///     - N     ( LINE_TYPE_SEGMENTS, color per line segment )
  ///     - N + 1 ( LINE_TYPE_PATH, color per vertex )
  ///     - N     ( LINE_TYPE_LOOP, color per vertex )
  ///
  /// \param[in] colors    The pointer to the array of colors per segment/vertex.
  /// \param[in] nb_colors The number of colors. The length of array.
  ///
  virtual void set_colors(mi::math::Color_struct* colors, mi::Size nb_colors) = 0;

  /// Get the pointer to the array of widths per segment/vertex.
  /// It depends on the line type:
  ///
  /// if N = number of line segments, then
  ///
  /// number of widths =
  ///     - N     ( LINE_TYPE_SEGMENTS, width per line segment )
  ///     - N + 1 ( LINE_TYPE_PATH, width per vertex )
  ///     - N     ( LINE_TYPE_LOOP, width per vertex )
  ///
  /// \return     The pointer to the array of widths per segment/vertex.
  ///
  virtual const mi::Float32* get_widths() const = 0;

  /// Get number of line segment width.
  ///
  /// \return     The number of line segment width.
  ///
  virtual mi::Size get_nb_widths() const = 0;

  /// Set the pointer to the array of widths per segment/vertex.
  /// It depends on the line type:
  ///
  /// if N = number of line segments, then
  ///
  /// number of widths =
  ///     - N     ( LINE_TYPE_SEGMENTS, width per line segment )
  ///     - N + 1 ( LINE_TYPE_PATH, width per vertex )
  ///     - N     ( LINE_TYPE_LOOP, width per vertex )
  ///
  /// \param[in] widths    The pointer to the array of widths.
  /// \param[in] nb_widths The number of widths. The length of array.
  ///
  virtual void set_widths(mi::Float32* widths, mi::Size nb_widths) = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ILINE_SET_H
