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

#ifndef vtknvindex_irregular_volume_importer_h
#define vtknvindex_irregular_volume_importer_h

#include <mi/dice.h>
#include <nv/index/idistributed_data_import_callback.h>

class vtknvindex_cluster_properties;

// The class vtknvindex_volume_importer represents a distributed data importer for NVIDIA IndeX.
// It load ParaView's subsets of a irregular volume dataset based shared memory.
class vtknvindex_irregular_volume_importer
  : public nv::index::Distributed_continuous_data_import_callback<0xa034b89a, 0xdd90, 0x464d, 0x85,
      0x9, 0x5f, 0xef, 0x93, 0xb8, 0x2d, 0x96>
{
public:
  vtknvindex_irregular_volume_importer();
  virtual ~vtknvindex_irregular_volume_importer();

  vtknvindex_irregular_volume_importer(
    const mi::Sint32& border_size, const std::string& scalar_type);

  // Estimates the size (in byte) of the volume data contained in the bounding box.
  mi::Size estimate(const mi::math::Bbox_struct<mi::Float32, 3>& bounding_box,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;
  using nv::index::Distributed_continuous_data_import_callback<0xa034b89a, 0xdd90, 0x464d, 0x85,
    0x9, 0x5f, 0xef, 0x93, 0xb8, 0x2d, 0x96>::estimate;

  // Sets the cluster properties triggered by ParaView.
  void set_cluster_properties(vtknvindex_cluster_properties* host_properties);

  // Create the internal storage of ParaView's subset inside bounding box.
  nv::index::IDistributed_data_subset* create(
    const mi::math::Bbox_struct<mi::Float32, 3>& bounding_box,
    nv::index::IData_subset_factory* factory,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;
  using nv::index::Distributed_continuous_data_import_callback<0xa034b89a, 0xdd90, 0x464d, 0x85,
    0x9, 0x5f, 0xef, 0x93, 0xb8, 0x2d, 0x96>::create;

  // DiCE methods
  void serialize(mi::neuraylib::ISerializer* serializer) const override;
  void deserialize(mi::neuraylib::IDeserializer* deserializer) override;
  virtual void get_references(mi::neuraylib::ITag_set* result) const;
  mi::base::Uuid subset_id() const override;

  mi::Sint32 m_border_size;                            // Subcube border size.
  std::string m_scalar_type;                           // Volume's scalar type as string.
  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties.
};

#endif // vtknvindex_irregular_volume_importer_h
