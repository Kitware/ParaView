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
  , m_scalar_type("")
  , m_cluster_properties(NULL)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_compute::vtknvindex_volume_compute(
  const mi::math::Vector_struct<mi::Uint32, 3>& volume_size, mi::Sint32 border_size,
  std::string scalar_type, vtknvindex_cluster_properties* cluster_properties)
  : m_volume_size(volume_size)
  , m_enabled(false)
  , m_border_size(border_size)
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
  mi::Sint32 rankid = controller ? controller->GetLocalProcessId() : 0;

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
    ERROR_LOG << LOG_prefix << "Unable to retrive sparse-volume data-subset.";
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

  mi::math::Bbox<mi::Float32, 3> request_bbox = svol_subset_desc->get_subregion_scene_space();

  // Fetch shared memory details from host properties
  std::string shm_memory_name;
  mi::math::Bbox<mi::Float32, 3> shm_bbox_flt;
  mi::Uint64 shmsize = 0;
  void* pv_subdivision_ptr = NULL;

  if (!m_cluster_properties->get_host_properties(rankid)->get_shminfo(
        request_bbox, shm_memory_name, shm_bbox_flt, shmsize, &pv_subdivision_ptr, 0))
  {
    ERROR_LOG << "Failed to get shared memory information in regular volume importer for rank: "
              << rankid << ".";
    return;
  }

  const Bbox3i shm_bbox(static_cast<mi::Sint32>(shm_bbox_flt.min.x),
    static_cast<mi::Sint32>(shm_bbox_flt.min.y), static_cast<mi::Sint32>(shm_bbox_flt.min.z),
    static_cast<mi::Sint32>(shm_bbox_flt.max.x), static_cast<mi::Sint32>(shm_bbox_flt.max.y),
    static_cast<mi::Sint32>(shm_bbox_flt.max.z));

  if (shm_memory_name.empty() || shm_bbox.empty())
  {
    ERROR_LOG << "Failed to open shared memory shmname: " << shm_memory_name
              << " with bbox: " << shm_bbox << ".";
    return;
  }

  INFO_LOG << LOG_prefix << "Reading '" << shm_memory_name << "', bounds: " << shm_bbox
           << ", format: " << vol_fmt << " [0x" << std::hex << vol_fmt << "]...";

  mi::Uint8* subdivision_ptr = nullptr;

  // Using volume subvision from ParaView scalar raw pointer
  if (pv_subdivision_ptr)
  {
    // Convert double scalar data to float.
    if (m_scalar_type == "double")
    {
      WARN_LOG << "Datasets in double format are not natively supported by IndeX.";
      WARN_LOG << "Converting scalar values to float format...";

      mi::Size nb_voxels = shmsize / sizeof(mi::Float64);
      void* subdivison_ptr_flt = malloc(nb_voxels * sizeof(mi::Float32));

      mi::Float32* voxels_flt = reinterpret_cast<mi::Float32*>(subdivison_ptr_flt);
      const mi::Float64* voxels_dlb = reinterpret_cast<mi::Float64*>(pv_subdivision_ptr);

      for (mi::Size i = 0; i < nb_voxels; ++i)
        voxels_flt[i] = static_cast<mi::Float32>(voxels_dlb[i]);

      pv_subdivision_ptr = subdivison_ptr_flt;
    }

    subdivision_ptr = reinterpret_cast<mi::Uint8*>(pv_subdivision_ptr);
  }
  else // Using volume subvision passed through shared memory pointer
  {
    subdivision_ptr = vtknvindex::util::get_vol_shm<mi::Uint8>(shm_memory_name, shmsize);
  }

  // Import all brick pieces in parallel
  vtknvindex_import_bricks import_bricks_job(svol_subset_desc.get(), svol_data_subset.get(),
    subdivision_ptr, vol_fmt_size, m_border_size, shm_bbox);

  dice_transaction->execute_fragmented(&import_bricks_job, import_bricks_job.get_nb_fragments());

  if (pv_subdivision_ptr)
  {
    // Free temporary voxel buffer
    if (m_scalar_type == "double")
      free(pv_subdivision_ptr);
  }
  else
  {
    // free memory space linked to shared memory
    vtknvindex::util::unmap_shm(subdivision_ptr, shmsize);
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
  serializer->write(&m_enabled, 1);

  mi::Uint32 scalar_typename_size = mi::Uint32(m_scalar_type.size());
  serializer->write(&scalar_typename_size, 1);
  serializer->write(
    reinterpret_cast<const mi::Uint8*>(m_scalar_type.c_str()), scalar_typename_size);

  serializer->write(&m_border_size, 1);
  serializer->write(&m_volume_size.x, 3);

  m_cluster_properties->serialize(serializer);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_compute::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  deserializer->read(&m_enabled, 1);

  mi::Uint32 scalar_typename_size = 0;
  deserializer->read(&scalar_typename_size, 1);
  m_scalar_type.resize(scalar_typename_size);
  deserializer->read(reinterpret_cast<mi::Uint8*>(&m_scalar_type[0]), scalar_typename_size);

  deserializer->read(&m_border_size, 1);
  deserializer->read(&m_volume_size.x, 3);

  m_cluster_properties = new vtknvindex_cluster_properties();
  m_cluster_properties->deserialize(deserializer);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_compute::get_references(mi::neuraylib::ITag_set* /*result*/) const
{
}
