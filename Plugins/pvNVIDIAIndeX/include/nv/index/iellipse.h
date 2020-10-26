/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene elements representing image space ellipses.

#ifndef NVIDIA_INDEX_IELLIPSE_SHAPES_H
#define NVIDIA_INDEX_IELLIPSE_SHAPES_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>
#include <nv/index/ifont.h>
#include <nv/index/ishape.h>

namespace nv
{
namespace index
{

/// NVIDIA IndeX provides scene elements that enable annotating 3D scenes or implementing 3D user-interface
/// widgets. The annotations and widgets should be composed as part of the scene description.
/// Such additional scene elements, which include for instance the following 2D ellipse, integrate seamlessly
/// into the regular 3D large-scale data rendering, i.e., the depth-correct rendering of all opaque of
/// semi-transparent elements is ensured by NVIDIA IndeX rendering.
/// A 2D ellipse represents a screen-space shape commonly used to annotating a scene or to build widgets.
/// The ellipse is defined in image space, i.e., an ellipse is parallel to the view plane and always faces
/// towards the viewer.
///
/// \ingroup nv_index_scene_description_image_shape
///
class IEllipse :
        public mi::base::Interface_declare<0x570a1c6e,0xfab3,0x41f6,0x9d,0x0d,0xfe,0x47,0x68,0x60,0x8f,0xb0,
                                           nv::index::IImage_space_shape>
{
public:
    /// An ellipse is defined by its center and two radii plus a rotation in screen space.
    /// The radii are always defined in screen space and given in pixels while the center
    /// of an ellipse is defined by a 3D coordinate. The rotation is a counter clock-wise
    /// rotation in screen space defined by an angle in radians.
    /// The following method sets the center of the ellipse as well as its
    /// screen space radii and rotation.
    ///
    /// \param[in] center           The center position of the ellipse defined in the
    ///                             ellipses local 3D space.
    ///
    /// \param[in] radius_x         The radius in pixels along ellipse's x (horizontal) axis.
    ///
    /// \param[in] radius_y         The radius in pixels along ellipse's y (vertical) axis.
    ///
    /// \param[in] rotation         The rotation angle in radians around it's
    ///                             center counter clock wise.
    ///
    virtual void set_geometry(
        const mi::math::Vector_struct<mi::Float32, 3>& center,
        mi::Float32                                    radius_x,
        mi::Float32                                    radius_y,
        mi::Float32                                    rotation = 0.f) = 0;

    /// An ellipse is defined by its center and two radii plus a rotation in screen space.
    /// The radii are always defined in screen space and given in pixels while the center
    /// of an ellipse is defined by a 3D coordinate. The rotation is a counter clock-wise
    /// rotation in screen space defined by an angle in radians.
    /// The following method gets the center of the ellipse as well as its
    /// screen space radii and rotation.
    ///
    /// \param[out] center          The center position of the ellipse defined in the
    ///                             ellipses local 3D space.
    ///
    /// \param[out] radius_x        The radius in pixels along ellipse's x (horizontal) axis.
    ///
    /// \param[out] radius_y        The radius in pixels along ellipse's y (vertical) axis.
    ///
    /// \param[out] rotation        The rotation angle in radians around it's
    ///                             center counter clock wise.
    ///
    virtual void get_geometry(
        mi::math::Vector_struct<mi::Float32, 3>&       center,
        mi::Float32&                                   radius_x,
        mi::Float32&                                   radius_y,
        mi::Float32&                                   rotation) const = 0;
        
    /// An ellipse's outline can have a style defined by the line color and a line width.
    /// The following method sets the line color and the width in screen space.
    ///
    /// \param[in] line_color       The color of the ellipse's outline.
    ///
    /// \param[in] line_width       The width of the ellipse's outline defined in pixels.
    ///
    virtual void set_outline_style(
        const mi::math::Color_struct&                  line_color,
        mi::Float32                                    line_width = 1.5f) = 0;
        
    /// An ellipse's outline can have a style defined by the line color and a line width.
    /// The following method gets the line color and the width in screen space.
    ///
    /// \param[out] line_color      The color of the ellipse's outline.
    ///
    /// \param[out] line_width      The width of the ellipse's outline defined in pixels.
    ///
    virtual void get_outline_style(
        mi::math::Color_struct&                        line_color,
        mi::Float32&                                   line_width) const = 0;

    /// The inner part of an ellipse is defined to be empty or can be filled. The fill style
    /// defines the appearance of the ellipse's interior.
    //
    enum Fill_style
    {
        FILL_EMPTY    = 0,      ///< No fill applied to the ellipse.
        FILL_SOLID    = 1,      ///< The ellipse will be filled using a solid color.
    };
        
        
    /// The inner part of a ellipse can be filled using a fill color or may be empty.
    /// The following method allows setting the fill style and sets the solid fill color.
    ///
    /// \param[in] fill_color       The color of the ellipse's fill color.
    ///
    /// \param[in] fill_style       The ellipse's fill style.
    ///
    virtual void set_fill_style(
        const mi::math::Color_struct&                  fill_color,
        Fill_style                                     fill_style) = 0;
        
    /// The inner part of a ellipse can be filled using a fill color or may be empty.
    /// The following method allows getting the fill style and sets the solid fill color.
    ///
    /// \param[out] fill_color      The color of the ellipse's fill color.
    ///
    /// \param[out] fill_style      The ellipse's fill style.
    ///
    virtual void get_fill_style(
        mi::math::Color_struct&                        fill_color,
        Fill_style&                                    fill_style) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IELLIPSE_SHAPES_H
