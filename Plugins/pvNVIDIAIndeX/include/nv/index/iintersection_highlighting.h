/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Highlight the intersection between two shapes.

#ifndef NVIDIA_INDEX_IINTERSECTION_HIGHLIGHTING_H
#define NVIDIA_INDEX_IINTERSECTION_HIGHLIGHTING_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// Enables the highlighting of an intersection with another shape on the
/// surface of the current shape. For example, when this attribute is applied to
/// an IPlane shape and the intersection shape is set to an
/// IRegular_heightfield, then the intersection between the plane and the
/// heightfield will be shown on the surface of the plane.
///
/// Multiple instances of this attribute can be active at the same time to show
/// the intersection with multiple shapes. The intersection will be drawn in the same
/// order the attributes are defined in the scene description, i.e. the highlight
/// for the attribute defined last will be rendered on top of the others.
///
/// \note Currently this attribute is only evaluated by IPlane. Only
/// IRegular_heightfield and ITriangle_mesh_scene_element are supported as
/// intersection shapes.
///
/// \ingroup nv_index_scene_description_attribute
///
class IIntersection_highlighting :
    public mi::base::Interface_declare<0x249cc579,0x0194,0x4399,0xb1,0x91,0x79,0xd1,0xe4,0x2d,0x09,0x9e,
                                       nv::index::IAttribute>
{
public:
    /// Sets the intersecting scene element.
    /// \param[in] shape_tag Tag of an IScene_element.
    ///
    virtual void set_intersection_shape(mi::neuraylib::Tag_struct shape_tag) = 0;

    /// Returns the intersecting scene element.
    /// \return Tag of an IScene_element.
    ///
    virtual mi::neuraylib::Tag_struct get_intersection_shape() const = 0;

    /// Sets the color used for highlighting the intersection.
    /// \param[in] color RGB color, alpha component is ignored.
    ///
    virtual void set_color(const mi::math::Color_struct& color) = 0;

    /// Returns the color used for highlighting the intersection.
    /// \return RGB color.
    ///
    virtual const mi::math::Color_struct& get_color() const = 0;

    /// Sets the width used for highlighting the intersection.
    ///
    /// To accommodate for the global scene transformation, the width is defined
    /// relative to the object-space z-axis. For example, with a global scaling
    /// of [2 2 3] a width value of 1 will correspond to 3 in world space,
    /// independent of the direction. This ensures that the highlights are not
    /// distorted by the scene transformation and that width values can be
    /// chosen independent of the scene transformation.
    ///
    /// \note Choosing a large width value that results in high screen coverage
    /// can lead to performance degradation.
    ///
    /// \param[in] width Width relative to the object-space z-axis.
    ///
    virtual void set_width(mi::Float32 width) = 0;

    /// Returns the width used for highlighting the intersection.
    ///
    /// \return Width relative to the object-space z-axis.
    ///
    virtual mi::Float32 get_width() const = 0;

    /// Controls how much the intersection should be smoothed.
    ///
    /// \param[in] smoothness Amount of smoothing applied: 0.0 means no
    ///                       smoothing, 1.0 means maximum smoothing.
    ///
    virtual void set_smoothness(mi::Float32 smoothness) = 0;

    /// Returns how much the intersection should be smoothed.
    ///
    /// \return Amount of smoothing applied.
    ///
    virtual mi::Float32 get_smoothness() const = 0;

    /// Controls how discontinuities in intersecting heightfields should be
    /// handled. The discontinuity limit defines how large the height value
    /// difference between adjacent cells must be so that it is considered as a
    /// discontinuity. The connection between the cells will not be highlighted
    /// in that case.
    ///
    /// The limit is specified relative to the highlighting width controlled by
    /// set_width(): If the adjacent height values differ by more than the
    /// highlighting width multiplied by the limit, then a discontinuity is
    /// assumed.
    ///
    /// \note This setting only affects intersections with IRegular_heightfield
    /// elements.
    ///
    /// \note The discontinuity detection only considers height values. For
    /// intersection planes that are mostly co-planar to the heightfield it
    /// should be disabled.
    ///
    /// \param[in] discontinuity_limit Discontinuity limit relative to the
    /// highlighting width, typically 1.0. A value of 0.0 disables the
    /// discontinuity detection, which is the default.
    ///
    virtual void set_discontinuity_limit(mi::Float32 discontinuity_limit) = 0;

    /// Returns how discontinuities in intersecting heightfields should be
    /// handled.
    ///
    /// \return Discontinuity limit relative to the highlighting width. A value
    /// of 0.0 means that the discontinuity detection is disabled.
    ///
    virtual mi::Float32 get_discontinuity_limit() const = 0;
};

}} // namespace

#endif // NVIDIA_INDEX_IINTERSECTION_HIGHLIGHTING_H
