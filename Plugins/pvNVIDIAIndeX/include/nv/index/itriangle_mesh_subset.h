/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Sub-meshes of a triangle mesh.

#ifndef NVIDIA_INDEX_ITRIANGLE_MESH_SUBSET_H
#define NVIDIA_INDEX_ITRIANGLE_MESH_SUBSET_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>
#include <mi/math/vector.h>

#include <nv/index/idistributed_data_subset.h>

namespace nv
{

namespace index
{

/// Defines the vertices and per-vertex attributes of a subset of a triangle mesh.
/// @ingroup nv_index_data_storage
class ITriangle_mesh_subset : public mi::base::Interface_declare<0x19fa1bbf, 0xcda6, 0x43d5, 0x89,
                                0x40, 0x57, 0x95, 0x78, 0x99, 0xb5, 0x3d, IDistributed_data_subset>
{
public:
  /// Triangle flags.
  ///
  /// Specifies edge visibility for a given triangle with vertices (p0, p1, p2) and
  /// edges (e01, e12, e20). This is used by get_edge_flags() when an
  /// IWireframe_rendering_style attribute is used with the mesh.
  enum Triflags
  {
    TRIFLAGS_VISIBLE_EDGE_NONE = 0,
    TRIFLAGS_VISIBLE_EDGE_01 = 1,
    TRIFLAGS_VISIBLE_EDGE_12 = 2,
    TRIFLAGS_VISIBLE_EDGE_20 = 4,
    TRIFLAGS_VISIBLE_EDGE_ALL = 7
  };

  /// Initializes the triangle mesh subset by assigning vertices and
  /// per-vertex attributes.
  ///
  /// When an array is provided (e.g. \c normals or \c texture_coordinates), but its
  /// index array (\c normal_indices, \c tex_coord_indices) is 0, then the vertex
  /// index array (\c vertex_indices) will be used instead. This way, you can share the
  /// attribute index map.
  ///
  /// The arrays that map a triangle index to an attribute index are all
  /// separate.
  ///
  /// \param[in] bounding_box           bounding box of the sub-mesh (required)
  ///
  /// \param[in] vertices               vertex position array (required)
  /// \param[in] nb_vertices            vertex position array size (required)
  /// \param[in] vertex_indices         tri-vertex to vertex index array (required)
  /// \param[in] nb_indices             index array size (required)
  /// \param[in] global_triangle_ids    global triangle ID array (required)
  /// \param[in] nb_global_triangle_ids global triangle ID array size (required)
  ///
  /// \param[in] normals                vertex normal array (optional)
  /// \param[in] nb_normals             vertex normal array size (optional)
  /// \param[in] texture_coordinates    vertex texture coordinate array (optional)
  /// \param[in] nb_texture_coordinates vertex texture coordinate array size (optional)
  /// \param[in] colors                 vertex color value array (optional)
  /// \param[in] nb_colors              vertex color value array size (optional)
  ///
  /// \param[in] normal_indices         tri-vertex to normal index array (optional)
  /// \param[in] tex_coord_indices      tri-vertex to texture coordinate index array (optional)
  /// \param[in] color_indices          tri-vertex to color value index array (optional)
  /// \param[in] colormap_indices       tri-vertex to colormap index array (optional)
  ///
  /// \param[in] materials              triangle material array (optional)
  /// \param[in] nb_materials           triangle material array size (optional)
  /// \param[in] triangle_flags         triangle flag array (optional)
  /// \param[in] nb_triangle_flags      triangle flag array size (optional)
  ///
  /// \return True when initialization succeeded.
  ///
  virtual bool initialize(const mi::math::Bbox_struct<mi::Float32, 3>& bounding_box,

    const mi::math::Vector_struct<mi::Float32, 3>* vertices, mi::Uint32 nb_vertices,

    const mi::Uint32* vertex_indices, mi::Uint32 nb_indices,

    const mi::Uint64* global_triangle_ids, mi::Uint64 nb_global_triangle_ids,

    const mi::math::Vector_struct<mi::Float32, 3>* normals = 0, mi::Uint32 nb_normals = 0,

    const mi::math::Vector_struct<mi::Float32, 2>* texture_coordinates = 0,
    mi::Uint32 nb_texture_coordinates = 0,

    const mi::math::Color_struct* colors = 0, mi::Uint32 nb_colors = 0,

    const mi::Uint32* normal_indices = 0, const mi::Uint32* tex_coord_indices = 0,
    const mi::Uint32* color_indices = 0, const mi::Uint32* colormap_indices = 0,

    const mi::Uint16* materials = 0, mi::Uint32 nb_materials = 0,

    const ITriangle_mesh_subset::Triflags* triangle_flags = 0,
    mi::Uint32 nb_triangle_flags = 0) = 0;

  /// Returns the number of triangles in the sub-mesh.
  /// \return Number of triangles
  virtual mi::Uint32 get_nb_triangles() const = 0;

  /// Returns the number of vertices.
  /// \return Length of the vertex position array
  virtual mi::Uint32 get_nb_vertices() const = 0;

  /// Returns the vertex position array.
  /// \return Vertex position array
  virtual const mi::math::Vector_struct<mi::Float32, 3>* get_vertices() const = 0;

  /// Returns the tri-vertex to vertex position index array.
  /// The array length is number of triangles * 3.
  /// \return Tri-vertex to vertex position index array
  virtual const mi::Uint32* get_vertex_indices() const = 0;

  /// Returns the number of vertex normals.
  /// \return Length of the vertex normal array
  virtual mi::Uint32 get_nb_normals() const = 0;

  /// Returns the vertex normals array.
  /// \return Vertex normal array
  virtual const mi::math::Vector_struct<mi::Float32, 3>* get_normals() const = 0;

  /// Returns the tri-vertex to normal index array.
  /// The array length is number of triangles * 3.
  /// \return Tri-vertex to normal index array
  virtual const mi::Uint32* get_normal_indices() const = 0;

  /// Returns the number of texture coordinates.
  /// \return Length of the texture coordinate array
  virtual mi::Uint32 get_nb_texture_coordinates() const = 0;

  /// Returns the vertex texture coordinate array.
  /// \return Vertex texture coordinate array
  virtual const mi::math::Vector_struct<mi::Float32, 2>* get_texture_coordinates() const = 0;

  /// Returns the tri-vertex to texture coordinate index array.
  /// The array length is number of triangles * 3.
  /// \return Tri-vertex to texture coordinate index array
  virtual const mi::Uint32* get_texture_coordinate_indices() const = 0;

  /// Returns the number of colors.
  /// \return Length of the vertex color array
  virtual mi::Uint32 get_nb_colors() const = 0;

  /// Returns the vertex color array.
  /// \return Vertex color array
  virtual const mi::math::Color_struct* get_colors() const = 0;

  /// Returns the tri-vertex to color index.
  /// The array length is number of triangles * 3.
  /// \return Tri-vertex to color index array
  virtual const mi::Uint32* get_color_indices() const = 0;

  /// Returns the tri-vertex to colormap index array.
  /// The array length is number of triangles * 3.
  /// \return Tri-vertex to colormap index array, or 0
  virtual const mi::Uint32* get_colormap_indices() const = 0;

  /// Returns the number of materials
  /// \return Length of the material array
  virtual mi::Uint32 get_nb_materials() const = 0;

  /// Returns the triangle materials.
  /// \return Material array
  virtual const mi::Uint16* get_materials() const = 0;

  /// Returns the number of triangle flags.
  /// \return Length of the triangle flag array
  virtual mi::Uint32 get_nb_triangle_flags() const = 0;

  /// Returns the triangle flags.
  /// \return Triangle flag array
  virtual const ITriangle_mesh_subset::Triflags* get_triangle_flags() const = 0;

  /// Returns the IDs of the triangles in the sub-mesh.
  /// The array length is number of triangles.
  /// \return Triangle ID array
  virtual const mi::Uint64* get_global_triangle_ids() const = 0;

  /// Returns the bounding box of the sub-mesh.
  ///
  /// \return Bounding box in the local coordinate system.
  ///
  virtual const mi::math::Bbox_struct<mi::Float32, 3>& get_bounding_box() const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ITRIANGLE_MESH_SUBSET_H
