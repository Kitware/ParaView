/* Copyright 2019 NVIDIA Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <string>
#include <typeinfo>
#include <unordered_map>

#include "vtkMultiProcessController.h"
#include "vtkTimerLog.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkUnstructuredGrid.h"

#include <nv/index/iirregular_volume_subset.h>

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_irregular_volume_importer.h"
#include "vtknvindex_utilities.h"

//-------------------------------------------------------------------------------------------------
vtknvindex_irregular_volume_importer::vtknvindex_irregular_volume_importer(
  const mi::Sint32& border_size, const std::string& scalar_type)
  : m_border_size(border_size)
  , m_scalar_type(scalar_type)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_irregular_volume_importer::vtknvindex_irregular_volume_importer()
  : m_border_size(2)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_irregular_volume_importer::~vtknvindex_irregular_volume_importer()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
mi::Size vtknvindex_irregular_volume_importer::estimate(
  const mi::math::Bbox_struct<mi::Float32, 3>& /*bounding_box*/,
  mi::neuraylib::IDice_transaction* /*dice_transaction*/) const
{
  return 0;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_importer::set_cluster_properties(
  vtknvindex_cluster_properties* cluster_properties)
{
  m_cluster_properties = cluster_properties;
}

//-------------------------------------------------------------------------------------------------
nv::index::IDistributed_data_subset* vtknvindex_irregular_volume_importer::create(
  const mi::math::Bbox_struct<mi::Float32, 3>& bounding_box,
  nv::index::IData_subset_factory* factory,
  mi::neuraylib::IDice_transaction* /*dice_transaction*/) const
{
  vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Importing");
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  mi::Sint32 rank_id = controller ? controller->GetLocalProcessId() : 0;

  const std::type_info* scalar_type_info =
    (m_scalar_type == "unsigned char") ? &typeid(mi::Uint8) : (m_scalar_type == "unsigned short")
      ? &typeid(mi::Uint16)
      : (m_scalar_type == "float") ? &typeid(mi::Float32) : &typeid(mi::Float64);

  // Fetch shared memory details from host properties.
  std::string shm_memory_name;
  mi::math::Bbox<mi::Float32, 3> shm_bbox;
  mi::Uint64 shm_size = 0;
  void* raw_mem_pointer = NULL;
  mi::Uint32 time_step = 0;

  mi::math::Bbox<mi::Float32, 3> query_bbox(bounding_box);
  const mi::math::Vector<mi::Float32, 3> center = (query_bbox.min + query_bbox.max) * 0.5f;
  query_bbox.min = query_bbox.max = center;

  if (!m_cluster_properties->get_host_properties(rank_id)->get_shminfo(
        query_bbox, shm_memory_name, shm_bbox, shm_size, &raw_mem_pointer, time_step))
  {
    ERROR_LOG << "Failed to get the shared memory information for the subregion: " << query_bbox
              << ".";
    return 0;
  }

  if (shm_memory_name.empty() || shm_bbox.empty())
  {
    ERROR_LOG << "Failed to open the shared memory: " << shm_memory_name
              << " with bbox: " << shm_bbox << ".";
    return 0;
  }

  vtkUnstructuredGridBase* pv_ugrid = nullptr;

  mi::Uint32 num_points = 0u;
  mi::Uint32 num_cells = 0u;
  mi::math::Vector<mi::Float32, 3>* points = NULL;
  mi::math::Vector<mi::Uint32, 4>* cells = NULL;
  void* scalars = NULL;
  mi::Uint8* shm_ivol = NULL;
  mi::Float32 max_edge_length_sqr = 0.f;

  INFO_LOG << "The bounding box requested by NVIDIA IndeX: " << bounding_box << ".";

  if (raw_mem_pointer) // The volume data is available in local memory.
  {
    vtknvindex_irregular_volume_data* ivol_data =
      static_cast<vtknvindex_irregular_volume_data*>(raw_mem_pointer);

    pv_ugrid = ivol_data->pv_unstructured_grid;
    scalars = ivol_data->scalars;
    max_edge_length_sqr = ivol_data->max_edge_length2;
  }
  else // The volume data is in shared memory.
  {
    INFO_LOG << "Using shared memory: " << shm_memory_name << " with bbox: " << shm_bbox << ".";

    shm_ivol = vtknvindex::util::get_vol_shm<mi::Uint8>(shm_memory_name, shm_size);

    mi::Uint8* shm_offset = shm_ivol;
    size_t size_elm;

    // num points
    size_elm = sizeof(num_points);
    num_points = *reinterpret_cast<mi::Uint32*>(shm_offset);
    shm_offset += size_elm;

    // num cells
    size_elm = sizeof(num_cells);
    num_cells = *reinterpret_cast<mi::Uint32*>(shm_offset);
    shm_offset += size_elm;

    // points
    size_elm = sizeof(mi::Float32) * 3 * num_points;
    points = reinterpret_cast<mi::math::Vector<mi::Float32, 3>*>(shm_offset);
    shm_offset += size_elm;

    // cells
    size_elm = sizeof(mi::Uint32) * 4 * num_cells;
    cells = reinterpret_cast<mi::math::Vector<mi::Uint32, 4>*>(shm_offset);
    shm_offset += size_elm;

    // scalars
    if (*scalar_type_info == typeid(mi::Uint8))
      size_elm = sizeof(mi::Uint8) * num_points;
    else if (*scalar_type_info == typeid(mi::Uint16))
      size_elm = sizeof(mi::Uint16) * num_points;
    else if (*scalar_type_info == typeid(mi::Float32))
      size_elm = sizeof(mi::Float32) * num_points;
    else // typeid(mi::Float64)
      size_elm = sizeof(mi::Float64) * num_points;

    scalars = reinterpret_cast<void*>(shm_offset);
    shm_offset += size_elm;

    // max square edge length
    max_edge_length_sqr = *reinterpret_cast<mi::Float32*>(shm_offset);
  }

  using mi::math::Vector;
  using mi::math::Vector_struct;

  typedef Vector<mi::Uint32, 4> Vec4ui;
  typedef Vector<mi::Float32, 3> Vec3f;

  // Read the tetrahedrons and collect only those that intersects the subset bounding box.
  mi::math::Bbox<mi::Float32, 3> subset_bbox(static_cast<mi::Float32>(bounding_box.min.x),
    static_cast<mi::Float32>(bounding_box.min.y), static_cast<mi::Float32>(bounding_box.min.z),
    static_cast<mi::Float32>(bounding_box.max.x), static_cast<mi::Float32>(bounding_box.max.y),
    static_cast<mi::Float32>(bounding_box.max.z));

  mi::Size nb_subset_vertices = 0u;
  mi::Size nb_subset_tetrahedrons = 0u;

  std::vector<Vec4ui> subset_tetrahedrons;
  std::vector<Vec3f> subset_vertices;
  std::vector<mi::Uint8> subset_scalars_uint8;
  std::vector<mi::Uint16> subset_scalars_uint16;
  std::vector<mi::Float32> subset_scalars_float32;
  std::unordered_map<mi::Uint32, mi::Uint32> global_to_local_vtx_idx_map;

  if (pv_ugrid != nullptr)
  {
    bool gave_error = false;
    vtkSmartPointer<vtkCellIterator> cellIter =
      vtkSmartPointer<vtkCellIterator>::Take(pv_ugrid->NewCellIterator());
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      vtkIdType npts = cellIter->GetNumberOfPoints();
      if (npts != 4)
      {
        if (!gave_error)
        {
          ERROR_LOG << "Encountered non-tetrahedral cell. NVIDIA IndeX's irregular volume "
                       "renderer supports tetrahedral cells only.";
          gave_error = true;
        }
        continue;
      }

      vtkIdType* cell_point_ids = cellIter->GetPointIds()->GetPointer(0);

      // check for degenerated cells
      bool invalid_cell = false;
      for (mi::Uint32 i = 0; i < 3; i++)
      {
        for (mi::Uint32 j = i + 1; j < 4; j++)
        {
          if (cell_point_ids[i] == cell_point_ids[j])
          {
            invalid_cell = true;
            break;
          }
        }

        if (invalid_cell)
          break;
      }

      if (invalid_cell)
        continue;

      mi::math::Bbox<mi::Float32, 3> tet_bbox;
      tet_bbox.clear();

      for (mi::Uint32 i = 0; i < 4; i++)
      {
        mi::Float64 point[3];
        pv_ugrid->GetPoint(cell_point_ids[i], point);
        tet_bbox.insert(mi::Float32_3(static_cast<mi::Float32>(point[0]),
          static_cast<mi::Float32>(point[1]), static_cast<mi::Float32>(point[2])));
      }

      if (tet_bbox.intersects(subset_bbox))
      {
        Vec4ui tet_vtx_indices(
          cell_point_ids[0], cell_point_ids[1], cell_point_ids[2], cell_point_ids[3]);
        subset_tetrahedrons.push_back(tet_vtx_indices);
      }
    }
  }
  else
  {
    for (mi::Uint32 t = 0u; t < num_cells; ++t)
    {
      const Vec4ui& tet_vtx_indices = cells[t];

      mi::math::Bbox<mi::Float32, 3> tet_bbox;
      tet_bbox.clear();

      for (mi::Uint32 k = 0; k < 4; k++)
        tet_bbox.insert(points[tet_vtx_indices[k]]);

      if (tet_bbox.intersects(subset_bbox))
        subset_tetrahedrons.push_back(tet_vtx_indices);
    }
  }

  // Build subset vertex list and remap indices.
  nb_subset_tetrahedrons = subset_tetrahedrons.size();

  // if no tetrahedrons in this subregion then return.
  if (nb_subset_tetrahedrons == 0)
  {
    // free memory space linked to shared memory
    if (shm_ivol)
      vtknvindex::util::unmap_shm(shm_ivol, shm_size);

    return NULL;
  }

  for (mi::Uint32 t = 0u; t < nb_subset_tetrahedrons; ++t)
  {
    Vec4ui& tet_vtx_indices = subset_tetrahedrons[t];
    for (mi::Uint32 j = 0; j < 4u; ++j)
    {
      mi::Uint32& vtx_index = tet_vtx_indices[j];

      std::unordered_map<mi::Uint32, mi::Uint32>::const_iterator kt =
        global_to_local_vtx_idx_map.find(vtx_index);
      if (kt != global_to_local_vtx_idx_map.end())
      {
        vtx_index = kt->second;
      }
      else
      {
        const mi::Uint32 new_vtx_idx = static_cast<mi::Uint32>(subset_vertices.size());
        global_to_local_vtx_idx_map[vtx_index] = new_vtx_idx;

        if (pv_ugrid)
        {
          double* cur_pv_vertice = pv_ugrid->GetPoint(vtx_index);
          Vec3f cur_vertice(static_cast<mi::Float32>(cur_pv_vertice[0]),
            static_cast<mi::Float32>(cur_pv_vertice[1]),
            static_cast<mi::Float32>(cur_pv_vertice[2]));

          subset_vertices.push_back(cur_vertice);
        }
        else
        {
          subset_vertices.push_back(points[vtx_index]);
        }

        if (*scalar_type_info == typeid(mi::Uint8))
          subset_scalars_uint8.push_back((reinterpret_cast<mi::Uint8*>(scalars))[vtx_index]);
        else if (*scalar_type_info == typeid(mi::Uint16))
          subset_scalars_uint16.push_back((reinterpret_cast<mi::Uint16*>(scalars))[vtx_index]);
        else if (*scalar_type_info == typeid(mi::Float32))
          subset_scalars_float32.push_back((reinterpret_cast<mi::Float32*>(scalars))[vtx_index]);
        else // typeid(mi::Float64)
          subset_scalars_float32.push_back(
            static_cast<mi::Float32>((reinterpret_cast<mi::Float64*>(scalars))[vtx_index]));

        vtx_index = new_vtx_idx;
      }
    }
  }

  // free memory space linked to shared memory
  if (shm_ivol)
    vtknvindex::util::unmap_shm(shm_ivol, shm_size);

  nb_subset_vertices = subset_vertices.size();
  mi::Size nb_cells = nb_subset_tetrahedrons;
  mi::Size nb_cell_face_indices = nb_cells * 4u;

  mi::Size nb_faces = nb_cell_face_indices;
  mi::Size nb_face_vtx_indices = nb_faces * 3u;

  nv::index::IIrregular_volume_subset::Mesh_parameters mesh_params;

  // General mesh geometry and topology info.
  mesh_params.nb_vertices = nb_subset_vertices;
  mesh_params.nb_face_vtx_indices = nb_face_vtx_indices;
  mesh_params.nb_faces = nb_faces;
  mesh_params.nb_cell_face_indices = nb_cell_face_indices;
  mesh_params.nb_cells = nb_cells;

  // Required mesh information for the renderer.
  mesh_params.global_max_edge_length = mi::math::sqrt(max_edge_length_sqr);

  mi::base::Handle<nv::index::IIrregular_volume_subset> irregular_volume_subset(
    factory->create_data_subset<nv::index::IIrregular_volume_subset>());

  if (!irregular_volume_subset.is_valid_interface())
  {
    ERROR_LOG << "The importer cannot create an irregular volume subset.";
    return NULL;
  }

  nv::index::IIrregular_volume_subset::Mesh_storage mesh_storage;
  if (!irregular_volume_subset->generate_mesh_storage(mesh_params, mesh_storage))
  {
    ERROR_LOG << "The importer is unable to generate an irregular volume mesh storage.";
    return NULL;
  }

  nv::index::IIrregular_volume_subset::Attribute_parameters attrib_params;
  attrib_params.affiliation = nv::index::IIrregular_volume_subset::ATTRIB_AFFIL_PER_VERTEX;
  attrib_params.nb_attrib_values = nb_subset_vertices;

  if (*scalar_type_info == typeid(mi::Uint8))
    attrib_params.type = nv::index::IIrregular_volume_subset::ATTRIB_TYPE_UINT8;
  else if (*scalar_type_info == typeid(mi::Uint16))
    attrib_params.type = nv::index::IIrregular_volume_subset::ATTRIB_TYPE_UINT16;
  else // typeid(mi::Float32)
    attrib_params.type = nv::index::IIrregular_volume_subset::ATTRIB_TYPE_FLOAT32;

  nv::index::IIrregular_volume_subset::Attribute_storage attribute_storage;
  if (!irregular_volume_subset->generate_attribute_storage(0u, attrib_params, attribute_storage))
  {
    ERROR_LOG << "The importer is unable to generate an irregular volume attribute storage.";
    return NULL;
  }

  if (*scalar_type_info == typeid(mi::Uint8))
  {
    attrib_params.type = nv::index::IIrregular_volume_subset::ATTRIB_TYPE_UINT8;
    mi::Uint8* subset_attrib_values = reinterpret_cast<mi::Uint8*>(attribute_storage.attrib_values);

    // Copy vertices and attributes.
    for (mi::Uint32 v = 0u; v < nb_subset_vertices; ++v)
    {
      mesh_storage.vertices[v] = subset_vertices[v];
      subset_attrib_values[v] = subset_scalars_uint8[v];
    }
  }
  else if (*scalar_type_info == typeid(mi::Uint16))
  {
    attrib_params.type = nv::index::IIrregular_volume_subset::ATTRIB_TYPE_UINT16;
    mi::Uint16* subset_attrib_values =
      reinterpret_cast<mi::Uint16*>(attribute_storage.attrib_values);

    // Copy vertices and attributes.
    for (mi::Uint32 v = 0u; v < nb_subset_vertices; ++v)
    {
      mesh_storage.vertices[v] = subset_vertices[v];
      subset_attrib_values[v] = subset_scalars_uint16[v];
    }
  }
  else // typeid(mi::Float32)
  {
    attrib_params.type = nv::index::IIrregular_volume_subset::ATTRIB_TYPE_FLOAT32;
    mi::Float32* subset_attrib_values =
      reinterpret_cast<mi::Float32*>(attribute_storage.attrib_values);

    // copy vertices and attributes
    for (mi::Uint32 v = 0u; v < nb_subset_vertices; ++v)
    {
      mesh_storage.vertices[v] = subset_vertices[v];
      subset_attrib_values[v] = subset_scalars_float32[v];
    }
  }

  mi::Uint32 next_vidx = 0u;
  mi::Uint32 next_fidx = 0u;

  // generate cells, cell's face indices, faces, face's vertex indices.
  for (mi::Size t = 0u; t < nb_subset_tetrahedrons; ++t)
  {
    const mi::Uint32 a = subset_tetrahedrons[t].x;
    const mi::Uint32 b = subset_tetrahedrons[t].y;
    const mi::Uint32 c = subset_tetrahedrons[t].z;
    const mi::Uint32 d = subset_tetrahedrons[t].w;

    const Vec3f& av = subset_vertices[a];
    const Vec3f& bv = subset_vertices[b];
    const Vec3f& cv = subset_vertices[c];
    const Vec3f& dv = subset_vertices[d];

    const Vec3f centroid = (av + bv + cv + dv) * 0.25f;

    // * adds a face to the ivol mesh storage
    // * tries to orient faces to have correct CCW vertex ordering
    auto ivol_add_tet_face = [&](mi::Uint32 i0, mi::Uint32 i1, mi::Uint32 i2, Vec3f const& p0,
      Vec3f const& p1, Vec3f const& p2) {
      const Vec3f e1 = (p1 - p0);
      const Vec3f e2 = (p2 - p0);

      // face plane
      Vec3f n = cross(e1, e2);
      n.normalize();
      const mi::Float32 dst = -(dot(n, p0));

      // tetrahedron centroid distance to face plane
      const mi::Float32 cd = dot(n, centroid) + dst;

      if (cd < 0.0f)
      {
        // correct ordering
        mesh_storage.face_vtx_indices[next_vidx + 0u] = i0;
        mesh_storage.face_vtx_indices[next_vidx + 1u] = i1;
        mesh_storage.face_vtx_indices[next_vidx + 2u] = i2;
      }
      else
      {
        // invert ordering
        mesh_storage.face_vtx_indices[next_vidx + 0u] = i0;
        mesh_storage.face_vtx_indices[next_vidx + 1u] = i2;
        mesh_storage.face_vtx_indices[next_vidx + 2u] = i1;
      }

      mesh_storage.faces[next_fidx].nb_vertices = 3u;
      mesh_storage.faces[next_fidx].start_vertex_index = next_vidx;

      next_fidx += 1;
      next_vidx += 3;
    };

    // cell's face indices
    // * this might seem redundant here, but consider datasets with shared faces among cells
    mesh_storage.cell_face_indices[next_fidx + 0] = next_fidx + 0;
    mesh_storage.cell_face_indices[next_fidx + 1] = next_fidx + 1;
    mesh_storage.cell_face_indices[next_fidx + 2] = next_fidx + 2;
    mesh_storage.cell_face_indices[next_fidx + 3] = next_fidx + 3;

    // fill cell data
    mesh_storage.cells[t].nb_faces = 4u;
    mesh_storage.cells[t].start_face_index = next_fidx;

    // Needs to happen down here as it increases next_fidx and next_vidx.
    ivol_add_tet_face(a, c, d, av, cv, dv);
    ivol_add_tet_face(a, b, c, av, bv, cv);
    ivol_add_tet_face(a, d, b, av, dv, bv);
    ivol_add_tet_face(b, d, c, bv, dv, cv);
  }

  irregular_volume_subset->retain();

  vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Importing");

  return irregular_volume_subset.get();
}

//-------------------------------------------------------------------------------------------------
mi::base::Uuid vtknvindex_irregular_volume_importer::subset_id() const
{
  return nv::index::IIrregular_volume_subset::IID();
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_importer::serialize(mi::neuraylib::ISerializer* serializer) const
{
  serializer->write(&m_border_size, 1);
  vtknvindex::util::serialize(serializer, m_scalar_type);

  m_cluster_properties->serialize(serializer);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_importer::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  deserializer->read(&m_border_size, 1);
  vtknvindex::util::deserialize(deserializer, m_scalar_type);

  m_cluster_properties = new vtknvindex_cluster_properties();
  m_cluster_properties->deserialize(deserializer);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_importer::get_references(mi::neuraylib::ITag_set* /*result*/) const
{
  // empty
}
