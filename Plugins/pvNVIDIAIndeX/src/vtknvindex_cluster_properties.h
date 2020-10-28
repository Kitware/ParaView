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

#ifndef vtknvindex_cluster_properties_h
#define vtknvindex_cluster_properties_h

#include <map>
#include <sstream>

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
  mi::Uint32 num_scalars; // either equal to num_points or num_cells, depending on cell_flag

  void* scalars;
  mi::Sint32 cell_flag; // 0: point scalars, 1: cell scalars, 2: field_scalars; see
                        // vtkAbstractMapper::GetScalars()
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

class vtknvindex_cluster_properties
{
public:
  vtknvindex_cluster_properties(bool use_kdtree);
  ~vtknvindex_cluster_properties();

  // Get general config settings.
  vtknvindex_config_settings* get_config_settings() const;

  // Get ParaView's domain subdivision affinity.
  nv::index::IAffinity_information* get_affinity() const;

  // Creates a copy of ParaView's domain subdivision affinity.
  nv::index::IAffinity_information* copy_affinity() const;

  // Print the affinity information as part of the scene dump.
  void scene_dump_affinity_info(std::ostringstream& s) const;

  // Get kd-tree affinity, or null if it doesn't exists
  vtknvindex_KDTree_affinity* get_affinity_kdtree() const;

  // Get regular (and irregular) volume general properties.
  vtknvindex_regular_volume_properties* get_regular_volume_properties() const;

  // Get global rank id.
  mi::Sint32 rank_id() const;

  // Return host_properties of the host this rank belongs to.
  vtknvindex_host_properties* get_host_properties(const mi::Sint32& rankid) const;

  // Get all shared memory infos that intersect with the given bounding box, searching over all
  // hosts.
  std::vector<vtknvindex_host_properties::shm_info*> get_shminfo_intersect(
    const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 time_step);

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

  // Returns the id that can be passed to get_instance(), or 0 if it was not registered.
  mi::Uint32 get_instance_id() const;

  // Returns a previously registered instance of this class.
  static vtknvindex_cluster_properties* get_instance(mi::Uint32 instance_id);

  // Store whether the associated representation is visible or not.
  void set_visibility(bool visible);

  // Returns whether this instance is active for rendering, i.e. if it is the visible instance with
  // the hightest instance id.
  bool is_active_instance() const;

  // Activate this instance for rendering. Returns false if it was already active.
  bool activate();

  // Prints a warning (once) or info message when multiple instance are visible, explaining that
  // only a single one of them will be rendered.
  void warn_if_multiple_visible_instances(const std::string& active_array_name);

private:
  vtknvindex_cluster_properties(const vtknvindex_cluster_properties&) = delete;
  void operator=(const vtknvindex_cluster_properties&) = delete;

  mi::Uint32 m_instance_id; // Identifier of this instance.
  bool m_visible;           // Visibility of associated representation.

  mi::Sint32 m_rank_id;                                           // Rank id for the host.
  mi::base::Handle<vtknvindex_affinity> m_affinity;               // Affinity for NVIDIA IndeX.
  mi::base::Handle<vtknvindex_KDTree_affinity> m_affinity_kdtree; // kd-tree affinity, optional.

  vtknvindex_config_settings* m_config_settings; // Configuration settings.
  vtknvindex_regular_volume_properties*
    m_regular_vol_properties;                             // Regular/irregular volume properties.
  mi::Uint32 m_num_ranks;                                 // Total number of MPI ranks.
  std::vector<mi::Sint32> m_all_rank_ids;                 // All the MPI rank ids from ParaView.
  std::map<std::string, mi::Uint32> m_hostname_to_hostid; // Host names to host ids.
  std::map<mi::Sint32, mi::Uint32> m_rankid_to_hostid;    // Rank_id to host id.
  std::map<mi::Uint32, vtknvindex_host_properties*> m_hostinfo; // Host_id to host_properties.

  static mi::Uint32 s_active_instance; // Instance that is currently active for rendering.

  static std::map<mi::Uint32, vtknvindex_cluster_properties*>
    s_instances; // All registered instances.
};

#endif
