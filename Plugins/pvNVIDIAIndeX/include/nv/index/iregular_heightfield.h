/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for regular heightfield scene elements

#ifndef NVIDIA_INDEX_REGULAR_HEIGHTFIELD_H
#define NVIDIA_INDEX_REGULAR_HEIGHTFIELD_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>
#include <mi/math/color.h>

#include <nv/index/iattribute.h>
#include <nv/index/idistributed_data.h>

namespace nv
{

namespace index
{

/// Scene attribute to control the rendering of embedded heightfield geometry.
/// The embedded geometry is closely associated with the heightfield, but rendered separately. This
/// attribute allows to control which of the different types of embedded geometry should be shown
/// and how they should be visualized.
/// @ingroup nv_index_scene_description_attribute
///
class IHeightfield_geometry_settings
  : public mi::base::Interface_declare<0xc1529d11, 0x1fa2, 0x47b0, 0x95, 0xa2, 0x76, 0x1f, 0x44,
      0xc5, 0xaf, 0x1c, nv::index::IAttribute>
{
public:
  /// Types of geometry to which this attribute should apply
  enum Geometry_type
  {
    /// Attribute will be ignored
    TYPE_NONE = 0,
    /// Isolated points
    TYPE_ISOLATED_POINTS = 1,
    /// Connecting lines
    TYPE_CONNECTING_LINES = 2,
    /// User-defined points
    TYPE_SEED_POINTS = 4,
    /// User-defined lines
    TYPE_SEED_LINES = 8,
    /// Apply to all geometry types
    TYPE_ALL = 0xFF,
  };

  /// Which colors should be mapped onto the geometry
  enum Color_mode
  {
    /// Use the fixed color specified by \c set_color()
    MODE_FIXED = 0,
    /// Apply the heightfield material and multiply the result with the fixed color
    MODE_MATERIAL = 1,
    /// Sample the active volume at the position of each geometry object and multiply the result
    /// with the fixed color
    MODE_VOLUME_TEXTURE = 2,
    /// Sample the active computed heightfield texture at the position of each geometry object
    /// and multiply the result with the fixed color
    MODE_COMPUTED_TEXTURE = 3,
  };

  /// How the geometry should be rendered.
  enum Render_mode
  {
    /// Use the default rendering mode of each geometry type.
    RENDER_DEFAULT = 0,
    /// Render points as discs and lines as quads, with the normal aligned to the z-axis. This
    /// is the default for \c TYPE_ISOLATED_POINTS and \c TYPE_CONNECTING_LINES.
    RENDER_Z_AXIS_ALIGNED = 1,
    /// Render points as discs and lines as quads, with the normal aligned to to screen, i.e.
    /// the inverse viewing direction.
    RENDER_SCREEN_ALIGNED = 2,
    /// Render rasterized points and lines. This is the default for \c TYPE_SEED_POINTS and \c
    /// TYPE_SEED_LINES.
    RENDER_RASTERIZED = 3,
    /// Do not render the geometry. It can still be shown indirectly by \c
    /// IIntersection_highlighting.
    RENDER_NONE = 4,
  };

  /// Controls which types of embedded geometry this attribute should apply to.
  ///
  /// \param type_mask Bit mask of values from \c Geometry_type.
  ///
  /// Example: To select both isolated points and connecting lines use
  /// \code set_type_mask(TYPE_ISOLATED_POINTS | TYPE_CONNECTING_LINES); \endcode
  virtual void set_type_mask(mi::Uint32 type_mask) = 0;

  /// Returns which types of embedded geometry this attribute should apply to.
  /// \return Bit mask of \c Geometry_type.
  virtual mi::Uint32 get_type_mask() const = 0;

  /// Controls whether the selected types of geometry should be shown.
  /// \param visible If true, the selected types will be rendered.
  virtual void set_visible(bool visible) = 0;

  /// Controls whether the selected types of geometry should be shown.
  /// \return Visibility
  virtual bool get_visible() const = 0;

  /// Sets the fixed color used for rendering
  /// \param[in] color The color is used directly with \c MODE_FIXED, and modulated to the result
  ///                  for other color modes.
  virtual void set_color(const mi::math::Color_struct& color) = 0;

  /// Returns the fixed color used for rendering
  /// \return Fixed color
  virtual const mi::math::Color_struct& get_color() const = 0;

  /// Sets the color mode for embedded geometry.
  /// \param color_mode Mode with which the selected geometry types should be rendered.
  virtual void set_color_mode(Color_mode color_mode) = 0;

  /// Returns the color mode for embedded geometry.
  /// \return Color mode
  virtual Color_mode get_color_mode() const = 0;

  /// Sets the rendering mode for embedded geometry.
  ///
  /// \param render_mode Mode with which the selected geometry types should be rendered.
  virtual void set_render_mode(Render_mode render_mode) = 0;

  /// Returns the rendering mode for embedded geometry.
  /// \return Render mode
  virtual Render_mode get_render_mode() const = 0;

  /// Controls the size of the geometry elements.
  ///
  /// \note In mode \c RENDER_RASTERIZED the radius and line width used for rendering is fixed.
  ///       This setting will still be used by the picking, creating an invisible border around
  ///       the geometry where picks will be detected.
  ///
  /// \param size Radius or line width, depending on geometry type. Should be less than 1.0 to
  ///             prevent overlap between adjacent geometry.
  ///             For \c RENDER_RASTERIZED a value of 2.0 typically gives good picking results.
  ///
  virtual void set_geometry_size(mi::Float32 size) = 0;

  /// Returns the size of the geometry elements.
  /// \return Radius or line width, depending on geometry type.
  virtual mi::Float32 get_geometry_size() const = 0;

  /// Controls how geometry far away from the camera should be faded out. When the feature is
  /// enabled, embedded geometry with a distance to the camera origin larger than the <em>end
  /// distance</em> will not be rendered. The opacity of geometry closer than the end distance but
  /// further away than the <em>start distance</em> will be linearly interpolated from fully
  /// opaque to fully transparent (using a ramp function).
  ///
  /// \param distances start and end distance of the ramp function. To disable (the default), set
  ///                  both values to 0.0.
  virtual void set_opacity_ramp_distances(
    const mi::math::Vector_struct<mi::Float32, 2>& distances) = 0;

  /// Returns the parameters of how geometry far away from the camera should be faded out.
  ///
  /// \return Start and end distances of the ramp function, or 0.0 for both if disabled.
  virtual const mi::math::Vector_struct<mi::Float32, 2>& get_opacity_ramp_distances() const = 0;

  /// Controls the offset that is added to the geometry in the \c RENDER_Z_AXIS_ALIGNED mode, to
  /// prevent z-fighting with the heightfield.
  /// \param offset distance in object space
  virtual void set_render_offset(mi::Float32 offset) = 0;

  /// Returns the offset between the geometry and the heightfield used in the \c
  /// RENDER_Z_AXIS_ALIGNED mode.
  /// \return Distance in object space
  virtual mi::Float32 get_render_offset() const = 0;
};

/// The abstract interface class representing a heightfield scene element.
///
/// A heightfield is a scene element as part of the scene description, i.e.,
/// in \c IScene. These heightfield scene elements are an abstract representation of the
/// 'physical' heightfield data uploaded to the cluster. They contain general information
/// about the heightfield, but do not store the patch data, which is handled by \c
/// IRegular_heightfield_patch_data.
///
/// Each heightfield scene element has a <em>region of interest</em> which defines the part of the
/// heightfield which should be used for rendering. It is defined as a bounding box in local 3D
/// space by calling \c set_IJK_region_of_interest(). When no region of interest is set, the entire
/// heightfield will be considered. The region of interest is used by \c
/// get_XYZ_clipped_bounding_box()
/// to clip the scene element bounding box before transforming it from local space to global
/// space. In contrast, \c get_XYZ_bounding_box() returns the unclipped scene element bounding box.
///
/// The rendering of a heightfield can be controlled using an \c IMaterial attribute in the
/// scene description. Arbitrary textures can be mapped onto the heightfield by
/// applying \c IDistributed_compute_technique.
/// @ingroup nv_index_scene_description_shape
///
class IRegular_heightfield
  : public mi::base::Interface_declare<0x7a8db268, 0x1c5b, 0x4579, 0xa4, 0x42, 0x0d, 0x5f, 0xb5,
      0xaa, 0x9d, 0x5b, nv::index::IDistributed_data>
{
public:
  /// Sets the user-defined name of the scene element.
  ///
  /// This can be useful to distinguish several scene elements of the same type.
  /// However, setting a name is optional.
  ///
  /// \param[in] name User-defined name. This should not be 0.
  virtual void set_name(const char* name) = 0;

  /// Returns the user-defined name of the scene element.
  /// \return User-defined name. An empty string, if no name has been set.
  virtual const char* get_name() const = 0;

  /// Returns the transformation matrix of the scene element.
  ///
  /// \return Transformation matrix from IJK (local) to XYZ (global) space
  virtual mi::math::Matrix_struct<mi::Float32, 4, 4> get_transform() const = 0;

  /// Returns the tag of the import strategy that is used for
  /// loading the dataset.
  ///
  /// \return tag of the IDistributed_data_import_strategy
  virtual mi::neuraylib::Tag_struct get_import_strategy() const = 0;

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Bounding box and region of interest
  ///@{

  /// Sets the IJK (local) bounding box of the entire (unclipped) heightfield.
  /// \param[in] bbox New bounding box of the heightfield.
  virtual void set_IJK_bounding_box(const mi::math::Bbox_struct<mi::Float32, 3>& bbox) = 0;

  /// Returns the IJK (local) bounding box of the entire (unclipped) heightfield.
  ///
  /// \return IJK bounding box
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_IJK_bounding_box() const = 0;

  /// Sets the region of interest of the heightfield in IJK (local) coordinates.
  ///
  /// \param[in] ijk_roi_bbox region of interest as a bounding box
  virtual void set_IJK_region_of_interest(
    const mi::math::Bbox_struct<mi::Float32, 3>& ijk_roi_bbox) = 0;

  /// Returns the region of interest of the heightfield in IJK (local) coordinates.
  ///
  /// \return IJK region of interest as a bounding box
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_IJK_region_of_interest() const = 0;

  /// Returns the XYZ (global) bounding box clipped against the region of interest.
  /// It is calculated by first clipping and then transforming the IJK bounding box to XYZ
  /// according to the current transformation matrix of the scene element.
  ///
  /// \return XYZ bounding box
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_XYZ_clipped_bounding_box() const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Colormap settings
  ///@{

  /// Sets volume color mapping mode for the heightfield.
  /// \deprecated Will be replaced by scene description attributes.
  /// \param[in] enable true when enable mapping.
  virtual void set_colormap_mapping(bool enable) = 0;

  /// Returns the volume color mapping mode for the heightfield.
  /// \deprecated Will be replaced by scene description attributes.
  /// \return volume color mapping mode
  virtual bool get_colormap_mapping() const = 0;

  /// Sets the colormap for the heightfield volume color mapping.
  /// \deprecated Will be replaced by scene description attributes.
  /// \param[in] tag  Tag of the colormap.
  virtual void assign_colormap(mi::neuraylib::Tag_struct tag) = 0;

  /// Returns the colormap for the heightfield volume color mapping.
  /// \deprecated Will be replaced by scene description attributes.
  /// \return     Tag of the colormap.
  virtual mi::neuraylib::Tag_struct assigned_colormap() const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Extra heightfield geometry
  ///@{

  /// Sets coordinates of point geometry to be rendered together with this heightfield as seed
  /// points. An internal copy of the point data is created and the caller is responsible for
  /// freeing the passed array.
  ///
  /// \note The points will be rendered using the color returned by get_color(), any material
  /// attributes
  /// are ignored.
  ///
  /// \todo Will be changed to work in IJK space.
  ///
  /// \param[in] points     Array of vertices (in XYZ space)
  /// \param[in] nb_points  Number of vertices in the array
  ///
  virtual void add_seed_points(
    mi::math::Vector_struct<mi::Float32, 3>* points, mi::Uint32 nb_points) = 0;

  /// Returns the number of seed points currently stored.
  ///
  /// \return                 Returns the number of seed points.
  ///
  virtual mi::Uint32 get_nb_seed_points() const = 0;

  /// Returns the seed point with the given index.
  ///
  /// \param[in] index        Index of the seed point, must be in
  ///                         [0, get_nb_seed_points()-1]
  /// \param[out] seed_point Vertex of the specified seed point
  ///
  virtual void get_seed_point(
    mi::Uint32 index, mi::math::Vector_struct<mi::Float32, 3>& seed_point) const = 0;

  /// Removes the seed point with the given index.
  ///
  /// \param[in] index        Index of the seed point, must be in
  ///                         [0, get_nb_seed_points()-1]
  ///
  virtual void remove_seed_point(mi::Uint32 index) = 0;

  /// Adds a isolated pick to the set of seed points.
  /// Calling this method is equivalent to \c add_seed_points() with a
  /// single XYZ coordinate as parameter.
  ///
  /// \param[in] ij_coordinate IJ coordinates of the point
  /// \param[in] elevation     elevation (= K) coordinate of the point
  ///
  virtual void add_isolated_pick(
    const mi::math::Vector_struct<mi::Float32, 2>& ij_coordinate, mi::Float32 elevation) = 0;

  /// Sets line geometry to be rendered together with this heightfield as
  /// seed lines. Each line is defined by two vertices, a start point and an
  /// end point. An internal copy of the vertex data is created and the caller
  /// is responsible for freeing the passed array.
  ///
  /// \note The lines will be rendered using the color returned by get_color(), any material
  /// attributes are ignored.
  ///
  /// \todo Will be changed to work in IJK space.
  ///
  /// \param[in] vertices     Array of vertices (in XYZ space).
  /// \param[in] nb_vertices  Number of vertices in the array.
  ///
  virtual void add_seed_line(
    mi::math::Vector_struct<mi::Float32, 3>* vertices, mi::Uint32 nb_vertices) = 0;

  /// Returns the number of seed lines currently stored.
  ///
  /// \return                 Returns the number of seed lines.
  ///
  virtual mi::Uint32 get_nb_seed_lines() const = 0;

  /// Each seed line is an array of vertices. The method exposes
  /// the array of vertices of a seed line with the given index.
  ///
  /// \param[in] index        Index of the seed point, must be in
  ///                         [0, get_nb_seed_lines()-1]
  /// \param[out] nb_vertices The number of vertices that define the
  ///                         seed line that corresponds to the index.
  ///
  /// \return                 Returns an array of the vertices that
  ///                         define the given seed line.
  ///
  virtual mi::math::Vector_struct<mi::Float32, 3>* get_seed_line(
    mi::Uint32 index, mi::Uint32& nb_vertices) const = 0;

  /// Removes the seed line with the given index.
  ///
  /// \param[in] index        Index of the seed point, must be in
  ///                         [0, get_nb_seed_lines()-1]
  ///
  virtual void remove_seed_line(mi::Uint32 index) = 0;

  ///@}
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_REGULAR_HEIGHTFIELD_H
