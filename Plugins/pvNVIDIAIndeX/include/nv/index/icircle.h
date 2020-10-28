/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene elements representing image space circles.

#ifndef NVIDIA_INDEX_ICIRCLE_SHAPES_H
#define NVIDIA_INDEX_ICIRCLE_SHAPES_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>
#include <nv/index/iattribute.h>
#include <nv/index/ifont.h>

namespace nv
{
namespace index
{

/// NVIDIA IndeX provides scene elements that enable annotating 3D scenes or implementing 3D user-interface
/// widgets. The annotations and widgets should be composed as part of the scene description.
/// Such additional scene elements, which include for instance the following 2D circle, integrate seamlessly
/// into the regular 3D large-scale data rendering, i.e., the depth-correct rendering of all opaque of
/// semi-transparent elements is ensured by NVIDIA IndeX rendering.
/// A 2D circle represents a screen-space shape commonly used to annotating a scene or to build widgets.
/// The circle is defined in image space, i.e., a circle is parallel to the view plane and always faces 
/// towards the viewer.
///
/// \ingroup nv_index_scene_description_object_shape
///
class ICircle :
        public mi::base::Interface_declare<0xf5617675,0x23cc,0x4f82,0xbc,0xf7,0x24,0x52,0x7e,0xd6,0xf7,0xe5,
                                           nv::index::IImage_space_shape>
{
public:
    /// A circle is defined by its center and a radius. The radius is always defined in
    /// screen space and given in pixels while the center of a circle is defined by a 3D
    /// coordinate. The following method sets the center of the circle as well as its
    /// screen space radius.
    ///
    /// \param[in] center           The center of the circle defined in the
    ///                             circles local 3D space.
    ///
    /// \param[in] radius           The radius in screen space defined in pixels.
    ///
    virtual void set_geometry(
        const mi::math::Vector_struct<mi::Float32, 3>& center,
        mi::Float32                                    radius) = 0;

    /// A circle is defined by its center and a radius. The radius is always defined in
    /// screen space and given in pixels while the center of a circle is defined by a 3D
    /// coordinate. The following method gets the center of the circle as well as its
    /// screen space radius by reference.
    ///
    /// \param[out] center          The center of the circle defined in the circles
    ///                             local 3D space.
    ///
    /// \param[out] radius          The radius in screen space defined in pixels.
    ///
    virtual void get_geometry(
        mi::math::Vector_struct<mi::Float32, 3>&       center,
        mi::Float32&                                   radius) const = 0;
        
    /// A circle's outline can have a style defined by the line color and a line width.
    /// The following method sets the line color and the width in screen space.
    ///
    /// \param[in] line_color       The color of the circle's outline.
    ///
    /// \param[in] line_width       The width of the circle's outline defined in pixels.
    ///
    virtual void set_outline_style(
        const mi::math::Color_struct&                  line_color,
        mi::Float32                                    line_width = 1.5f) = 0;
        
    /// A circle's outline can have a style defined by the line color and a line width.
    /// The following method gets the line color and the width in screen space.
    ///
    /// \param[out] line_color      The color of the circle's outline.
    ///
    /// \param[out] line_width      The width of the circle's outline defined in pixels.
    ///
    virtual void get_outline_style(
        mi::math::Color_struct&                        line_color,
        mi::Float32&                                   line_width) const = 0;

    /// The inner part of a circle is defined to be empty or can be filled. The fill style
    /// defines the appearance of the circle's interior.
    //
    enum Fill_style
    {
        FILL_EMPTY    = 0,      ///< No fill applied to the circle.
        FILL_SOLID    = 1,      ///< The circle will be filled using a solid color.
    };
        
        
    /// The inner part of a circle can be filled using a fill color or may be empty.
    /// The following method allows setting the fill style and sets the solid fill color.
    ///
    /// \param[in] fill_color       The color of the circle's fill color
    ///
    /// \param[in] fill_style       The circle's fill style.
    ///
    virtual void set_fill_style(
        const mi::math::Color_struct&                  fill_color,
        Fill_style                                     fill_style) = 0;
        
    /// The inner part of a circle can be filled using a fill color or may be empty.
    /// The following method allows getting the fill style and sets the solid fill color.
    ///
    /// \param[out] fill_color      The color of the circle's fill color.
    ///
    /// \param[out] fill_style      The circle's fill style.
    ///
    virtual void get_fill_style(
        mi::math::Color_struct&                        fill_color,
        Fill_style&                                    fill_style) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ICIRCLE_SHAPES_H
