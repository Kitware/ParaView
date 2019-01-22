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

#ifndef vtknvindex_volume_importer_h
#define vtknvindex_volume_importer_h

#include <mi/dice.h>
#include <nv/index/idistributed_data_import_callback.h>

class vtknvindex_cluster_properties;

// Use appropriate voxel storage.
// Based on the data type of the dataset.
template <typename T>
bool resolve_voxel_type(T* shmem_volume, T* voxel_data_storage,
  const mi::math::Bbox<mi::Sint32, 3>& bounds, const mi::math::Bbox<mi::Sint32, 3>& shmbbox,
  mi::neuraylib::IDice_transaction* dice_transaction, bool zyx_to_xyz);

// The class vtknvindex_volume_importer represents a distributed data importer for NVIDIA IndeX
// to load subsets of a regular volume dataset based shared memory.
class vtknvindex_volume_importer
  : public nv::index::Distributed_discrete_data_import_callback<0xecac5c8d, 0x46d6, 0x43a5, 0xab,
      0xa3, 0x4, 0xa0, 0xa4, 0xd2, 0x48, 0xa1>
{
public:
  vtknvindex_volume_importer();
  virtual ~vtknvindex_volume_importer();

  vtknvindex_volume_importer(const mi::math::Vector_struct<mi::Uint32, 3>& volume_size,
    const mi::Sint32& border_size, const std::string& scalar_type);

  // Estimates the volume data size inside the bounding box (in bytes).
  mi::Size estimate(const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;
  using nv::index::Distributed_discrete_data_import_callback<0xecac5c8d, 0x46d6, 0x43a5, 0xab, 0xa3,
    0x4, 0xa0, 0xa4, 0xd2, 0x48, 0xa1>::estimate;

  // The cluster properties triggered by ParaView.
  void set_cluster_properties(vtknvindex_cluster_properties* host_properties);

  // NVIDIA IndeX triggers this callback if time varying data shall be imported.
  nv::index::IDistributed_data_subset* create(const mi::math::Bbox_struct<mi::Sint32, 3>& bbox,
    mi::Uint32 time_step, nv::index::IData_subset_factory* factory,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;

  // Create internal storage of ParaView's subset inside bounding box.
  nv::index::IDistributed_data_subset* create(
    const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
    nv::index::IData_subset_factory* factory,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;
  using nv::index::Distributed_discrete_data_import_callback<0xecac5c8d, 0x46d6, 0x43a5, 0xab, 0xa3,
    0x4, 0xa0, 0xa4, 0xd2, 0x48, 0xa1>::create;

  // DiCE methods.
  void serialize(mi::neuraylib::ISerializer* serializer) const override;
  void deserialize(mi::neuraylib::IDeserializer* deserializer) override;
  virtual void get_references(mi::neuraylib::ITag_set* result) const;
  mi::base::Uuid subset_id() const override;

private:
  //// Use appropriate voxel storage.
  //// Based on the data type of the dataset.
  // template <typename T>
  // bool resolve_voxel_type(T* shmem_volume, T* voxel_data_storage,
  //  const mi::math::Bbox<mi::Sint32, 3>& bounds, const mi::math::Bbox<mi::Sint32, 3>& shmbbox,
  //  mi::neuraylib::IDice_transaction* dice_transaction, bool zyx_to_xyz) const;

  mi::Sint32 m_border_size;                            // subcube border size.
  mi::math::Vector<mi::Uint32, 3> m_volume_size;       // Volume size.
  std::string m_scalar_type;                           // Volume scalar type as string.
  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties.
};

#endif // vtknvindex_volume_importer_h
