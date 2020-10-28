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

#include "vtknvindex_volume_compute.h"
#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_sparse_volume_importer.h"
#include "vtknvindex_utilities.h"

inline mi::Sint32 volume_format_size(const nv::index::Sparse_volume_voxel_format fmt)
{
  switch (fmt)
  {
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT8:
      return 1 * sizeof(mi::Uint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT8:
      return 1 * sizeof(mi::Sint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT16:
      return 1 * sizeof(mi::Uint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT16:
      return 1 * sizeof(mi::Sint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32:
      return 1 * sizeof(mi::Float32);
    default:
      return 0;
  }
}

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_compute::vtknvindex_volume_compute()
  : m_enabled(false)
  , m_border_size(0)
  , m_ghost_levels(0)
  , m_scalar_type("")
  , m_cluster_properties(NULL)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_compute::vtknvindex_volume_compute(
  const mi::math::Vector_struct<mi::Uint32, 3>& volume_size, mi::Sint32 border_size,
  const mi::Sint32& ghost_levels, std::string scalar_type,
  vtknvindex_cluster_properties* cluster_properties)
  : m_volume_size(volume_size)
  , m_enabled(false)
  , m_border_size(border_size)
  , m_ghost_levels(ghost_levels)
  , m_scalar_type(scalar_type)
  , m_cluster_properties(cluster_properties)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_compute::launch_compute(mi::neuraylib::IDice_transaction* dice_transaction,
  nv::index::IDistributed_compute_destination_buffer* dst_buffer) const
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  const mi::Sint32 rankid = controller ? controller->GetLocalProcessId() : 0;

  static const std::string LOG_prefix = "Sparse volume compute technique: ";

  using namespace nv::index;
  using namespace vtknvindex::util;
  using mi::base::Handle;
  using nv::index::IDistributed_compute_destination_buffer_3d_sparse_volume;

  // retrieve 2d texture buffer destination buffer interface
  const Handle<IDistributed_compute_destination_buffer_3d_sparse_volume> svol_dst_buffer(
    dst_buffer->get_interface<IDistributed_compute_destination_buffer_3d_sparse_volume>());

  if (!svol_dst_buffer)
  {
    ERROR_LOG << LOG_prefix
              << "Unable to retrieve valid 3d sparse-volume destination buffer interface.";
    return;
  }

  mi::base::Handle<ISparse_volume_subset> svol_data_subset(
    svol_dst_buffer->get_distributed_data_subset());

  if (!svol_data_subset.is_valid_interface())
  {
    ERROR_LOG << LOG_prefix << "Unable to retrieve sparse-volume data-subset.";
    return;
  }

  mi::base::Handle<const ISparse_volume_attribute_set_descriptor> svol_attrib_desc(
    svol_data_subset->get_attribute_set_descriptor());
  mi::base::Handle<const ISparse_volume_subset_data_descriptor> svol_subset_desc(
    svol_data_subset->get_subset_data_descriptor());

  // we are writing the first attribute
  const mi::Uint32 active_compute_attrib_idx = 0u;
  ISparse_volume_attribute_set_descriptor::Attribute_parameters active_compute_attrib_params;

  if (!svol_attrib_desc->get_attribute_parameters(
        active_compute_attrib_idx, active_compute_attrib_params))
  {
    ERROR_LOG << LOG_prefix << "Unable to retrieve valid attribute parameters for attribute index "
              << active_compute_attrib_idx << ".";
    return;
  }

  const nv::index::Sparse_volume_voxel_format vol_fmt = active_compute_attrib_params.format;
  const mi::Sint32 vol_fmt_size = volume_format_size(vol_fmt);

  const mi::math::Bbox<mi::Float32, 3> subset_subregion_bbox =
    svol_subset_desc->get_subregion_scene_space();

  // Fetch shared memory details from host properties
  std::string shm_memory_name;
  mi::math::Bbox<mi::Float32, 3> shm_bbox_flt;

  vtknvindex_host_properties* host_props = m_cluster_properties->get_host_properties(rankid);

  const vtknvindex_host_properties::shm_info* shm_info;
  const mi::Uint32 time_step = 0;
  const mi::Uint8* subset_data_buffer =
    host_props->get_subset_data_buffer(subset_subregion_bbox, time_step, &shm_info);

  if (!shm_info)
  {
    ERROR_LOG << LOG_prefix << "Failed to retrieve shared memory info for subset "
              << subset_subregion_bbox << ".";
    return;
  }

  const Bbox3i shm_bbox(shm_info->m_shm_bbox); // no rounding necessary, only uses integer values

  if (shm_info->m_shm_name.empty() || shm_bbox.empty())
  {
    ERROR_LOG << LOG_prefix << "Failed to open shared memory shmname: " << shm_info->m_shm_name
              << " with bbox: " << shm_bbox << ".";
    return;
  }
  if (!subset_data_buffer)
  {
    ERROR_LOG << LOG_prefix << "Could not retrieve data for shared memory: " << shm_info->m_shm_name
              << " box " << shm_bbox << " from rank: " << rankid << ".";
    return;
  }

  bool free_buffer = false;
  // Convert double scalar data to float, but only if the data is local. Data in shared memory will
  // already have been converted at this point.
  if (shm_info->m_rank_id == rankid && m_scalar_type == "double")
  {
    mi::Size nb_voxels = shm_info->m_size / sizeof(mi::Float64);

    mi::Float32* voxels_float = new mi::Float32[nb_voxels];
    const mi::Float64* voxels_double = reinterpret_cast<const mi::Float64*>(subset_data_buffer);

    for (mi::Size i = 0; i < nb_voxels; ++i)
      voxels_float[i] = static_cast<mi::Float32>(voxels_double[i]);

    subset_data_buffer = reinterpret_cast<mi::Uint8*>(voxels_float);
    free_buffer = true;
  }

  if (shm_info->m_neighbors)
  {
    // Ensure border data is available
    shm_info->m_neighbors->fetch_data(host_props, time_step);
  }

  INFO_LOG << "Updating volume data from " << (rankid == shm_info->m_rank_id ? "local" : "shared")
           << " "
           << "memory (" << shm_info->m_shm_name << ") on rank " << rankid << ", "
           << (m_scalar_type == "double" ? "converted to float, " : "") << "data bbox " << shm_bbox
           << ", "
           << "subset bbox " << subset_subregion_bbox << ", border " << m_ghost_levels << "/"
           << m_border_size << ".";

  // Import all brick pieces in parallel
  vtknvindex_import_bricks import_bricks_job(svol_subset_desc.get(), svol_data_subset.get(),
    subset_data_buffer, vol_fmt_size, m_border_size, m_ghost_levels, shm_bbox,
    shm_info->m_neighbors.get());

  dice_transaction->execute_fragmented(&import_bricks_job, import_bricks_job.get_nb_fragments());

  host_props->set_read_flag(time_step, shm_info->m_shm_name);

  if (free_buffer)
  {
    // Free temporary data buffer (used for double-float conversion)
    delete[] subset_data_buffer;
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_compute::set_enabled(bool enable)
{
  m_enabled = enable;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_volume_compute::get_enabled() const
{
  return m_enabled;
}

//-------------------------------------------------------------------------------------------------
mi::neuraylib::IElement* vtknvindex_volume_compute::copy() const
{
  vtknvindex_volume_compute* other = new vtknvindex_volume_compute();

  other->m_enabled = this->m_enabled;
  other->m_border_size = this->m_border_size;
  other->m_ghost_levels = this->m_ghost_levels;
  other->m_scalar_type = this->m_scalar_type;
  other->m_cluster_properties = this->m_cluster_properties;
  other->m_volume_size = this->m_volume_size;

  return other;
}

//-------------------------------------------------------------------------------------------------
const char* vtknvindex_volume_compute::get_class_name() const
{
  return "vtknvindex_volume_compute";
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_compute::serialize(mi::neuraylib::ISerializer* serializer) const
{
  vtknvindex::util::serialize(serializer, m_scalar_type);
  serializer->write(&m_enabled);
  serializer->write(&m_border_size);
  serializer->write(&m_ghost_levels);
  serializer->write(&m_volume_size.x, 3);

  const mi::Uint32 instance_id = m_cluster_properties->get_instance_id();
  serializer->write(&instance_id);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_compute::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  vtknvindex::util::deserialize(deserializer, m_scalar_type);
  deserializer->read(&m_enabled);
  deserializer->read(&m_border_size);
  deserializer->read(&m_ghost_levels);
  deserializer->read(&m_volume_size.x, 3);

  mi::Uint32 instance_id;
  deserializer->read(&instance_id);
  m_cluster_properties = vtknvindex_cluster_properties::get_instance(instance_id);
}
