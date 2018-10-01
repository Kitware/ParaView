/* Copyright 2018 NVIDIA Corporation. All rights reserved.
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

#include <nv/index/iregular_volume.h>
#include <nv/index/iregular_volume_brick.h>

#include "vtkMultiProcessController.h"

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_utilities.h"
#include "vtknvindex_volume_importer.h"

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_importer::vtknvindex_volume_importer(
  const mi::math::Vector_struct<mi::Uint32, 3>& volume_size, const mi::Sint32& border_size,
  const std::string& scalar_type) //,
  : m_border_size(border_size),
    m_volume_size(volume_size),
    m_scalar_type(scalar_type)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_importer::vtknvindex_volume_importer()
  : m_border_size(2)
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_importer::~vtknvindex_volume_importer()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
// Give an estimate of the size in bytes of this subcube.
mi::Size vtknvindex_volume_importer::estimate(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
  mi::neuraylib::IDice_transaction* /*dice_transaction*/) const
{
  const mi::Size dx = bounding_box.max.x - bounding_box.min.x;
  const mi::Size dy = bounding_box.max.y - bounding_box.min.y;
  const mi::Size dz = bounding_box.max.z - bounding_box.min.z;
  const mi::Size volume_brick_size = dx * dy * dz;

  if (m_scalar_type == "unsigned char")
    return volume_brick_size;
  if (m_scalar_type == "unsigned short")
    return volume_brick_size * sizeof(mi::Uint16);
  if (m_scalar_type == "float" || m_scalar_type == "double")
    return volume_brick_size * sizeof(mi::Float32);

  ERROR_LOG << "Failed to give an estimate for the unsupported scalar_type: " << m_scalar_type
            << ".";
  return 0;
}

void vtknvindex_volume_importer::set_cluster_properties(
  vtknvindex_cluster_properties* cluster_properties)
{
  m_cluster_properties = cluster_properties;
}

//-------------------------------------------------------------------------------------------------
template <typename T>
bool resolve_voxel_type(T* shmem_volume, T* voxel_data_storage,
  const mi::math::Bbox<mi::Sint32, 3>& bounds, const mi::math::Bbox<mi::Sint32, 3>& shmbbox,
  mi::neuraylib::IDice_transaction* /*dice_transaction*/, bool zyx_to_xyz)
{
  if (voxel_data_storage == NULL)
  {
    ERROR_LOG << "Unable to generate voxel storage in the regular volume importer.";
    return false;
  }

  if (shmem_volume == NULL)
  {
    ERROR_LOG << "Invalid shared memory pointer in the regular volume importer.";
    return false;
  }

  // The size for volumetric data brick that needs to be loaded.
  mi::Sint64 dx = bounds.max.x - bounds.min.x;
  mi::Sint64 dy = bounds.max.y - bounds.min.y;
  mi::Sint64 dz = bounds.max.z - bounds.min.z;

  const mi::math::Vector<mi::Sint64, 3> vtkvolume_size((shmbbox.max.x - shmbbox.min.x),
    (shmbbox.max.y - shmbbox.min.y), (shmbbox.max.z - shmbbox.min.z));

  mi::Uint64 X_DST, Y_DST, Z_DST;
  mi::Sint64 min_x, min_y, min_z, max_x, max_y, max_z;

  mi::Sint64 vsx_vsy = vtkvolume_size.x * vtkvolume_size.y;

  if (shmbbox.min.x > bounds.min.x)
  {
    X_DST = shmbbox.min.x - bounds.min.x;
    min_x = shmbbox.min.x;
  }
  else
  {
    X_DST = 0;
    min_x = bounds.min.x;
  }

  if (shmbbox.min.y > bounds.min.y)
  {
    Y_DST = shmbbox.min.y - bounds.min.y;
    min_y = shmbbox.min.y;
  }
  else
  {
    Y_DST = 0;
    min_y = bounds.min.y;
  }

  if (shmbbox.min.z > bounds.min.z)
  {
    Z_DST = shmbbox.min.z - bounds.min.z;
    min_z = shmbbox.min.z;
  }
  else
  {
    Z_DST = 0;
    min_z = bounds.min.z;
  }

  max_x = (shmbbox.max.x < bounds.max.x) ? shmbbox.max.x : bounds.max.x;
  max_y = (shmbbox.max.y < bounds.max.y) ? shmbbox.max.y : bounds.max.y;
  max_z = (shmbbox.max.z < bounds.max.z) ? shmbbox.max.z : bounds.max.z;

  // Transform volume data from ParaView zyx format to IndeX xyz format.
  if (zyx_to_xyz)
  {
    mi::Uint64 x_dst = X_DST;
    for (mi::Sint64 x = min_x; x < max_x; ++x, ++x_dst)
    {
      mi::Uint64 y_dst = Y_DST;
      for (mi::Sint64 y = min_y; y < max_y; ++y, ++y_dst)
      {
        mi::Uint64 z_dst = Z_DST;

        T* storage_ptr = voxel_data_storage + (dz * dy * x_dst + dz * y_dst + z_dst);

        T* shm_ptr = shmem_volume + (x - shmbbox.min.x + (y - shmbbox.min.y) * vtkvolume_size.x +
                                      (min_z - shmbbox.min.z) * vsx_vsy);

        for (mi::Sint64 z = min_z; z < max_z; ++z)
        {
          *storage_ptr = *shm_ptr;
          storage_ptr++;
          shm_ptr += vsx_vsy;
        }
      }
    }
  }
  else // Volume is already in xyz format.
  {
    mi::Uint64 x_dst = X_DST;
    for (mi::Sint64 x = min_x; x < max_x; ++x, ++x_dst)
    {
      mi::Uint64 y_dst = Y_DST;
      for (mi::Sint64 y = min_y; y < max_y; ++y, ++y_dst)
      {
        mi::Uint64 z_dst = Z_DST;

        mi::Uint64 write_idx = static_cast<mi::Uint64>(dz * dy * x_dst + dz * y_dst + z_dst);
        mi::Uint64 read_idx =
          static_cast<mi::Uint64>((x - shmbbox.min.x) * vtkvolume_size.z * vtkvolume_size.y +
            (y - shmbbox.min.y) * vtkvolume_size.z + (min_z - shmbbox.min.z));

        memcpy(
          voxel_data_storage + write_idx, shmem_volume + read_idx, (max_z - min_z) * sizeof(T));
      }
    }
  }

  //
  // Duplicate voxels at the dataset boundary.
  //

  // Calculate position of the voxels at the boundary of the current brick in memory.
  // These voxels will be duplicated towards the border.
  mi::math::Vector<mi::Sint32, 3> boundary_min = -bounds.min + shmbbox.min;

  const mi::math::Bbox<mi::Sint32, 3> boundary_dst(boundary_min,
    mi::math::Vector<mi::Sint32, 3>(shmbbox.max) - bounds.min - mi::math::Vector<mi::Sint32, 3>(1));

  // Duplicate voxels at the minimum z boundary of the dataset (if that is part of the current
  // brick, i.e., the min component of the bounding box is zero).
  for (mi::Sint64 z = 0; z < -bounds.min.z; ++z)
  {
    for (mi::Sint64 x = 0; x < dx; ++x)
    {
      T* vdt_dest_ptr = voxel_data_storage + (dz * dy * x + z);
      T* vdt_src_ptr = voxel_data_storage + (dz * dy * x + boundary_dst.min.z);

      for (mi::Sint64 y = 0; y < dy; ++y)
      {
        *vdt_dest_ptr = *vdt_src_ptr;
        vdt_src_ptr += dz;
        vdt_dest_ptr += dz;
      }
    }
  }

  // Duplicate voxels at the maximum z boundary of the dataset (if that is part of the current
  // brick, i.e., the max component of the bounding box is larger than the dataset).
  for (mi::Sint64 z = shmbbox.max.z; z < bounds.max.z; ++z)
  {
    for (mi::Sint64 x = 0; x < dx; ++x)
    {
      T* vdt_dest_ptr = voxel_data_storage + (dz * dy * x + (z - bounds.min.z));
      T* vdt_src_ptr = voxel_data_storage + (dz * dy * x + boundary_dst.max.z);

      for (mi::Sint64 y = 0; y < dy; ++y)
      {
        *vdt_dest_ptr = *vdt_src_ptr;
        vdt_src_ptr += dz;
        vdt_dest_ptr += dz;
      }
    }
  }

  // Duplicate min y.
  for (mi::Sint64 y = 0; y < -bounds.min.y; ++y)
  {
    for (mi::Sint64 x = 0; x < dx; ++x)
    {
      T* vdt_dest_ptr = voxel_data_storage + (dz * dy * x + dz * y);
      T* vdt_src_ptr = voxel_data_storage + (dz * dy * x + dz * boundary_dst.min.y);

      memcpy(vdt_dest_ptr, vdt_src_ptr, dz * sizeof(T));
    }
  }

  // Duplicate max y.
  for (mi::Sint64 y = shmbbox.max.y; y < bounds.max.y; ++y)
  {
    for (mi::Sint64 x = 0; x < dx; ++x)
    {
      T* vdt_dest_ptr = voxel_data_storage + (dz * dy * x + dz * (y - bounds.min.y));
      T* vdt_src_ptr = voxel_data_storage + (dz * dy * x + dz * boundary_dst.max.y);

      memcpy(vdt_dest_ptr, vdt_src_ptr, dz * sizeof(T));
    }
  }

  // Duplicate min x.
  for (mi::Sint64 x = 0; x < -bounds.min.x; ++x)
  {
    memcpy(&voxel_data_storage[dz * dy * x], &voxel_data_storage[dz * dy * boundary_dst.min.x],
      sizeof(T) * dz * dy);
  }

  // Duplicate max x.
  for (mi::Sint64 x = shmbbox.max.x; x < bounds.max.x; ++x)
  {
    memcpy(&voxel_data_storage[dz * dy * (x - bounds.min.x)],
      &voxel_data_storage[dz * dy * boundary_dst.max.x], sizeof(T) * dz * dy);
  }

  return true;
}

//-------------------------------------------------------------------------------------------------
nv::index::IDistributed_data_subset* vtknvindex_volume_importer::create(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
  nv::index::IData_subset_factory* factory,
  mi::neuraylib::IDice_transaction* dice_transaction) const
{
  return create(bounding_box, 0, factory, dice_transaction);
}

//-------------------------------------------------------------------------------------------------
nv::index::IDistributed_data_subset* vtknvindex_volume_importer::create(
  const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box, mi::Uint32 time_step,
  nv::index::IData_subset_factory* factory,
  mi::neuraylib::IDice_transaction* dice_transaction) const
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  mi::Sint32 rankid = controller ? controller->GetLocalProcessId() : 0;

  const mi::math::Bbox<mi::Sint32, 3> bounds = bounding_box;
  mi::Sint64 dx = bounds.max.x - bounds.min.x;
  mi::Sint64 dy = bounds.max.y - bounds.min.y;
  mi::Sint64 dz = bounds.max.z - bounds.min.z;

  // Calculate bounds without the border.
  // Because VTK bbox written into the shared memory has no border.
  mi::math::Bbox<mi::Float32, 3> bounds_no_border;
  bounds_no_border.min.x = bounds.min.x + m_border_size;
  bounds_no_border.min.y = bounds.min.y + m_border_size;
  bounds_no_border.min.z = bounds.min.z + m_border_size;
  bounds_no_border.max.x = bounds.max.x - m_border_size;
  bounds_no_border.max.y = bounds.max.y - m_border_size;
  bounds_no_border.max.z = bounds.max.z - m_border_size;

  // Fetch shared memory details from host properties
  std::string shm_memory_name;
  mi::math::Bbox<mi::Float32, 3> shmbbox_flt;
  mi::math::Bbox<mi::Sint32, 3> shmbbox;
  mi::Uint64 shmsize = 0;
  void* raw_mem_pointer = NULL;

  m_cluster_properties->get_host_properties(rankid)->free_shm_pointer(time_step - 1);

  if (!m_cluster_properties->get_host_properties(rankid)->get_shminfo(
        bounds_no_border, shm_memory_name, shmbbox_flt, shmsize, &raw_mem_pointer, time_step))
  {
    ERROR_LOG << "Failed to get shared memory information in regular volume importer for rank: "
              << rankid << ".";
    return 0;
  }

  shmbbox.min.x = static_cast<mi::Sint32>(shmbbox_flt.min.x);
  shmbbox.min.y = static_cast<mi::Sint32>(shmbbox_flt.min.y);
  shmbbox.min.z = static_cast<mi::Sint32>(shmbbox_flt.min.z);
  shmbbox.max.x = static_cast<mi::Sint32>(shmbbox_flt.max.x);
  shmbbox.max.y = static_cast<mi::Sint32>(shmbbox_flt.max.y);
  shmbbox.max.z = static_cast<mi::Sint32>(shmbbox_flt.max.z);

  if (shm_memory_name.empty() || shmbbox.empty())
  {
    ERROR_LOG << "Failed to open shared memory shmname: " << shm_memory_name
              << " with bbox: " << shmbbox << ".";
    return 0;
  }

  INFO_LOG << "Using shared memory: " << shm_memory_name << " box " << shmbbox
           << " from rank: " << rankid << ".";
  INFO_LOG << "Bounding box requested by NVIDIA IndeX: " << bounding_box
           << " size: " << mi::math::Vector<mi::Sint32, 3>(dx, dy, dz) << ".";

  nv::index::IDistributed_data_subset* data_subset_ret = 0;

  // get volume data from shared memory for this bounding_box
  if (m_scalar_type == "unsigned char")
  {
    mi::Uint8* shmem_volume = raw_mem_pointer
      ? reinterpret_cast<mi::Uint8*>(raw_mem_pointer)
      : vtknvindex::util::get_vol_shm<mi::Uint8>(shm_memory_name, shmsize);

    // Create brick (must be derived from IRegular_volume_brick_data)
    // and return the database element to the caller. The caller keeps ownership.
    mi::base::Handle<nv::index::IRegular_volume_brick_uint8> volume_brick(
      factory->create_data_subset<nv::index::IRegular_volume_brick_uint8>());

    if (!volume_brick.is_valid_interface())
    {
      ERROR_LOG << "Unable to create a volume brick : " << m_scalar_type << ".";
    }
    else
    {
      // Allocate voxel data storage via volume_brick.
      mi::Uint8* voxel_data_storage = volume_brick->generate_voxel_storage(bounding_box);

      if (resolve_voxel_type<mi::Uint8>(shmem_volume, voxel_data_storage, bounds, shmbbox,
            dice_transaction, raw_mem_pointer != NULL))
      {
        m_cluster_properties->get_host_properties(rankid)->mark_shm_used(
          shm_memory_name, shmem_volume, shmsize);

        volume_brick->retain();
        data_subset_ret = volume_brick.get();
      }
    }

    // free memory space linked to shared memory
    vtknvindex::util::unmap_shm(shmem_volume, shmsize);
  }
  else if (m_scalar_type == "unsigned short")
  {
    mi::Uint16* shmem_volume = raw_mem_pointer
      ? reinterpret_cast<mi::Uint16*>(raw_mem_pointer)
      : vtknvindex::util::get_vol_shm<mi::Uint16>(shm_memory_name, shmsize);

    // Create brick (must be derived from IRegular_volume_brick_data)
    // and return the database element to the caller. The caller keeps ownership.
    mi::base::Handle<nv::index::IRegular_volume_brick_uint16> volume_brick(
      factory->create_data_subset<nv::index::IRegular_volume_brick_uint16>());

    if (!volume_brick.is_valid_interface())
    {
      ERROR_LOG << "Unable to create a volume brick : " << m_scalar_type << ".";
    }
    else
    {
      // Allocate voxel data storage via volume_brick.
      mi::Uint16* voxel_data_storage = volume_brick->generate_voxel_storage(bounding_box);

      if (resolve_voxel_type<mi::Uint16>(shmem_volume, voxel_data_storage, bounds, shmbbox,
            dice_transaction, raw_mem_pointer != NULL))
      {
        m_cluster_properties->get_host_properties(rankid)->mark_shm_used(
          shm_memory_name, shmem_volume, shmsize);
        volume_brick->retain();
        data_subset_ret = volume_brick.get();
      }
    }

    // free memory space linked to shared memory
    vtknvindex::util::unmap_shm(shmem_volume, shmsize);
  }
  else if (m_scalar_type == "float")
  {
    mi::Float32* shmem_volume = raw_mem_pointer
      ? reinterpret_cast<mi::Float32*>(raw_mem_pointer)
      : vtknvindex::util::get_vol_shm<mi::Float32>(shm_memory_name, shmsize);

    // Create brick (must be derived from IRegular_volume_brick_data)
    // and return the database element to the caller. The caller keeps ownership.
    mi::base::Handle<nv::index::IRegular_volume_brick_float32> volume_brick(
      factory->create_data_subset<nv::index::IRegular_volume_brick_float32>());

    if (!volume_brick.is_valid_interface())
    {
      ERROR_LOG << "Unable to create a volume brick : " << m_scalar_type << ".";
    }
    else
    {
      // Allocate voxel data storage via volume_brick.
      mi::Float32* voxel_data_storage = volume_brick->generate_voxel_storage(bounding_box);

      if (resolve_voxel_type<mi::Float32>(shmem_volume, voxel_data_storage, bounds, shmbbox,
            dice_transaction, raw_mem_pointer != NULL))
      {
        m_cluster_properties->get_host_properties(rankid)->mark_shm_used(
          shm_memory_name, shmem_volume, shmsize);

        volume_brick->retain();
        data_subset_ret = volume_brick.get();
      }
    }

    // free memory space linked to shared memory
    vtknvindex::util::unmap_shm(shmem_volume, shmsize);
  }
  else if (m_scalar_type == "double")
  {
    mi::Float64* shmem_volume = raw_mem_pointer
      ? reinterpret_cast<mi::Float64*>(raw_mem_pointer)
      : vtknvindex::util::get_vol_shm<mi::Float64>(shm_memory_name, shmsize);

    mi::Uint64 nb_scalars = shmsize / sizeof(mi::Float64);

    mi::Float32* shmem_volume_flt = new mi::Float32[nb_scalars];
    if (!shmem_volume_flt)
    {
      ERROR_LOG << "Unable to create conversion buffer.";
      return 0;
    }

    for (mi::Uint64 i = 0; i < nb_scalars; ++i)
      shmem_volume_flt[i] = static_cast<mi::Float32>(shmem_volume[i]);

    // Create brick (must be derived from IRegular_volume_brick_data)
    // and return the database element to the caller. The caller keeps ownership.
    mi::base::Handle<nv::index::IRegular_volume_brick_float32> volume_brick(
      factory->create_data_subset<nv::index::IRegular_volume_brick_float32>());

    if (!volume_brick.is_valid_interface())
    {
      ERROR_LOG << "Unable to create a volume brick : " << m_scalar_type << ".";
    }
    else
    {
      // Allocate voxel data storage via volume_brick.
      mi::Float32* voxel_data_storage = volume_brick->generate_voxel_storage(bounding_box);

      if (resolve_voxel_type<mi::Float32>(shmem_volume_flt, voxel_data_storage, bounds, shmbbox,
            dice_transaction, raw_mem_pointer != NULL))
      {
        m_cluster_properties->get_host_properties(rankid)->mark_shm_used(
          shm_memory_name, shmem_volume, shmsize);

        volume_brick->retain();
        data_subset_ret = volume_brick.get();
      }
    }

    delete[] shmem_volume_flt;

    // free memory space linked to shared memory
    vtknvindex::util::unmap_shm(shmem_volume, shmsize);
  }
  else
  {
    // It will fail in the Representation class as well
    ERROR_LOG << "Volume scalar type are not supported by NVIDIA IndeX.";
  }
  return data_subset_ret;
}

//-------------------------------------------------------------------------------------------------
mi::base::Uuid vtknvindex_volume_importer::subset_id() const
{
  if (m_scalar_type == "unsigned char")
    return nv::index::IRegular_volume_brick_uint8::IID();
  else if (m_scalar_type == "unsigned short")
    return nv::index::IRegular_volume_brick_uint16::IID();
  else if (m_scalar_type == "float")
    return nv::index::IRegular_volume_brick_float32::IID();
  else if (m_scalar_type == "double")
    return nv::index::IRegular_volume_brick_float32::IID();

  ERROR_LOG << "Invalid scalar type: " << m_scalar_type << " returning IID for 8-bit";
  return nv::index::IRegular_volume_brick_uint8::IID();
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_importer::serialize(mi::neuraylib::ISerializer* serializer) const
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
void vtknvindex_volume_importer::deserialize(mi::neuraylib::IDeserializer* deserializer)
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
void vtknvindex_volume_importer::get_references(mi::neuraylib::ITag_set* /*result*/) const
{
  // empty
}
