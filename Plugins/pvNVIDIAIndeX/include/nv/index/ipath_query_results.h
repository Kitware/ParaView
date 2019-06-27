/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Path specific query results returned by the NVIDIA IndeX library when querying a scene's contents, e.g., using the pick operation.

#ifndef NVIDIA_INDEX_IPATH_QUERY_RESULTS_H
#define NVIDIA_INDEX_IPATH_QUERY_RESULTS_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_query_results.h>

namespace nv
{
namespace index
{
/// @ingroup scene_queries

/// Interface class that represents the path-specific result of a pick operation/query.
/// The interface class sub classes from \c IScene_pick_result to provide additional 
/// intersection results specific to \c IPath_2d or \c IPath_3d scene elements.
/// In the future, the path specific pick result interface class may be extended 
/// by a derived interface class that returns 3D related pick information such as 
/// the normal or 1D/2D-texture coordinates at an intersection point.
///
class IPath_pick_result :
    public mi::base::Interface_declare<0x48da609d,0x1f76,0x4a53,0x91,0x06,0x92,0x26,0xdf,0x3e,0x86,0x7c,
                                        nv::index::IScene_pick_result>
{
public:
    /// Returns the index that specifies which of the path segment.
    /// Each path in constructed by segments defined two consecutive vertices. For instance, 
    /// the i-th segment is defined by the i-th and the (i+1)-th path vertices.
    /// 
    /// \return     Returns the index of the picked segment.
    ///
    virtual mi::Uint32 get_segment() const = 0;
};

/// Interface class that represents the path specific entry lookup result.
/// The interface class sub classes from \c IScene_lookup_result to provide additional 
/// results related to an entry of a \c IPath_2d or \c IPath_3d scene elements.
/// Each entry or lookup index refers to a single vertex of the path and the values stored along
/// with the vertex, such as the per-vertex color or the radius.
///
class IPath_lookup_result :
    public mi::base::Interface_declare<0x44666ae9,0x28b3,0x4f79,0xb5,0x63,0x2d,0x02,0xba,0xdc,0xf3,0x15,
                                        nv::index::IScene_lookup_result>
{
public:
    /// The set of vertices defines the path in 3D space. The per-vertex color values are exposed
    /// by the query.
    /// 
    /// \return         Returns the vertex's 3D position in the path's local space.
    ///
    virtual const mi::math::Vector_struct<mi::Float32, 3>& get_vertex() const = 0;

    /// The color values defined with the vertices defines a path's appearance.
    /// Each vertex has assigned a color that can be queried.
    /// 
    /// \return         Returns the per-vertex color.
    ///
    virtual const mi::math::Color_struct& get_color() const = 0;

    /// The color index values defined with the vertices are used to index color maps
    /// that determine a color which then defines a path's appearance. The per-vertex
    /// color index is exposed by the query.
    /// 
    /// \return         Returns the per-vertex color index.
    ///
    virtual mi::Uint32 get_color_index() const = 0;
    
    /// A radius defines the path's shape and is currently set per path.
    /// The radius is exposed by the query.
    /// 
    /// \return         Returns the radius defined for the path.
    ///
    virtual mi::Float32 get_radius() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IPATH_QUERY_RESULTS_H
