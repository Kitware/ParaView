/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element representing a path.

#ifndef NVIDIA_INDEX_IPATH_SHAPE_H
#define NVIDIA_INDEX_IPATH_SHAPE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>
#include <nv/index/ishape.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute

/// Interface class representing the styles that can be applied to a path.
/// Currently, the style merely defines the interpolation technique.
/// Only one style can be active at a time.
///
class IPath_style : public mi::base::Interface_declare<0xb4568002, 0xc4f5, 0x4e32, 0xb2, 0xf6, 0x5e,
                      0x04, 0x12, 0x45, 0x2d, 0xac, nv::index::IAttribute>
{
public:
  /// caps style
  enum Cap_style
  {
    /// No caps.
    CAP_STYLE_NONE = 0,
    /// flat caps (default).
    CAP_STYLE_FLAT = 1,
    // round caps.
    CAP_STYLE_ROUND = 2
  };

  /// Get cap style. Look for 'Cap_style' enum for details.
  ///
  /// \return     The cap style chosen for
  ///             subsequent paths in the scene description.
  ///
  virtual Cap_style get_cap_style() const = 0;

  /// Set the cap style. Look for 'Cap_style' enum for details.
  ///
  /// \param[in] cap_style    The cap style to be used by
  ///                         subsequent paths in the scene description.
  ///
  virtual void set_cap_style(Cap_style cap_style) = 0;

  /// Available interpolation techniques.
  enum Interpolation
  {
    INTERPOLATION_SEGMENT = 0, // A single solid color per segment (default).
    INTERPOLATION_NEAREST = 1, // A solid color to nearest value.
    INTERPOLATION_LINEAR = 2   // Linear color interpolation between points.
  };

  /// Get the interpolation technique. Look for 'interpolation' enum for details.
  ///
  /// \return     The interpolation technique chosen for
  ///             subsequent paths in the scene description.
  ///
  virtual Interpolation get_interpolation() const = 0;

  /// Set the interpolation technique.Look for 'interpolation' enum for details.
  ///
  /// \param[in] technique    The interpolation technique to be used by
  ///                         subsequent paths in the scene description.
  ///
  virtual void set_interpolation(Interpolation technique) = 0;

  /// Available color sources.
  enum Color_source
  {
    COLOR_SOURCE_NONE = 0,          // disable colormap and rgba array sources.
    COLOR_SOURCE_COLORMAP_ONLY = 1, // It uses colormap only. rgba array is disabled.
    COLOR_SOURCE_RGBA_ONLY = 2,     // It uses rgba array only. Colormap is disabled.
    COLOR_SOURCE_BOTH = 3,          // It uses both color sources or any defined (default).
                                    // When both defined it mixes the colors from both sources.
  };

  /// Get the color source. Look for 'Color_source' enum for details.
  ///
  /// \return     The current color source used by the points of the path
  ///
  virtual Color_source get_color_source() const = 0;

  /// Set the color source. Look for 'Color_source' enum for details.
  ///
  /// \param[in] source       The color source to be used by the path
  ///
  virtual void set_color_source(Color_source source) = 0;

  /// Enable/Disable the upsampling of the path. Increase the number of points by fitting a
  /// cubic spline over the original path producing a smoother path.
  ///
  /// \param[in] enable       Enable/disable the upsampling (default = disabled)
  /// \param[in] up_factor    The increase in the sampling rate. up_scaling >= 2 (default=2)
  /// \param[in] tension      The tension of the fitting curve. 0 < tension < 1.0
  ///                         tension = 0.0 (lowest, default); 1.0 (highest)
  ///
  virtual void set_upsampling(bool enable, mi::Uint32 up_factor = 2, mi::Float32 tension = 0.f) = 0;

  /// Get the current upsampling state
  ///
  /// \param[out] enable       The upsampling enable/disable state
  /// \param[out] up_factor    The increase in the sampling rate. up_scaling >= 2.
  /// \param[out] tension      The tension of the fitting curve. 0.0 < tension < 1.0
  ///
  virtual void get_upsampling(bool& enable, mi::Uint32& up_factor, mi::Float32& tension) const = 0;
};

/// @ingroup nv_index_scene_description_shape
///
/// Interface class for 3D paths.
/// The path is represented by a collection of 3D points connected by an unique and
/// continuous line path. Every point contains data represented as colors.
///
/// \note This shape type still supports the per-object supersampling, but the
/// full screen antialiasing take precedence, see IConfig_settings::set_rendering_samples().
///
class IPath_3D : public mi::base::Interface_declare<0xe284adf5, 0x6beb, 0x4792, 0x84, 0x36, 0x95,
                   0xc7, 0x78, 0xb9, 0xff, 0x8a, nv::index::IObject_space_shape>
{
public:
  /// Get the radius of the path
  ///
  /// \return     The radius of the internal representation for the path (line).
  ///
  ///
  virtual mi::Float32 get_radius() const = 0;

  /// set the radius of the path
  ///
  /// \param[in] radius   The radius of the internal representation for the path (line).
  ///
  ///
  virtual void set_radius(mi::Float32 radius) = 0;

  /// Get the number of path points.
  ///
  /// \return The number of path points.
  ///
  virtual mi::Uint32 get_nb_points() const = 0;

  /// Get the pointer to the array of path points.
  ///
  /// \return     The pointer to the array of path points.
  ///
  virtual const mi::math::Vector_struct<mi::Float32, 3>* get_points() const = 0;

  /// Set the pointer to the array of points.
  /// This method is expected to be called before set_color/set_color_maps_indexs/set_material_ids
  ///
  /// \param[in] points    The pointer to the array of points.
  /// \param[in] nb_points The number of vertices. The length of array.
  ///
  virtual void set_points(
    mi::math::Vector_struct<mi::Float32, 3>* points, mi::Uint32 nb_points) = 0;

  /// Get the pointer to the array of radii per point.
  ///
  /// \return     The pointer to the array of radii per point.
  ///
  virtual const mi::Float32* get_radii() const = 0;

  /// Set the pointer to the array of radii per point.
  ///
  /// \param[in] radii    The pointer to the array of radii.
  /// \param[in] nb_radii The number of radii. The length of array.
  ///                     When nb_radii doesn't match the number of points 'nb_points', the
  ///                     following is used:
  ///                     nb_radii < nb_points: The missing radii use the last radius
  ///                     nb_radii > nb_points: The extra radii are ignored
  ///
  virtual void set_radii(mi::Float32* radii, mi::Uint32 nb_radii) = 0;

  /// Get the pointer to the array of colors per segment/point.
  ///
  /// \return     The pointer to the array of colors per segment/point.
  ///
  virtual const mi::math::Color_struct* get_colors() const = 0;

  /// Set the pointer to the array of color per segment/point.
  ///
  /// \param[in] colors    The pointer to the array of colors.
  /// \param[in] nb_colors The number of colors. The length of array
  ///                      When nb_colors doesn't match the number of point 'nb_points', the
  ///                      following is used:
  ///                      nb_colors < nb_points: The missing colors use the last color
  ///                      nb_colors > nb_points: The extra colors are ignored
  ///
  virtual void set_colors(mi::math::Color_struct* colors, mi::Uint32 nb_colors) = 0;

  /// Get the pointer to the array of color map indexes per segment/point.
  ///
  /// \return              The pointer to the array of color map indexes per segment/point.
  ///
  virtual const mi::Uint32* get_color_map_indexes() const = 0;

  /// Set the pointer to the array of colormap indexes per segment/point.
  ///
  /// \param[in] indexes    The pointer to the array of color map indexes.
  /// \param[in] nb_indexes The number of indexes. The length of array
  ///                       When nb_indexes doesn't match the number of point 'nb_points', the
  ///                       following is used:
  ///                       nb_indexes < nb_points: The missing indexes use the last index
  ///                       nb_indexes > nb_points: The extra indexes are ignored
  ///
  virtual void set_color_map_indexes(mi::Uint32* indexes, mi::Uint32 nb_indexes) = 0;

  /// Get a pointer to the list of material ids. Zero based integer values with index to the list
  /// of materials defined in the scene description for the same group as this path
  ///
  /// \return The pointer to the list of material ids.
  ///
  virtual const mi::Uint32* get_material_ids() const = 0;

  /// Set a pointer to the list of material ids. Zero based integer values with index to the list
  /// of materials defined in the scene description for the same group as this path
  ///
  /// \param[in] ids       The pointer to the list of material ids.
  /// \param[in] nb_ids    The number of ids. The length of array.
  ///                      nb_ids is expected to be the same value as nb_points.
  ///                      When they are not the same a warning message is raised:
  ///                      nb_ids < nb_points: The missing ids use the last id
  ///                      nb_ids > nb_points: The extra ids are ignored
  ///
  virtual void set_material_ids(mi::Uint32* ids, mi::Uint32 nb_ids) = 0;
};

/// @ingroup nv_index_scene_description_shape
///
/// Interface class for 2D paths.
/// The path is represented by a collection of 3D points connected by an unique and
/// continuous line path. Every point contains data represented as colors.
class IPath_2D : public mi::base::Interface_declare<0xae7cff77, 0x114b, 0x472e, 0x9b, 0x77, 0x1b,
                   0xde, 0x7d, 0x98, 0xff, 0x7d, nv::index::IImage_space_shape>
{
public:
  /// Get the radius of the path
  ///
  /// \return     The radius (in pixel units) of the internal representation for the path (line).
  ///
  ///
  virtual mi::Float32 get_radius() const = 0;

  /// set the radius (in pixels units) of the path
  ///
  /// \param[in] radius   The radius of the internal representation for the path (line).
  ///
  ///
  virtual void set_radius(mi::Float32 radius) = 0;

  /// Get the number of path points.
  ///
  /// \return The number of path points.
  ///
  virtual mi::Uint32 get_nb_points() const = 0;

  /// Get the pointer to the array of path points.
  ///
  /// \return     The pointer to the array of path points.
  ///
  virtual const mi::math::Vector_struct<mi::Float32, 3>* get_points() const = 0;

  /// Set the pointer to the array of points.
  /// This method is expected to be called before set_color/set_color_maps_indexs/set_material_ids
  ///
  /// \param[in] points    The pointer to the array of points.
  /// \param[in] nb_points The number of vertices. The length of array.
  ///
  virtual void set_points(
    mi::math::Vector_struct<mi::Float32, 3>* points, mi::Uint32 nb_points) = 0;

  /// Get the pointer to the array of radii per point.
  ///
  /// \return     The pointer to the array of radii per point.
  ///
  virtual const mi::Float32* get_radii() const = 0;

  /// Set the pointer to the array of radii per point.
  ///
  /// \param[in] radii    The pointer to the array of radii.
  /// \param[in] nb_radii The number of radii. The length of array.
  ///                     When nb_radii doesn't match the number of point 'nb_points', the following
  ///                     is used:
  ///                     nb_radii < nb_points: The missing radii use the last radius
  ///                     nb_radii > nb_points: The extra radii are ignored
  ///
  virtual void set_radii(mi::Float32* radii, mi::Uint32 nb_radii) = 0;

  /// Get the pointer to the array of colors per segment/point.
  ///
  /// \return     The pointer to the array of colors per segment/point.
  ///
  virtual const mi::math::Color_struct* get_colors() const = 0;

  /// Set the pointer to the array of color per segment/point.
  ///
  /// \param[in] colors    The pointer to the array of colors.
  /// \param[in] nb_colors The number of colors. The length of array
  ///                      When nb_colors doesn't match the number of point 'nb_points', the
  ///                      following is used:
  ///                      nb_colors < nb_points: The missing colors use the last color
  ///                      nb_colors > nb_points: The extra colors are ignored
  ///
  virtual void set_colors(mi::math::Color_struct* colors, mi::Uint32 nb_colors) = 0;

  /// Get the pointer to the array of color map indexes per segment/point.
  ///
  /// \return              The pointer to the array of color map indexes per segment/point.
  ///
  virtual const mi::Uint32* get_color_map_indexes() const = 0;

  /// Set the pointer to the array of colormap indexes per segment/point.
  ///
  /// \param[in] indexes    The pointer to the array of color map indexes.
  /// \param[in] nb_indexes The number of indexes. The length of array
  ///                       When nb_indexes doesn't match the number of point 'nb_points', the
  ///                       following is used:
  ///                       nb_indexes < nb_points: The missing indexes use the last index
  ///                       nb_indexes > nb_points: The extra indexes are ignored
  ///
  virtual void set_color_map_indexes(mi::Uint32* indexes, mi::Uint32 nb_indexes) = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IPATH_SHAPE_H
