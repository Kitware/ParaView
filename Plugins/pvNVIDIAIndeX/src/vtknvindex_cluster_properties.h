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

#ifndef vtknvindex_cluster_properties_h
#define vtknvindex_cluster_properties_h

#include <map>

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"

#include "vtknvindex_affinity.h"
#include "vtknvindex_config_settings.h"
#include "vtknvindex_host_properties.h"
#include "vtknvindex_regular_volume_properties.h"
#include "vtknvindex_volumemapper.h"

class vtkUnstructuredGridBase;

// Representing regular volume data.
struct vtknvindex_regular_volume_data
{
  void* scalars;
};

// Representing irregular volume data.
struct vtknvindex_irregular_volume_data
{
  // Subregion info on ParaView's domain subdivision kd-tree.
  mi::Sint32 subregion_id;
  mi::math::Bbox<mi::Float32, 3> subregion_bbox;

  // Only this data is accounted for shared memory size.
  vtkUnstructuredGridBase* pv_unstructured_grid;
  mi::Uint32 num_points;
  mi::Uint32 num_cells;

  void* scalars;
  mi::Float32 max_edge_length2;

  // Get the memory required to allocate this dataset.
  size_t get_memory_size(const std::string& scalar_type);
};

// Represents common dataset parameters.
struct vtknvindex_dataset_parameters
{
  vtknvindex_scene::Volume_type volume_type;
  std::string scalar_type;
  mi::Float32 voxel_range[2];
  mi::Float32 scalar_range[2];
  mi::Float32 bounds[6];
  void* volume_data;
};

// The class vtknvindex_cluster_properties contains several information like volume properties,
// general config settings,
// list of host in the cluster, affinity information.

class vtknvindex_cluster_properties : public mi::neuraylib::Element<0xba7f59f5, 0xffed, 0x467b,
                                        0xa6, 0x5a, 0xd4, 0x9f, 0x50, 0xb7, 0x00, 0x8f>
{
public:
  vtknvindex_cluster_properties();
  ~vtknvindex_cluster_properties();

  // Get general config settings.
  vtknvindex_config_settings* get_config_settings() const;

  // Get ParaView's domain subdivision affinity.
  mi::base::Handle<vtknvindex_affinity> get_affinity() const;

  // Get regular (and irregular) volume general properties.
  vtknvindex_regular_volume_properties* get_regular_volume_properties() const;

  // Get global rank id.
  mi::Sint32 rank_id() const;

  // Return host_properties of the host this rank belongs to.
  vtknvindex_host_properties* get_host_properties(const mi::Sint32& rankid) const;

  // Gather process information used for setting affinity
  // for NVIDIA IndeX.
  bool retrieve_process_configuration(const vtknvindex_dataset_parameters& dataset_parameters);

  // Gather cluster information used for setting affinity
  // for NVIDIA IndeX and writing data into shared memory.
  bool retrieve_cluster_configuration(const vtknvindex_dataset_parameters& dataset_parameters,
    mi::Sint32 current_hostid, bool is_index_rank);

  // Free resources associated to shared memory
  void unlink_shared_memory(bool reset);

  // Print all the cluster details.
  void print_info() const;

  // DiCE database element methods
  mi::neuraylib::IElement* copy() const override;
  void serialize(mi::neuraylib::ISerializer* serializer) const override;
  void deserialize(mi::neuraylib::IDeserializer* deserializer) override;
  const char* get_class_name() const override;
  mi::base::Uuid get_class_id() const override;

  // Build the host list with its rank list assignments.

private:
  vtknvindex_cluster_properties(const vtknvindex_cluster_properties&) = delete;
  void operator=(const vtknvindex_cluster_properties&) = delete;

  mi::Sint32 m_rank_id;                             // Rank id for the host.
  mi::base::Handle<vtknvindex_affinity> m_affinity; // Affinity for NVIDIA IndeX.
  vtknvindex_config_settings* m_config_settings;    // Configuration settings.
  vtknvindex_regular_volume_properties*
    m_regular_vol_properties;                             // Regular/irregular volume properties.
  mi::Uint32 m_num_ranks;                                 // Total number of MPI ranks.
  std::vector<mi::Sint32> m_all_rank_ids;                 // All the MPI rank ids from ParaView.
  std::map<std::string, mi::Uint32> m_hostname_to_hostid; // Host names to host ids.
  std::map<mi::Sint32, mi::Uint32> m_rankid_to_hostid;    // Rank_id to host id.
  std::map<mi::Uint32, vtknvindex_host_properties*> m_hostinfo; // Host_id to host_properties.
};

#endif
