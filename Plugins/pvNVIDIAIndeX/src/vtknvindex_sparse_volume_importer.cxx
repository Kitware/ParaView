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

#include <string>

#include <fcntl.h>

#include "vtkMultiProcessController.h"
#include "vtkTimerLog.h"

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_sparse_volume_importer.h"

static const std::string LOG_svol_rvol_prefix = "Sparse volume importer: ";

//-------------------------------------------------------------------------------------------------
inline nv::index::Sparse_volume_voxel_format match_volume_format(const std::string& fmt_string)
{
  if (fmt_string == "char")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT8;
  }
  else if (fmt_string == "unsigned char")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT8;
  }
  else if (fmt_string == "unsigned short")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT16;
  }
  else if (fmt_string == "short")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT16;
  }
  else if (fmt_string == "float" || fmt_string == "double")
  {
    return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32;
  }

  return nv::index::SPARSE_VOLUME_VOXEL_FORMAT_COUNT; // invalid format
}

//-------------------------------------------------------------------------------------------------
inline mi::Size volume_format_size(const nv::index::Sparse_volume_voxel_format fmt)
{
  switch (fmt)
  {
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT8:
      return sizeof(mi::Uint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT8:
      return sizeof(mi::Sint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT16:
      return sizeof(mi::Uint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT16:
      return sizeof(mi::Sint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32:
      return sizeof(mi::Float32);
    default:
      return 0;
  }
}

//-------------------------------------------------------------------------------------------------
vtknvindex_import_bricks::vtknvindex_import_bricks(
  const nv::index::ISparse_volume_subset_data_descriptor* subset_data_descriptor,
  nv::index::ISparse_volume_subset* volume_subset, mi::Uint8* brick_storage, mi::Size vol_fmt_size,
  mi::Sint32 border_size, const vtknvindex::util::Bbox3i& read_bounds)
  : m_subset_data_descriptor(subset_data_descriptor)
  , m_volume_subset(volume_subset)
  , m_app_subdivision(brick_storage)
  , m_vol_fmt_size(vol_fmt_size)
  , m_border_size(border_size)
{
  m_nb_bricks = subset_data_descriptor->get_subset_number_of_data_bricks();
  m_nb_fragments = vtknvindex_sysinfo::get_sysinfo()->get_number_logical_cpu();

  // if number of cpus could not be determinated, used default fragments.
  if (m_nb_fragments == 0)
  {
    const mi::Size DEFAULT_NB_FRAGMENTS = 8;
    m_nb_fragments = DEFAULT_NB_FRAGMENTS;
  }

  if (m_nb_fragments > m_nb_bricks)
    m_nb_fragments = m_nb_bricks;

  m_read_bounds = read_bounds;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_import_bricks::execute_fragment(
  mi::neuraylib::IDice_transaction* /*dice_transaction*/, mi::Size index, mi::Size /*count*/,
  const mi::neuraylib::IJob_execution_context* /*context*/)
{
  using namespace nv::index;
  using namespace vtknvindex::util;

  const mi::Uint32 svol_attrib_index_0 = 0u;
  mi::Size nb_bricks_per_index = m_nb_bricks / m_nb_fragments;
  if (m_nb_bricks % m_nb_fragments > 0)
    nb_bricks_per_index++;

  mi::Uint32 min_brick_idx = static_cast<mi::Uint32>(index * nb_bricks_per_index);
  mi::Uint32 max_brick_idx = static_cast<mi::Uint32>((index + 1) * nb_bricks_per_index);
  if (max_brick_idx > m_nb_bricks)
    max_brick_idx = m_nb_bricks;

  for (mi::Uint32 brick_idx = min_brick_idx; brick_idx < max_brick_idx; brick_idx++)
  {
    const ISparse_volume_subset_data_descriptor::Data_brick_info brick_info =
      m_subset_data_descriptor->get_subset_data_brick_info(brick_idx);
    const ISparse_volume_subset::Data_brick_buffer_info brick_data_info =
      m_volume_subset->access_brick_data_buffer(brick_idx, svol_attrib_index_0);

    mi::Uint8* svol_brick_data_raw = reinterpret_cast<mi::Uint8*>(brick_data_info.data);
    if (svol_brick_data_raw == NULL)
    {
      ERROR_LOG << LOG_svol_rvol_prefix
                << "Error accessing brick data pointer, brick id: " << brick_idx;
      return;
    }

    Bbox3i pv_volume_bounds = m_subset_data_descriptor->get_dataset_lod_level_box(0);
    const Vec3i read_dims = m_read_bounds.extent();

    const Vec3i brick_pos = brick_info.brick_position;
    const Vec3u brick_dim = m_subset_data_descriptor->get_subset_data_brick_dimensions();
    Bbox3i brick_bounds(brick_pos, brick_pos + static_cast<Vec3i>(brick_dim));

    Bbox3i subdivision_bounds(m_read_bounds);
    for (mi::Uint32 i = 0; i < 3; ++i)
    {
      if (subdivision_bounds.min[i] == pv_volume_bounds.min[i])
        subdivision_bounds.min[i] -= m_border_size;

      if (subdivision_bounds.max[i] == pv_volume_bounds.max[i])
        subdivision_bounds.max[i] += m_border_size;
    }

    subdivision_bounds.max += Vec3i(1);

    brick_bounds.min =
      mi::math::clamp(brick_bounds.min, subdivision_bounds.min, subdivision_bounds.max);

    brick_bounds.max =
      mi::math::clamp(brick_bounds.max, subdivision_bounds.min, subdivision_bounds.max);

    brick_bounds.min -= m_read_bounds.min;
    brick_bounds.max -= m_read_bounds.min;

    const Vec3i dest_delta(brick_pos - m_read_bounds.min);

    for (mi::Sint32 z = brick_bounds.min.z; z < brick_bounds.max.z; ++z)
    {
      const mi::Sint32 zz = mi::math::clamp(z, 0, read_dims.z - 1);
      const mi::Size dst_off_z =
        static_cast<mi::Size>(z - dest_delta.z) * brick_dim.x * brick_dim.y;
      const mi::Size src_off_z = static_cast<mi::Size>(zz) * read_dims.x * read_dims.y;

      for (mi::Sint32 y = brick_bounds.min.y; y < brick_bounds.max.y; ++y)
      {
        const mi::Sint32 yy = mi::math::clamp(y, 0, read_dims.y - 1);
        const mi::Size dst_off_yz =
          static_cast<mi::Size>(y - dest_delta.y) * brick_dim.x + dst_off_z;
        const mi::Size src_off_yz = static_cast<mi::Size>(yy) * read_dims.x + src_off_z;

        mi::Sint32 xmin = mi::math::clamp(brick_bounds.min.x, 0, read_dims.x);
        mi::Sint32 xmax = mi::math::clamp(brick_bounds.max.x, 0, read_dims.x);

        // Read interior voxels
        if (xmax > xmin)
        {
          // destination offset
          const mi::Size dst_off = static_cast<mi::Size>(xmin - dest_delta.x) + dst_off_yz;

          // source offset
          const mi::Size src_off = static_cast<mi::Size>(xmin) + src_off_yz;

          memcpy(svol_brick_data_raw + dst_off * m_vol_fmt_size,
            m_app_subdivision + src_off * m_vol_fmt_size,
            m_vol_fmt_size * static_cast<mi::Size>(xmax - xmin));
        }

        // Read lower bound when x < 0
        for (mi::Sint32 x = brick_bounds.min.x; x < 0; ++x)
        {
          // destination offset
          const mi::Size dst_off = static_cast<mi::Size>(x - dest_delta.x) + dst_off_yz;

          // source offset
          const mi::Size src_off = src_off_yz;

          memcpy(svol_brick_data_raw + dst_off * m_vol_fmt_size,
            m_app_subdivision + src_off * m_vol_fmt_size, m_vol_fmt_size);
        }

        // Read upper bound when x >= read_dims.x
        for (mi::Sint32 x = read_dims.x; x < brick_bounds.max.x; ++x)
        {
          // destination offset
          const mi::Size dst_off = static_cast<mi::Size>(x - dest_delta.x) + dst_off_yz;

          // source offset
          const mi::Size src_off = static_cast<mi::Size>(read_dims.x - 1) + src_off_yz;

          memcpy(svol_brick_data_raw + dst_off * m_vol_fmt_size,
            m_app_subdivision + src_off * m_vol_fmt_size, m_vol_fmt_size);
        }
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------
mi::Size vtknvindex_import_bricks::get_nb_fragments() const
{
  return m_nb_fragments;
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sparse_volume_importer::vtknvindex_sparse_volume_importer(
  const mi::math::Vector_struct<mi::Uint32, 3>& volume_size, const mi::Sint32& border_size,
  const std::string& scalar_type) //,
  : m_border_size(border_size),
    m_volume_size(volume_size),
    m_scalar_type(scalar_type)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sparse_volume_importer::vtknvindex_sparse_volume_importer()
  : m_border_size(2)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_sparse_volume_importer::~vtknvindex_sparse_volume_importer()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
// Give an estimate of the size in bytes of this subcube.
mi::Size vtknvindex_sparse_volume_importer::estimate(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
  mi::neuraylib::IDice_transaction* /*dice_transaction*/) const
{
  const mi::Size dx = bounding_box.max.x - bounding_box.min.x;
  const mi::Size dy = bounding_box.max.y - bounding_box.min.y;
  const mi::Size dz = bounding_box.max.z - bounding_box.min.z;
  const mi::Size volume_brick_size = dx * dy * dz;

  if (m_scalar_type == "char" || m_scalar_type == "unsigned char")
    return volume_brick_size;
  if (m_scalar_type == "short" || m_scalar_type == "unsigned short")
    return volume_brick_size * sizeof(mi::Uint16);
  if (m_scalar_type == "float")
    return volume_brick_size * sizeof(mi::Float32);

  // Double scalar data is converted to float berfore passed to IndeX
  if (m_scalar_type == "double")
    return volume_brick_size * sizeof(mi::Float32);

  ERROR_LOG << "Failed to give an estimate for the unsupported scalar_type: " << m_scalar_type
            << ".";
  return 0;
}

//-------------------------------------------------------------------------------------------------
nv::index::IDistributed_data_subset* vtknvindex_sparse_volume_importer::create(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box, mi::Uint32 time_step,
  nv::index::IData_subset_factory* factory,
  mi::neuraylib::IDice_transaction* dice_transaction) const
{
  using namespace nv::index;
  using namespace vtknvindex::util;
  using mi::base::Handle;

  // Setup the attribute-set descriptor
  Handle<ISparse_volume_attribute_set_descriptor> svol_attrib_set_desc(
    factory->create_attribute_set_descriptor<ISparse_volume_attribute_set_descriptor>());
  if (!svol_attrib_set_desc.is_valid_interface())
  {
    ERROR_LOG << LOG_svol_rvol_prefix
              << "Unable to create a sparse-volume attribute-set descriptor.";
    return NULL;
  }

  const mi::Uint32 svol_attrib_index_0 = 0u;
  ISparse_volume_attribute_set_descriptor::Attribute_parameters svol_attrib_param_0;
  svol_attrib_param_0.format = match_volume_format(m_scalar_type);

  if (svol_attrib_param_0.format == nv::index::SPARSE_VOLUME_VOXEL_FORMAT_COUNT)
  {
    ERROR_LOG << LOG_svol_rvol_prefix << "Invalid volume format '" << m_scalar_type << "'.";
    return NULL;
  }

  svol_attrib_set_desc->setup_attribute(svol_attrib_index_0, svol_attrib_param_0);

  // Create sparse volume data subset
  Handle<ISparse_volume_subset> svol_data_subset(
    factory->create_data_subset<ISparse_volume_subset>(svol_attrib_set_desc.get()));

  if (!svol_data_subset.is_valid_interface())
  {
    ERROR_LOG << LOG_svol_rvol_prefix << "Unable to create a sparse-volume data-subset.";
    return NULL;
  }

  const nv::index::Sparse_volume_voxel_format vol_fmt = svol_attrib_param_0.format;
  const mi::Size vol_fmt_size = volume_format_size(vol_fmt);

  // Input the required data-bricks into the subset
  Handle<const ISparse_volume_subset_data_descriptor> svol_subset_desc(
    svol_data_subset->get_subset_data_descriptor());

  mi::math::Bbox<mi::Float32, 3> request_bbox = svol_subset_desc->get_subregion_scene_space();

  // ERROR_LOG << "DBBOX: " << request_bbox << ", QBBOX: " << bounding_box;

  // Fetch shared memory details from host properties
  std::string shm_memory_name;
  mi::math::Bbox<mi::Float32, 3> shm_bbox_flt;
  mi::Uint64 shmsize = 0;
  void* pv_subdivision_ptr = NULL;

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  mi::Sint32 rankid = controller ? controller->GetLocalProcessId() : 0;

  if (!m_cluster_properties->get_host_properties(rankid)->get_shminfo(
        request_bbox, shm_memory_name, shm_bbox_flt, shmsize, &pv_subdivision_ptr, time_step))
  {
    ERROR_LOG << "Failed to get shared memory information in regular volume importer for rank: "
              << rankid << ".";
    return 0;
  }

  const Bbox3i shm_bbox(static_cast<mi::Sint32>(shm_bbox_flt.min.x),
    static_cast<mi::Sint32>(shm_bbox_flt.min.y), static_cast<mi::Sint32>(shm_bbox_flt.min.z),
    static_cast<mi::Sint32>(shm_bbox_flt.max.x), static_cast<mi::Sint32>(shm_bbox_flt.max.y),
    static_cast<mi::Sint32>(shm_bbox_flt.max.z));

  if (shm_memory_name.empty() || shm_bbox.empty())
  {
    ERROR_LOG << "Failed to open shared memory shmname: " << shm_memory_name
              << " with bbox: " << shm_bbox << ".";
    return 0;
  }

  INFO_LOG << "Using shared memory: " << shm_memory_name << " box " << shm_bbox
           << " from rank: " << rankid << ".";
  INFO_LOG << "Bounding box requested by NVIDIA IndeX: " << bounding_box;
  //<< " size: " << mi::math::Vector<mi::Sint32, 3>(dx, dy, dz) << ".";

  // const mi::Size vol_mem_size =
  //  static_cast<mi::Size>(read_dims.x) * read_dims.y * read_dims.z * vol_fmt_size;

  // INFO_LOG << LOG_svol_rvol_prefix << "Reading '" << shm_memory_name
  // << "', bounds: " << shm_bbox << ", format: " << vol_fmt << " [0x" << std::hex
  // << vol_fmt << "]...";

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

  svol_data_subset->retain();

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

  return svol_data_subset.get();
}

//-------------------------------------------------------------------------------------------------
nv::index::IDistributed_data_subset* vtknvindex_sparse_volume_importer::create(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
  nv::index::IData_subset_factory* factory,
  mi::neuraylib::IDice_transaction* dice_transaction) const
{
  return create(bounding_box, 0u, factory, dice_transaction);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_sparse_volume_importer::set_cluster_properties(
  vtknvindex_cluster_properties* cluster_properties)
{
  m_cluster_properties = cluster_properties;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_sparse_volume_importer::serialize(mi::neuraylib::ISerializer* serializer) const
{
  mi::Uint32 scalar_typename_size = mi::Uint32(m_scalar_type.size());
  serializer->write(&scalar_typename_size, 1);
  serializer->write(
    reinterpret_cast<const mi::Uint8*>(m_scalar_type.c_str()), scalar_typename_size);

  serializer->write(&m_volume_size.x, 3);
  serializer->write(&m_border_size, 1);

  m_cluster_properties->serialize(serializer);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_sparse_volume_importer::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  mi::Uint32 scalar_typename_size = 0;
  deserializer->read(&scalar_typename_size, 1);
  m_scalar_type.resize(scalar_typename_size);
  deserializer->read(reinterpret_cast<mi::Uint8*>(&m_scalar_type[0]), scalar_typename_size);

  deserializer->read(&m_volume_size.x, 3);
  deserializer->read(&m_border_size, 1);

  m_cluster_properties = new vtknvindex_cluster_properties();
  m_cluster_properties->deserialize(deserializer);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_sparse_volume_importer::get_references(mi::neuraylib::ITag_set* /*result*/) const
{
  // empty
}

//-------------------------------------------------------------------------------------------------
mi::base::Uuid vtknvindex_sparse_volume_importer::subset_id() const
{
  return nv::index::ISparse_volume_subset::IID();
}
