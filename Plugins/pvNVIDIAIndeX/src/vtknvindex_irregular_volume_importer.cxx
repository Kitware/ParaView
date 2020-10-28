/* Copyright 2020 NVIDIA Corporation. All rights reserved.
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
  const std::string& scalar_type)
  : m_scalar_type(scalar_type)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_irregular_volume_importer::vtknvindex_irregular_volume_importer()
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
  const mi::Sint32 rank_id = controller ? controller->GetLocalProcessId() : 0;

  const std::type_info* scalar_type_info =
    (m_scalar_type == "unsigned char") ? &typeid(mi::Uint8) : (m_scalar_type == "unsigned short")
      ? &typeid(mi::Uint16)
      : (m_scalar_type == "float") ? &typeid(mi::Float32) : &typeid(mi::Float64);

  // Fetch shared memory details from host properties.
  const mi::Uint32 time_step = 0;
  const bool support_time_steps = false;
  const mi::math::Bbox<mi::Float32, 3> query_bbox(
    mi::math::Bbox<mi::Float32, 3>(bounding_box).center());
  const vtknvindex_host_properties::shm_info* shm_info;
  const mi::Uint8* subset_data_buffer =
    m_cluster_properties->get_host_properties(rank_id)->get_subset_data_buffer(
      query_bbox, time_step, &shm_info, support_time_steps);

  if (!shm_info)
  {
    ERROR_LOG << "Failed to retrieve shared memory info for subset " << query_bbox << " on rank "
              << rank_id << ".";
    return nullptr;
  }

  const mi::math::Bbox<mi::Float32, 3>& shm_bbox = shm_info->m_shm_bbox;

  if (!subset_data_buffer)
  {
    ERROR_LOG << "Could not retrieve data for shared memory: " << shm_info->m_shm_name << " box "
              << shm_bbox << " on rank " << rank_id << ".";
    return nullptr;
  }

  const bool data_is_local = (rank_id == shm_info->m_rank_id);

  bool face_filtering = false;
  if (bounding_box.min.x > shm_bbox.min.x || bounding_box.min.y > shm_bbox.min.y ||
    bounding_box.min.z > shm_bbox.min.z || bounding_box.max.x < shm_bbox.max.x ||
    bounding_box.max.y < shm_bbox.max.y || bounding_box.max.z < shm_bbox.max.z)
  {
    face_filtering = true;
  }

  vtkUnstructuredGridBase* pv_ugrid = nullptr;

  mi::Uint32 num_cells = 0u; // only used with shared memory
  bool per_cell_scalars = false;
  const mi::math::Vector<mi::Float32, 3>* points = nullptr;
  const mi::math::Vector<mi::Uint32, 4>* cells = nullptr;
  const void* scalars = nullptr;
  mi::Float32 max_edge_length_sqr = 0.f;

  INFO_LOG << "Importing unstructured volume data from " << (data_is_local ? "local" : "shared")
           << " memory (" << shm_info->m_shm_name << ") on rank " << rank_id << ", "
           << "data bbox " << shm_bbox << ", importer bbox " << bounding_box << ".";

  if (data_is_local) // The data is available in local memory.
  {
    const vtknvindex_irregular_volume_data* ivol_data =
      reinterpret_cast<const vtknvindex_irregular_volume_data*>(subset_data_buffer);

    pv_ugrid = ivol_data->pv_unstructured_grid;
    scalars = ivol_data->scalars;
    max_edge_length_sqr = ivol_data->max_edge_length2;
    per_cell_scalars = (ivol_data->cell_flag == 1);
  }
  else // The data is in shared memory.
  {
    const mi::Uint8* shm_offset = subset_data_buffer;
    size_t size_elm;

    // num points
    mi::Uint32 num_points;
    size_elm = sizeof(num_points);
    num_points = *reinterpret_cast<const mi::Uint32*>(shm_offset);
    shm_offset += size_elm;

    // num cells
    size_elm = sizeof(num_cells);
    num_cells = *reinterpret_cast<const mi::Uint32*>(shm_offset);
    shm_offset += size_elm;

    // num scalars
    mi::Uint32 num_scalars;
    size_elm = sizeof(num_scalars);
    num_scalars = *reinterpret_cast<const mi::Uint32*>(shm_offset);
    shm_offset += size_elm;

    // cell flag
    mi::Sint32 cell_flag;
    size_elm = sizeof(cell_flag);
    cell_flag = *reinterpret_cast<const mi::Sint32*>(shm_offset);
    per_cell_scalars = (cell_flag == 1);
    shm_offset += size_elm;

    // points
    size_elm = sizeof(mi::Float32) * 3 * num_points;
    points = reinterpret_cast<const mi::math::Vector<mi::Float32, 3>*>(shm_offset);
    shm_offset += size_elm;

    // cells
    size_elm = sizeof(mi::Uint32) * 4 * num_cells;
    cells = reinterpret_cast<const mi::math::Vector<mi::Uint32, 4>*>(shm_offset);
    shm_offset += size_elm;

    // scalars
    if (*scalar_type_info == typeid(mi::Uint8))
      size_elm = sizeof(mi::Uint8) * num_scalars;
    else if (*scalar_type_info == typeid(mi::Uint16))
      size_elm = sizeof(mi::Uint16) * num_scalars;
    else if (*scalar_type_info == typeid(mi::Float32))
      size_elm = sizeof(mi::Float32) * num_scalars;
    else // typeid(mi::Float64)
      size_elm = sizeof(mi::Float64) * num_scalars;

    scalars = reinterpret_cast<const void*>(shm_offset);
    shm_offset += size_elm;

    // max square edge length
    max_edge_length_sqr = *reinterpret_cast<const mi::Float32*>(shm_offset);
  }

  typedef mi::math::Vector<mi::Uint32, 4> Vec4ui;
  typedef mi::math::Vector<mi::Float32, 3> Vec3f;

  // Read the tetrahedrons and collect only those that intersects the subset bounding box.
  std::vector<Vec4ui> subset_tetrahedrons;
  std::vector<Vec3f> subset_vertices;
  std::vector<mi::Uint8> subset_scalars_uint8;
  std::vector<mi::Uint16> subset_scalars_uint16;
  std::vector<mi::Float32> subset_scalars_float32;
  std::unordered_map<mi::Uint32, mi::Uint32> global_to_local_vtx_idx_map;

  if (pv_ugrid != nullptr)
  {
    // The data is available in local memory.
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
          ERROR_LOG << "Encountered non-tetrahedral cell with " << npts
                    << " points. The NVIDIA IndeX plugin currently "
                       "supports tetrahedral cells only.";
          gave_error = true;
        }
        continue;
      }

      const vtkIdType* cell_point_ids = cellIter->GetPointIds()->GetPointer(0);
      mi::math::Bbox<mi::Float32, 3> tet_bbox;
      for (mi::Uint32 i = 0; i < 4; i++)
      {
        mi::math::Vector<mi::Float64, 3> point;
        pv_ugrid->GetPoint(cell_point_ids[i], point.begin());
        tet_bbox.insert(Vec3f(point));
      }

      if (!face_filtering || tet_bbox.intersects(bounding_box))
      {
        const Vec4ui tet_vtx_indices(
          cell_point_ids[0], cell_point_ids[1], cell_point_ids[2], cell_point_ids[3]);
        subset_tetrahedrons.push_back(tet_vtx_indices);

        if (per_cell_scalars)
        {
          const vtkIdType cell_id = cellIter->GetCellId();

          if (*scalar_type_info == typeid(mi::Uint8))
            subset_scalars_uint8.push_back((reinterpret_cast<const mi::Uint8*>(scalars))[cell_id]);
          else if (*scalar_type_info == typeid(mi::Uint16))
            subset_scalars_uint16.push_back(
              (reinterpret_cast<const mi::Uint16*>(scalars))[cell_id]);
          else if (*scalar_type_info == typeid(mi::Float32))
            subset_scalars_float32.push_back(
              (reinterpret_cast<const mi::Float32*>(scalars))[cell_id]);
          else // typeid(mi::Float64)
            subset_scalars_float32.push_back(
              static_cast<mi::Float32>((reinterpret_cast<const mi::Float64*>(scalars))[cell_id]));
        }
      }
    }
  }
  else
  {
    // The data is in shared memory.
    for (mi::Uint32 t = 0u; t < num_cells; ++t)
    {
      const Vec4ui& tet_vtx_indices = cells[t];

      mi::math::Bbox<mi::Float32, 3> tet_bbox;
      for (mi::Uint32 k = 0; k < 4; k++)
      {
        tet_bbox.insert(points[tet_vtx_indices[k]]);
      }

      if (!face_filtering || tet_bbox.intersects(bounding_box))
      {
        subset_tetrahedrons.push_back(tet_vtx_indices);

        if (per_cell_scalars)
        {
          if (*scalar_type_info == typeid(mi::Uint8))
            subset_scalars_uint8.push_back((reinterpret_cast<const mi::Uint8*>(scalars))[t]);
          else if (*scalar_type_info == typeid(mi::Uint16))
            subset_scalars_uint16.push_back((reinterpret_cast<const mi::Uint16*>(scalars))[t]);
          else if (*scalar_type_info == typeid(mi::Float32))
            subset_scalars_float32.push_back((reinterpret_cast<const mi::Float32*>(scalars))[t]);
          else // typeid(mi::Float64)
            subset_scalars_float32.push_back(
              static_cast<mi::Float32>((reinterpret_cast<const mi::Float64*>(scalars))[t]));
        }
      }
    }
  }

  // Build subset vertex list and remap indices.
  const mi::Size nb_subset_tetrahedrons = subset_tetrahedrons.size();

  // if no tetrahedrons in this subregion then return an empty subset.
  if (nb_subset_tetrahedrons == 0)
  {
    mi::base::Handle<nv::index::IIrregular_volume_subset> irregular_volume_subset(
      factory->create_data_subset<nv::index::IIrregular_volume_subset>());

    if (!irregular_volume_subset.is_valid_interface())
    {
      ERROR_LOG << "The importer cannot create an irregular volume subset.";
      return nullptr;
    }

    irregular_volume_subset->retain();

    vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Importing");

    return irregular_volume_subset.get();
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
          const mi::Float64* cur_pv_vertex = pv_ugrid->GetPoint(vtx_index);
          const Vec3f cur_vertex(static_cast<mi::Float32>(cur_pv_vertex[0]),
            static_cast<mi::Float32>(cur_pv_vertex[1]), static_cast<mi::Float32>(cur_pv_vertex[2]));

          subset_vertices.push_back(cur_vertex);
        }
        else
        {
          subset_vertices.push_back(points[vtx_index]);
        }

        if (!per_cell_scalars)
        {
          if (*scalar_type_info == typeid(mi::Uint8))
            subset_scalars_uint8.push_back(
              (reinterpret_cast<const mi::Uint8*>(scalars))[vtx_index]);
          else if (*scalar_type_info == typeid(mi::Uint16))
            subset_scalars_uint16.push_back(
              (reinterpret_cast<const mi::Uint16*>(scalars))[vtx_index]);
          else if (*scalar_type_info == typeid(mi::Float32))
            subset_scalars_float32.push_back(
              (reinterpret_cast<const mi::Float32*>(scalars))[vtx_index]);
          else // typeid(mi::Float64)
            subset_scalars_float32.push_back(
              static_cast<mi::Float32>((reinterpret_cast<const mi::Float64*>(scalars))[vtx_index]));
        }

        vtx_index = new_vtx_idx;
      }
    }
  }

  const mi::Size nb_subset_vertices = subset_vertices.size();
  const mi::Size nb_cells = nb_subset_tetrahedrons;
  const mi::Size nb_cell_face_indices = nb_cells * 4u;

  const mi::Size nb_faces = nb_cell_face_indices;
  const mi::Size nb_face_vtx_indices = nb_faces * 3u;

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
    return nullptr;
  }

  nv::index::IIrregular_volume_subset::Mesh_storage mesh_storage;
  if (!irregular_volume_subset->generate_mesh_storage(mesh_params, mesh_storage))
  {
    ERROR_LOG << "The importer is unable to generate an irregular volume mesh storage.";
    return nullptr;
  }

  nv::index::IIrregular_volume_subset::Attribute_parameters attrib_params;
  if (per_cell_scalars)
  {
    attrib_params.affiliation = nv::index::IIrregular_volume_subset::ATTRIB_AFFIL_PER_CELL;
    attrib_params.nb_attrib_values = nb_cells;
  }
  else
  {
    attrib_params.affiliation = nv::index::IIrregular_volume_subset::ATTRIB_AFFIL_PER_VERTEX;
    attrib_params.nb_attrib_values = nb_subset_vertices;
  }

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
    return nullptr;
  }

  // Copy vertices.
  memcpy(mesh_storage.vertices, subset_vertices.data(),
    subset_vertices.size() * sizeof(mesh_storage.vertices[0]));

  // Copy scalars.
  const void* scalars_buffer;
  mi::Size scalars_buffer_size;
  if (*scalar_type_info == typeid(mi::Uint8))
  {
    scalars_buffer = subset_scalars_uint8.data();
    scalars_buffer_size = subset_scalars_uint8.size() * sizeof(mi::Uint8);
  }
  else if (*scalar_type_info == typeid(mi::Uint16))
  {
    scalars_buffer = subset_scalars_uint16.data();
    scalars_buffer_size = subset_scalars_uint16.size() * sizeof(mi::Uint16);
  }
  else // typeid(mi::Float32)
  {
    scalars_buffer = subset_scalars_float32.data();
    scalars_buffer_size = subset_scalars_float32.size() * sizeof(mi::Float32);
  }

  memcpy(attribute_storage.attrib_values, scalars_buffer, scalars_buffer_size);

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

  m_cluster_properties->get_host_properties(rank_id)->set_read_flag(
    time_step, shm_info->m_shm_name);

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
  vtknvindex::util::serialize(serializer, m_scalar_type);

  const mi::Uint32 instance_id = m_cluster_properties->get_instance_id();
  serializer->write(&instance_id);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_importer::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  vtknvindex::util::deserialize(deserializer, m_scalar_type);

  mi::Uint32 instance_id;
  deserializer->read(&instance_id);
  m_cluster_properties = vtknvindex_cluster_properties::get_instance(instance_id);
}
