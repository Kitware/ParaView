/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Triangle mesh specific query results returned by the NVIDIA IndeX library when querying a scene's contents.

#ifndef NVIDIA_INDEX_ITRIANGLE_MESH_QUERY_RESULTS_H
#define NVIDIA_INDEX_ITRIANGLE_MESH_QUERY_RESULTS_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_query_results.h>

namespace nv
{
namespace index
{

/// Interface class that returns the triangle mesh specific result of a pick operation/query.
/// The interface class sub classes from \c IScene_pick_result to provide additional 
/// intersection results specific to \c ITriangle_mesh_scene_element.
///
/// \ingroup nv_scene_queries
///
class ITriangle_mesh_pick_result :
    public mi::base::Interface_declare<0xe2697b96,0xdd49,0x4506,0x81,0x57,0x40,0x31,0x96,0x43,0x0e,0x2f,
                                        nv::index::IScene_pick_result>
{
public:
    /// Returns triangle index of the hit triangle. The triangle index is defined globally in the triangle mesh
    /// regardless of the machine where the triangle of the triangle mesh is stored and rendered.
    /// The global triangle index allows querying the triangle's specific details using NVIDIA IndeX's lookup
    /// query, which then returns an instance of the interface class \c ITriangle_mesh_lookup_result.
    ///
    /// \return         Returns the global triangle index.
    ///
    virtual mi::Uint64 get_triangle_index() const = 0;
    
    /// Returns the local triangle index; the local index refers to the triangle
    /// order on the machine where the data is stored.
    ///
    /// \return         Returns the local triangle index.
    ///
    virtual mi::Uint32 get_local_triangle_index() const = 0;

    /// Returns the averaged normal at the pick position.
    ///
    /// \return         Returns the averaged normal.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 3> get_normal() const = 0;

    /// Returns the average per-vertex color at the pick location
    /// or transparent black if not per-vertex color values have been set.
    ///
    /// \return         Returns the averaged per-vertex color.
    ///
    virtual mi::math::Color_struct get_color_value() const = 0;

    /// Returns the average colormap color value at the pick location
    /// or transparent black if not colormap nor colormap indices
    /// have been set.
    ///
    /// \return         Returns the average colormap color.
    ///
    virtual mi::math::Color_struct get_colormap_value() const = 0;

    /// Returns the barycentric coordinate at the pick location. These could be used to 
    /// average per-vertex attributes stored at the triangle's vertices.
    ///
    /// \return         Returns the barycentric coordinates.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 3> get_barycentric_coordinates() const = 0;
};


/// Interface class that returns the triangle mesh specific result of a query operation.
/// The interface class sub classes from \c IScene_lookup_result to provide additional
/// results related to an entry of a \c ITriangle_mesh_scene_element elements.
/// Each entry or lookup index refers to a single triangle of the triangle mesh and the per-vertex
/// attributes stored along with the triangle, such as the per-vertex colors. The lookup result
/// provides the per-vertex values in counter-clockwise order.
///
/// \ingroup nv_scene_queries
///
class ITriangle_mesh_lookup_result :
    public mi::base::Interface_declare<0xd91ca5a9,0x8222,0x42ec,0xab,0x44,0x65,0xd1,0x72,0xad,0x0b,0x71,
                                        nv::index::IScene_lookup_result>
{
public:
    /// Each triangle of a triangle mesh is defined by 3 vertices with their locations defined in the triangle mesh's 3D space.
    /// The vertices are defined in counter-clockwise order.
    ///
    /// \param[out] v0      Returns the first vertex position.
    /// \param[out] v1      Returns the second vertex position.
    /// \param[out] v2      Returns the third  vertex position.
    ///
    virtual void get_vertices(
        mi::math::Vector_struct<mi::Float32, 3>& v0,
        mi::math::Vector_struct<mi::Float32, 3>& v1,
        mi::math::Vector_struct<mi::Float32, 3>& v2) const = 0;

    /// A normal can be defined for each vertex of the triangle of a triangle.
    /// The call provides the per-vertex normals.
    ///
    /// \param[out] n0      Returns the first per-vertex normal.
    /// \param[out] n1      Returns the second per-vertex normal.
    /// \param[out] n2      Returns the third per-vertex normal.
    ///
    virtual void get_normals(
        mi::math::Vector_struct<mi::Float32, 3>& n0,
        mi::math::Vector_struct<mi::Float32, 3>& n1,
        mi::math::Vector_struct<mi::Float32, 3>& n2) const = 0;

    /// A color index can be defined for each vertex of the triangle of a triangle.
    /// The call provides the per-vertex color index values.
    ///
    /// \param[out] c0      Returns the first per-vertex color index.
    /// \param[out] c1      Returns the second per-vertex color index.
    /// \param[out] c2      Returns the third per-vertex color index.
    ///
    virtual void get_color_indices(
        mi::Uint32& c0,
        mi::Uint32& c1,
        mi::Uint32& c2) const = 0;

    /// A color can be defined for each vertex of the triangle of a triangle.
    /// The call provides the per-vertex colors.
    ///
    /// \param[out] c0      Returns the first per-vertex color value.
    /// \param[out] c1      Returns the second per-vertex color value.
    /// \param[out] c2      Returns the third per-vertex color value.
    ///
    virtual void get_colors(
        mi::math::Color_struct& c0,
        mi::math::Color_struct& c1,
        mi::math::Color_struct& c2) const = 0;

    /// A texture coordinate can be defined for each vertex of the triangle of a triangle.
    /// The call provides the per-vertex texture coordinate.
    ///
    /// \param[out] st0     Returns the first per-vertex texture coordinate.
    /// \param[out] st1     Returns the second per-vertex texture coordinate.
    /// \param[out] st2     Returns the third per-vertex texture coordinate.
    ///
    virtual void get_texture_coordinates(
        mi::math::Vector_struct<mi::Float32, 2>& st0,
        mi::math::Vector_struct<mi::Float32, 2>& st1,
        mi::math::Vector_struct<mi::Float32, 2>& st2) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ITRIANGLE_MESH_QUERY_RESULTS_H
