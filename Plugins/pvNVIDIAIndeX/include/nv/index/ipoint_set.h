/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element for point geometry.

#ifndef NVIDIA_INDEX_IPOINT_SET_H
#define NVIDIA_INDEX_IPOINT_SET_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description_shape
///
/// Interface class for a point set, which is a scene element and
/// can be added the scene description.
/// Each point has its own color and radius. 
///
/// Applications can derive from the interface class to implement user-defined 
/// point geometry that may have arbitrary (per-vertex) attributes that impact
/// the rendering attributes (such as 3D position, color and radius).
///
class IPoint_set :
        public mi::base::Interface_declare<0x7dfbeee1,0x4fda,0x4b8f,0xb8,0xce,0x4e,0xa1,0x92,0xa1,0xb6,0xb1,
                                           nv::index::IImage_space_shape>
{
public:
    /// The point style defines the appearance of a point when rendered using the
    /// rasterizer. 
    ///
    enum Point_style
    {
        FLAT_CIRCLE   = 0,      ///< A flat monochrome disc centered at the point's origin and facing the camera.
        SHADED_CIRCLE = 1,      ///< A 3D sphere like appearance centered at the origin including depth in accordance
                                ///< to the radius and the center and shading in accordance to light sources.
        FLAT_SQUARE   = 2,      ///< A flat monochrome square centered at the origin and facing the camera.
        FLAT_TRIANGLE = 3,      ///< A flat monochrome triangle centered at the origin and facing the camera.
        SHADED_FLAT_CIRCLE = 4  ///< A 3D sphere like appearance centered at the origin including shading
                                ///< in accordance to light sources.
    };

    /// Get number of 3D points or vertices.
    ///
    /// \return     The number of vertices.
    /// 
    virtual mi::Size get_nb_vertices() const = 0;

    /// Get the pointer to the array of vertices (points).
    ///
    /// \return     The pointer to the array of vertices.
    ///
    virtual const mi::math::Vector_struct<mi::Float32, 3>* get_vertices() const = 0;

    /// Set the pointer to the array of vertices (points).
    ///
    /// \param[in] vertices    The pointer to the array of vertices.
    /// \param[in] nb_vertices The number of vertices. The length of array. 
    ///
    virtual void set_vertices(
        mi::math::Vector_struct<mi::Float32, 3>* vertices,
        mi::Size                                 nb_vertices) = 0;

    /// Get number of color values.
    ///
    /// \return     The number of color values.
    /// 
    virtual mi::Size get_nb_colors() const = 0;

    /// Get the pointer to the array of per-vertex color values.
    ///
    /// \return     The pointer to the array of per-vertex color values.
    ///
    virtual const mi::math::Color_struct* get_colors() const = 0;

    /// Set the pointer to the array of per-vertex color values. The
    /// color of 3D point is multiplied by the associated
    /// material color.
    ///
    /// \param[in] colors    The pointer to the array of colors.
    /// \param[in] nb_colors The number of colors. The length of array. 
    ///
    virtual void set_colors(
        mi::math::Color_struct* colors,
        mi::Size                nb_colors) = 0;

    /// Get number of radii.
    ///
    /// \return     The number of radii.
    /// 
    virtual mi::Size get_nb_radii() const = 0;

    /// Get the pointer to the array of per-vertex radii.
    ///
    /// \return     The pointer to the array of per-vertex radii.
    ///
    virtual const mi::Float32* get_radii() const = 0;
    
    /// Set the pointer to the array of per-vertex radii.
    ///
    /// \param[in] radii    The pointer to the array of radii.
    /// \param[in] nb_radii The number of radii. The length of array. 
    ///
    virtual void set_radii(
        mi::Float32* radii,
        mi::Size     nb_radii) = 0;

    /// Get the point style.
    ///
    /// \return     The point style.
    ///
    virtual Point_style get_point_style() const = 0;

    /// Set the point style.
    ///
    /// \param[in] point_style The point style to be set.
    ///
    virtual void set_point_style(Point_style point_style) = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IPOINT_SET_H
