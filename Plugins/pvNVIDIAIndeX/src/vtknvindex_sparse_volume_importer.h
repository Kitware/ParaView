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

#ifndef vtknvindex_sparse_volume_importer_h
#define vtknvindex_sparse_volume_importer_h

#include <mi/dice.h>
#include <nv/index/idistributed_data_import_callback.h>
#include <nv/index/isparse_volume_subset.h>

#include "vtknvindex_utilities.h"

class vtknvindex_cluster_properties;

// Fragmented job to import in parallel all brick storage pieces
class vtknvindex_import_bricks : public mi::neuraylib::Fragmented_job<0x6538523c, 0xba06, 0x4227,
                                   0xbe, 0xb4, 0x2, 0xdf, 0x9, 0xb6, 0x69, 0xa4>
{

public:
  vtknvindex_import_bricks(
    const nv::index::ISparse_volume_subset_data_descriptor* subset_data_descriptor,
    nv::index::ISparse_volume_subset* volume_subset, mi::Uint8* brick_storage,
    mi::Size vol_fmt_size, mi::Sint32 border_size, const vtknvindex::util::Bbox3i& read_bounds);

  virtual ~vtknvindex_import_bricks() {}

  virtual void execute_fragment(mi::neuraylib::IDice_transaction* dice_transaction, mi::Size index,
    mi::Size count, const mi::neuraylib::IJob_execution_context* context);

  mi::Size get_nb_fragments() const;

private:
  const nv::index::ISparse_volume_subset_data_descriptor* m_subset_data_descriptor;
  nv::index::ISparse_volume_subset* m_volume_subset;
  mi::Uint8* m_app_subdivision;
  mi::Size m_vol_fmt_size;
  mi::Sint32 m_border_size;
  mi::Size m_nb_fragments;
  mi::Size m_nb_bricks;
  vtknvindex::util::Bbox3i m_read_bounds;
};

// The class vtknvindex_sparse_volume_importer represents a distributed data importer for NVIDIA
// IndeX
// to load subsets of a sparse volume dataset based shared memory.
class vtknvindex_sparse_volume_importer
  : public nv::index::Distributed_discrete_data_import_callback<0x5c35b7ce, 0x496c, 0x4342, 0xa3,
      0x5f, 0x9d, 0x85, 0x4, 0xa1, 0x69, 0x7e>
{
public:
  vtknvindex_sparse_volume_importer();

  vtknvindex_sparse_volume_importer(const mi::math::Vector_struct<mi::Uint32, 3>& volume_size,
    const mi::Sint32& border_size, const std::string& scalar_type);

  virtual ~vtknvindex_sparse_volume_importer();

  // Estimates the volume data size inside the bounding box (in bytes).
  mi::Size estimate(const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;

  using nv::index::Distributed_discrete_data_import_callback<0x5c35b7ce, 0x496c, 0x4342, 0xa3, 0x5f,
    0x9d, 0x85, 0x4, 0xa1, 0x69, 0x7e>::estimate;

  // NVIDIA IndeX triggers this callback if time varying data shall be imported.
  nv::index::IDistributed_data_subset* create(
    const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box, mi::Uint32 time_step,
    nv::index::IData_subset_factory* factory,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;

  // Create internal storage of ParaView's subset inside bounding box.
  nv::index::IDistributed_data_subset* create(
    const mi::math::Bbox_struct<mi::Sint32, 3>& bounding_box,
    nv::index::IData_subset_factory* factory,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;

  using nv::index::Distributed_discrete_data_import_callback<0x5c35b7ce, 0x496c, 0x4342, 0xa3, 0x5f,
    0x9d, 0x85, 0x4, 0xa1, 0x69, 0x7e>::create;

  // The cluster properties triggered by ParaView.
  void set_cluster_properties(vtknvindex_cluster_properties* host_properties);

  // DiCE methods.
  void serialize(mi::neuraylib::ISerializer* serializer) const override;
  void deserialize(mi::neuraylib::IDeserializer* deserializer) override;
  virtual void get_references(mi::neuraylib::ITag_set* result) const;
  mi::base::Uuid subset_id() const override;

private:
  mi::Sint32 m_border_size;                            // subcube border size.
  mi::math::Vector<mi::Uint32, 3> m_volume_size;       // Volume size.
  std::string m_scalar_type;                           // Volume scalar type as string.
  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties.
};

#endif // vtknvindex_sparse_volume_importer_h
