/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene elements representing object space and image space labels.

#ifndef NVIDIA_INDEX_IPOLYGON_H
#define NVIDIA_INDEX_IPOLYGON_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>
#include <nv/index/iattribute.h>
#include <nv/index/ifont.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description_shape
///
/// A polygon of n vertices defined in image space. 
/// The polygon always faces towards the viewer, i.e., the polygon is parallel to the view plane.
class IPolygon :
    public mi::base::Interface_declare<0x6558d81b,0xe2de,0x4349,0x92,0xcf,0x86,0xd2,0x27,0x1b,0x3b,0xfc,
                                       nv::index::IImage_space_shape>

{
public:
    /// set polygon geometry
    ///
    /// \param[in] verts            The 2D vertex list in counter-clockwise order
    ///                             in pixel coordinates respect to
    ///                             the polygon center in 2D.
    ///
    /// \param[in] nb_verts         The number of 2D vertices in the vertex list
    ///
    /// \param[in] center           The center position of the polygon in 3D
    ///
    /// \return value               true :  Valid polygon was created. A valid polygon
    ///                                     can be convex or concave representing
    ///                                     a single shape, without holes or self
    ///                                     crossing silhouette.
    ///                             false:  Polygon creating fail. Polygon is not
    ///                                     not well shaped. Not simple shape or has holes 
    ///                                     or is self intersecting.
    ///
    virtual bool set_geometry(
        const mi::math::Vector_struct<mi::Float32, 2>* verts,
        mi::Uint32                                     nb_verts,
        const mi::math::Vector_struct<mi::Float32, 3>& center) = 0;

    /// Get polygon geometry.
    ///
    /// \param[out] verts            The 2D vertex list in counter-clockwise order
    ///                              in pixel coordinates respect to
    ///                              the polygon center in 2D.
    ///
    /// \param[out] nb_verts         The number of 2D vertices in the vertex list
    ///
    /// \param[out] center           The center position of the polygon in 3D
    ///
    virtual void get_geometry(
        mi::math::Vector_struct<mi::Float32, 2>**      verts,
        mi::Uint32&                                    nb_verts,
        mi::math::Vector_struct<mi::Float32, 3>&       center) const = 0;
        
    /// Fill style defines the appearance of the polygon.
    //
    enum Fill_style
    {
        FILL_EMPTY    = 0,      ///< No fill
        FILL_SOLID    = 1,      ///< Flat color
    };
        
        
    /// Set polygon fill style.
    ///
    /// \param[in] fill_color       fill color
    /// \param[in] fill_style       fill style
    ///
    virtual void set_fill_style(
        const mi::math::Color_struct&                  fill_color,
        Fill_style                                     fill_style) = 0;
        
    /// Get polygon fill style.
    ///
    /// \param[out] fill_color      fill color
    /// \param[out] fill_style      fill style
    ///
    virtual void get_fill_style(
        mi::math::Color_struct&                        fill_color,
        Fill_style&                                    fill_style) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IPOLYGON_H
