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

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkUnstructuredGrid.h"
#include <vtkDataArray.h>

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_host_properties.h"
#include "vtknvindex_regular_volume_properties.h"
#include "vtknvindex_utilities.h"

// ------------------------------------------------------------------------------------------------
vtknvindex_regular_volume_properties::vtknvindex_regular_volume_properties()
  : m_is_timeseries_data(false)
  , m_time_steps_written(0)
  , m_nb_time_steps(1)
  , m_current_time_step(0)
  , m_ghost_levels(0)

{
  m_volume_translation = mi::math::Vector<mi::Float32, 3>(0.f);
  m_volume_scaling = mi::math::Vector<mi::Float32, 3>(1.f);
  m_volume_extents.min = m_volume_extents.max = mi::math::Vector<mi::Sint32, 3>(0);
}

// ------------------------------------------------------------------------------------------------
vtknvindex_regular_volume_properties::~vtknvindex_regular_volume_properties()
{
  // empty
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_scalar_type(std::string scalar_type)
{
  m_scalar_type = scalar_type;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::get_scalar_type(std::string& scalar_type) const
{
  scalar_type = m_scalar_type;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_voxel_range(
  mi::math::Vector<mi::Float32, 2> voxel_range)
{
  m_voxel_range = voxel_range;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::get_voxel_range(
  mi::math::Vector<mi::Float32, 2>& voxel_range) const
{
  voxel_range = m_voxel_range;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_scalar_range(
  mi::math::Vector<mi::Float32, 2> scalar_range)
{
  m_scalar_range = scalar_range;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::get_scalar_range(
  mi::math::Vector<mi::Float32, 2>& scalar_range) const
{
  scalar_range = m_scalar_range;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_regular_volume_properties::is_timeseries_data() const
{
  return m_is_timeseries_data;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_is_timeseries_data(bool is_timeseries)
{
  m_is_timeseries_data = is_timeseries;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_nb_time_steps(mi::Uint32 nb_time_steps)
{
  m_nb_time_steps = nb_time_steps;
}

//-------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_regular_volume_properties::get_nb_time_steps() const
{
  return m_nb_time_steps;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_current_time_step(mi::Uint64 current_time_step)
{
  m_current_time_step = current_time_step;
}

//-------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_regular_volume_properties::get_current_time_step() const
{
  return m_current_time_step;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_time_step_start(mi::Uint32 time_step_start)
{
  m_time_step_start = time_step_start;
}

//-------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_regular_volume_properties::get_time_step_start() const
{
  return m_time_step_start;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_volume_size(
  mi::math::Vector_struct<mi::Uint32, 3> volume_size)
{
  m_volume_size = volume_size;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::get_volume_size(
  mi::math::Vector_struct<mi::Uint32, 3>& volume_size) const
{
  volume_size = m_volume_size;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_volume_extents(
  mi::math::Bbox<mi::Sint32, 3> volume_extents)
{
  m_volume_extents = volume_extents;
  m_ivol_volume_extents.min.x = volume_extents.min.x;
  m_ivol_volume_extents.min.y = volume_extents.min.y;
  m_ivol_volume_extents.min.z = volume_extents.min.z;
  m_ivol_volume_extents.max.x = volume_extents.max.x;
  m_ivol_volume_extents.max.y = volume_extents.max.y;
  m_ivol_volume_extents.max.z = volume_extents.max.z;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::get_volume_extents(
  mi::math::Bbox<mi::Sint32, 3>& volume_extents) const
{
  volume_extents = m_volume_extents;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_ivol_volume_extents(
  mi::math::Bbox<mi::Float32, 3> volume_extents)
{
  m_ivol_volume_extents = volume_extents;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::get_ivol_volume_extents(
  mi::math::Bbox<mi::Float32, 3>& volume_extents) const
{
  volume_extents = m_ivol_volume_extents;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_ghost_levels(mi::Sint32 ghost_levels)
{
  m_ghost_levels = ghost_levels;
}

// ------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_regular_volume_properties::get_ghost_levels() const
{
  return m_ghost_levels;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_volume_translation(
  mi::math::Vector<mi::Float32, 3> translation)
{
  m_volume_translation = translation;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::get_volume_translation(
  mi::math::Vector<mi::Float32, 3>& translation) const
{
  translation = m_volume_translation;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::set_volume_scaling(
  mi::math::Vector<mi::Float32, 3> scaling)
{
  m_volume_scaling = scaling;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::get_volume_scaling(
  mi::math::Vector<mi::Float32, 3>& scaling) const
{
  scaling = m_volume_scaling;
}

//---------------------------------------------------------------------------
template <typename T>
void vtknvindex_regular_volume_properties::transform_zyx_to_xyz(
  T* pv_volume, T* shm_volume, mi::Sint32* bounds) const
{
  if (shm_volume == NULL || pv_volume == NULL)
    return;

  mi::Uint64 dx = bounds[1] - bounds[0] + 1;
  mi::Uint64 dy = bounds[3] - bounds[2] + 1;
  mi::Uint64 dz = bounds[5] - bounds[4] + 1;
  mi::Uint64 dxdy = dx * dy;

  mi::Uint64 index_offset = 0;
  for (mi::Uint64 x = 0; x < dx; ++x)
  {
    for (mi::Uint64 y = 0; y < dy; ++y)
    {
      mi::Uint64 pv_offset = x + y * dx;
      for (mi::Uint64 z = 0; z < dz; ++z)
      {
        shm_volume[index_offset] = pv_volume[pv_offset];
        index_offset++;
        pv_offset += dxdy;
      }
    }
  }
}

//----------------------------------------------------------------------------
bool vtknvindex_regular_volume_properties::write_shared_memory(vtkDataArray* scalar_array,
  mi::Sint32* bounds, vtknvindex_host_properties* host_properties, mi::Uint32 current_timestep,
  bool use_shared_mem)
{
  if (!host_properties)
  {
    ERROR_LOG << "Invalid host_properties in "
              << " vtknvindex_regular_volume_properties::write_shared_memory ";
    return false;
  }

  // If the origin is not [0,0,0] we have to translate all the pieces.
  mi::math::Bbox<mi::Float32, 3> current_bbox(bounds[0] - m_volume_extents.min.x,
    bounds[2] - m_volume_extents.min.y, bounds[4] - m_volume_extents.min.z,
    bounds[1] - m_volume_extents.min.x + 1, bounds[3] - m_volume_extents.min.y + 1,
    bounds[5] - m_volume_extents.min.z + 1);

  vtknvindex_host_properties::shm_info* shm_info =
    host_properties->get_shminfo(current_bbox, current_timestep);
  if (!shm_info)
  {
    ERROR_LOG << "Failed to get shared memory in "
              << "vtknvindex_regular_volume_properties::write_shared_memory.";
    return false;
  }

  if (shm_info->m_shm_name.empty())
  {
    ERROR_LOG << "Received invalid shared memory in "
              << "vtknvindex_regular_volume_properties::write_shared_memory.";
    return false;
  }

  if (use_shared_mem)
  {
    if (shm_info->m_size == 0)
    {
      return false;
    }

    mi::Uint8* shm_ptr = vtknvindex::util::get_vol_shm(shm_info->m_shm_name, shm_info->m_size);
    if (!shm_ptr)
    {
      ERROR_LOG << "Error retrieving shared memory pointer in "
                << "vtknvindex_regular_volume_properties::write_shared_memory.";
      return false;
    }

    const std::string scalar_type = scalar_array->GetDataTypeAsString();
    if (scalar_type == "double")
    {
      // Convert double to float data.
      // shm_ptr and m_size were already set up for float data.
      const mi::Size nb_voxels = shm_info->m_size / sizeof(mi::Float32);

      const mi::Float64* src =
        reinterpret_cast<const mi::Float64*>(scalar_array->GetVoidPointer(0));
      mi::Float32* dst = reinterpret_cast<mi::Float32*>(shm_ptr);
      for (mi::Size i = 0; i < nb_voxels; ++i)
      {
        dst[i] = static_cast<mi::Float32>(src[i]);
      }
    }
    else
    {
      memcpy(shm_ptr, scalar_array->GetVoidPointer(0), shm_info->m_size);
    }

    // free memory space linked to shared memory
    vtknvindex::util::unmap_shm(shm_ptr, shm_info->m_size);
  }
  else
  {
    // shm_info->m_subset_ptr = scalar_array->GetVoidPointer(0);
  }

  m_time_steps_written++;

  if (use_shared_mem)
  {
    INFO_LOG << "Done writing bounding box " << current_bbox
             << " into shared memory: " << shm_info->m_shm_name << ".";
  }
  else
  {
    INFO_LOG << "Bounding box " << current_bbox
             << " is available in local memory: " << shm_info->m_shm_name << ".";
  }

  return true;
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_regular_volume_properties::write_shared_memory(
  vtknvindex_irregular_volume_data* ivol_data, vtkUnstructuredGridBase* ugrid,
  vtknvindex_host_properties* host_properties, mi::Uint32 current_timestep)
{
  if (!host_properties)
  {
    ERROR_LOG << "Invalid host properties in "
              << " vtknvindex_regular_volume_properties::write_shared_memory.";
    return false;
  }

  std::string shm_memory_name;
  mi::math::Bbox<mi::Float32, 3> shm_bbox;
  mi::Uint64 shm_size = 0;
  void* subset_ptr = NULL;

  if (!host_properties->get_shminfo(ivol_data->subregion_bbox, shm_memory_name, shm_bbox, shm_size,
        &subset_ptr, current_timestep))
  {
    ERROR_LOG << "Failed to get shared memory in "
              << "vtknvindex_regular_volume_properties::write_shared_memory.";
    return false;
  }

  if (shm_memory_name.empty())
  {
    ERROR_LOG << "Received invalid shared memory in "
              << "vtknvindex_regular_volume_properties::write_shared_memory.";
    return false;
  }

  // check scalar type
  const mi::Size scalar_size = get_scalar_size(m_scalar_type);
  if (scalar_size == 0)
  {
    return false;
  }

  mi::Uint8* shm_ptr = vtknvindex::util::get_vol_shm(shm_memory_name, shm_size);
  if (!shm_ptr)
  {
    ERROR_LOG << "Encountered an error when retrieving a shred memory pointer in "
              << "vtknvindex_regular_volume_properties::write_shared_memory.";
    return false;
  }

  mi::Uint8* shm_offset = shm_ptr;
  size_t size_elm;

  // copy irregular volume data to shared memory

  // num points
  size_elm = sizeof(ivol_data->num_points);
  memcpy(shm_offset, &ivol_data->num_points, size_elm);
  shm_offset += size_elm;

  // num cells
  size_elm = sizeof(ivol_data->num_cells);
  memcpy(shm_offset, &ivol_data->num_cells, size_elm);
  shm_offset += size_elm;

  // num scalars
  size_elm = sizeof(ivol_data->num_scalars);
  memcpy(shm_offset, &ivol_data->num_scalars, size_elm);
  shm_offset += size_elm;

  // cell flag
  size_elm = sizeof(ivol_data->cell_flag);
  memcpy(shm_offset, &ivol_data->cell_flag, size_elm);
  shm_offset += size_elm;

  // points
  size_elm = sizeof(mi::Float32) * 3;

  for (vtkIdType i = 0; i < ivol_data->num_points; ++i)
  {
    mi::math::Vector<mi::Float64, 3> point_pv;
    ugrid->GetPoint(i, point_pv.begin());
    const mi::math::Vector<mi::Float32, 3> point_index(point_pv);
    memcpy(shm_offset, &point_index, size_elm);
    shm_offset += size_elm;
  }

  // cells
  size_elm = sizeof(mi::Uint32) * 4;

  const bool per_cell_scalars = (ivol_data->cell_flag == 1);
  mi::Uint8* shm_scalar_offset = shm_offset + ivol_data->num_cells * size_elm;

  vtkSmartPointer<vtkCellIterator> cellIter =
    vtkSmartPointer<vtkCellIterator>::Take(ugrid->NewCellIterator());
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    const vtkIdType npts = cellIter->GetNumberOfPoints();
    if (npts != 4)
      continue;

    const vtkIdType* cell_point_ids = cellIter->GetPointIds()->GetPointer(0);

    const mi::Uint32 cur_cell_index[4] = { static_cast<mi::Uint32>(cell_point_ids[0]),
      static_cast<mi::Uint32>(cell_point_ids[1]), static_cast<mi::Uint32>(cell_point_ids[2]),
      static_cast<mi::Uint32>(cell_point_ids[3]) };

    memcpy(shm_offset, cur_cell_index, size_elm);
    shm_offset += size_elm;

    if (per_cell_scalars)
    {
      // copy per-cell scalars
      const vtkIdType cell_id = cellIter->GetCellId();
      memcpy(shm_scalar_offset,
        &reinterpret_cast<const mi::Uint8*>(ivol_data->scalars)[cell_id * scalar_size],
        scalar_size);
      shm_scalar_offset += scalar_size;
    }
  }

  // scalars
  size_elm = scalar_size * ivol_data->num_scalars;
  if (!per_cell_scalars)
  {
    // copy per-point scalars here (any per-cell scalars were already copied in the loop above)
    memcpy(shm_offset, ivol_data->scalars, size_elm);
  }
  shm_offset += size_elm;

  // max edge length
  size_elm = sizeof(ivol_data->max_edge_length2);
  memcpy(shm_offset, &ivol_data->max_edge_length2, size_elm);
  shm_offset += size_elm;

  if (shm_offset - shm_ptr != static_cast<mi::Difference>(shm_size))
  {
    ERROR_LOG << "Encountered a shared memory copy mismatch, diff: " << shm_offset - shm_ptr
              << ", size: " << shm_size << ".";
    return false;
  }

  m_time_steps_written++;

  INFO_LOG << "Finished writing the bounding box: " << ivol_data->subregion_bbox
           << " to shared memory: " << shm_memory_name << ".";

  // free memory space linked to shared memory
  vtknvindex::util::unmap_shm(shm_ptr, shm_size);

  return true;
}

// Print all the host details
// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::print_info() const
{
  INFO_LOG << "Scalar type: " << m_scalar_type;
  INFO_LOG << "Volume bbox: " << m_ivol_volume_extents;
  INFO_LOG << "Volume size: " << m_ivol_volume_extents.max - m_ivol_volume_extents.min;
  INFO_LOG << "Data values: " << m_voxel_range;
  INFO_LOG << "Time series: " << ((m_is_timeseries_data) ? "Yes" : "No");
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::serialize(mi::neuraylib::ISerializer* serializer) const
{
  // Serialize scalar type string.
  mi::Uint32 scalar_typename_size = mi::Uint32(m_scalar_type.size());
  serializer->write(&scalar_typename_size);
  serializer->write(
    reinterpret_cast<const mi::Uint8*>(m_scalar_type.c_str()), scalar_typename_size);

  serializer->write(&m_voxel_range.x, 2);

  serializer->write(&m_ghost_levels);
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_regular_volume_properties::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  // Deserialize scalar type string.
  mi::Uint32 scalar_typename_size = 0;
  deserializer->read(&scalar_typename_size);
  m_scalar_type.resize(scalar_typename_size);
  deserializer->read(reinterpret_cast<mi::Uint8*>(&m_scalar_type[0]), scalar_typename_size);

  deserializer->read(&m_voxel_range.x, 2);

  deserializer->read(&m_ghost_levels);
}

// ------------------------------------------------------------------------------------------------
mi::Size vtknvindex_regular_volume_properties::get_scalar_size(const std::string& scalar_type)
{
  mi::Size scalar_size = 0;

  if (scalar_type == "char" || scalar_type == "unsigned char")
  {
    scalar_size = sizeof(mi::Uint8);
  }
  else if (scalar_type == "short" || scalar_type == "unsigned short")
  {
    scalar_size = sizeof(mi::Uint16);
  }
  else if (scalar_type == "float")
  {
    scalar_size = sizeof(mi::Float32);
  }
  else if (scalar_type == "double")
  {
    scalar_size = sizeof(mi::Float64);
  }

  return scalar_size;
}
