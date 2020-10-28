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

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/SystemInformation.hxx"

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_instance.h"
#include "vtknvindex_utilities.h"

// ------------------------------------------------------------------------------------------------
size_t vtknvindex_irregular_volume_data::get_memory_size(const std::string& scalar_type)
{
  size_t scalar_size;

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
  else
  {
    ERROR_LOG << "The scalar type: " << scalar_type << " is not supported by NVIDIA IndeX.";
    return 0;
  }

  return sizeof(num_points) + sizeof(num_cells) + sizeof(num_scalars) + sizeof(cell_flag) +
    sizeof(mi::math::Vector<mi::Float32, 3>) * num_points +
    sizeof(mi::math::Vector<mi::Uint32, 4>) * num_cells + scalar_size * num_scalars +
    sizeof(max_edge_length2);
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_cluster_properties::s_active_instance = 0;

// ------------------------------------------------------------------------------------------------
std::map<mi::Uint32, vtknvindex_cluster_properties*> vtknvindex_cluster_properties::s_instances;

// ------------------------------------------------------------------------------------------------
vtknvindex_cluster_properties::vtknvindex_cluster_properties(bool use_kdtree)
  : m_rank_id(-1)
  , m_visible(true)
{
  // Register the instance
  static mi::Uint32 instance_counter = 0;
  instance_counter++;
  m_instance_id = instance_counter;
  s_instances[m_instance_id] = this;

  m_affinity = new vtknvindex_affinity();
#ifdef VTKNVINDEX_USE_KDTREE
  if (use_kdtree)
  {
    m_affinity_kdtree = new vtknvindex_KDTree_affinity();
  }
#else
  // prevent compile warning
  (void)use_kdtree;
#endif

  m_config_settings = new vtknvindex_config_settings();
  m_regular_vol_properties = new vtknvindex_regular_volume_properties();
}

// ------------------------------------------------------------------------------------------------
vtknvindex_cluster_properties::~vtknvindex_cluster_properties()
{
  // Unlink shared memory and delete host properties.
  for (auto& shmit : m_hostinfo)
  {
    vtknvindex_host_properties* host_props = shmit.second;
    if (host_props)
    {
      host_props->shm_cleanup(true);
      delete host_props;
    }
  }

  delete m_regular_vol_properties;
  delete m_config_settings;

  if (m_instance_id > 0)
  {
    // Unregister the instance
    s_instances.erase(m_instance_id);
  }
}

// ------------------------------------------------------------------------------------------------
vtknvindex_config_settings* vtknvindex_cluster_properties::get_config_settings() const
{
  return m_config_settings;
}

// ------------------------------------------------------------------------------------------------
vtknvindex_regular_volume_properties* vtknvindex_cluster_properties::get_regular_volume_properties()
  const
{
  return m_regular_vol_properties;
}

// ------------------------------------------------------------------------------------------------
nv::index::IAffinity_information* vtknvindex_cluster_properties::get_affinity() const
{
  if (m_affinity_kdtree && m_affinity_kdtree->get_nb_nodes() > 0)
    return m_affinity_kdtree.get();
  else
    return m_affinity.get();
}

// ------------------------------------------------------------------------------------------------
nv::index::IAffinity_information* vtknvindex_cluster_properties::copy_affinity() const
{
  if (m_affinity_kdtree && m_affinity_kdtree->get_nb_nodes() > 0)
    return m_affinity_kdtree->copy();
  else
    return m_affinity->copy();
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_cluster_properties::scene_dump_affinity_info(std::ostringstream& s) const
{
  if (m_affinity_kdtree)
    return m_affinity_kdtree->scene_dump_affinity_info(s);
  else if (m_affinity)
    return m_affinity->scene_dump_affinity_info(s);
}

// ------------------------------------------------------------------------------------------------
vtknvindex_KDTree_affinity* vtknvindex_cluster_properties::get_affinity_kdtree() const
{
  return m_affinity_kdtree.get();
}

// ------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_cluster_properties::rank_id() const
{
  return m_rank_id;
}

// ------------------------------------------------------------------------------------------------
vtknvindex_host_properties* vtknvindex_cluster_properties::get_host_properties(
  const mi::Sint32& rankid) const
{
  std::map<mi::Sint32, mi::Uint32>::const_iterator rankit = m_rankid_to_hostid.find(rankid);
  if (rankit == m_rankid_to_hostid.end())
    return NULL;

  std::map<mi::Uint32, vtknvindex_host_properties*>::const_iterator shmit =
    m_hostinfo.find(rankit->second);
  if (shmit == m_hostinfo.end())
    return NULL;

  return shmit->second;
}

// ------------------------------------------------------------------------------------------------
std::vector<vtknvindex_host_properties::shm_info*>
vtknvindex_cluster_properties::get_shminfo_intersect(
  const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 time_step)
{
  std::vector<vtknvindex_host_properties::shm_info*> infos_all;

  for (auto& hostinfo : m_hostinfo)
  {
    const auto infos_host = hostinfo.second->get_shminfo_intersect(bbox, time_step);
    infos_all.insert(infos_all.end(), infos_host.begin(), infos_host.end());
  }

  return infos_all;
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_cluster_properties::retrieve_process_configuration(
  const vtknvindex_dataset_parameters& dataset_parameters)
{
  // Volume general properties.
  m_regular_vol_properties->set_scalar_type(dataset_parameters.scalar_type);
  m_regular_vol_properties->set_voxel_range(mi::math::Vector<mi::Float32, 2>(
    dataset_parameters.voxel_range[0], dataset_parameters.voxel_range[1]));
  m_regular_vol_properties->set_scalar_range(mi::math::Vector<mi::Float32, 2>(
    dataset_parameters.scalar_range[0], dataset_parameters.scalar_range[1]));

  m_num_ranks = 1;
  m_rank_id = 0;
  m_all_rank_ids.clear();
  m_all_rank_ids.push_back(0);

  mi::math::Bbox<mi::Sint32, 3> volume_extents;
  m_regular_vol_properties->get_volume_extents(volume_extents);

  // Gather bounds of all the pieces.
  mi::math::Bbox<mi::Float32, 3> current_bbox;

  if (dataset_parameters.volume_type == vtknvindex_scene::VOLUME_TYPE_REGULAR)
  {
    current_bbox.min.x = dataset_parameters.bounds[0];
    current_bbox.min.y = dataset_parameters.bounds[2];
    current_bbox.min.z = dataset_parameters.bounds[4];
    current_bbox.max.x = dataset_parameters.bounds[1];
    current_bbox.max.y = dataset_parameters.bounds[3];
    current_bbox.max.z = dataset_parameters.bounds[5];

    current_bbox.min.x -= volume_extents.min.x;
    current_bbox.min.y -= volume_extents.min.y;
    current_bbox.min.z -= volume_extents.min.z;
    current_bbox.max.x -= (volume_extents.min.x - 1);
    current_bbox.max.y -= (volume_extents.min.y - 1);
    current_bbox.max.z -= (volume_extents.min.z - 1);
  }
  else // VOLUME_TYPE_IRREGULAR
  {
    vtknvindex_irregular_volume_data* volume_data =
      static_cast<vtknvindex_irregular_volume_data*>(dataset_parameters.volume_data);

    current_bbox = volume_data->subregion_bbox;
  }

  vtksys::SystemInformation sys_info;
  std::string host_name = sys_info.GetHostname();
  m_hostname_to_hostid[host_name] = 1;

  // Set affinity information for NVIDIA IndeX.
  m_affinity->reset_affinity();
  m_affinity->add_affinity(current_bbox, 1);

  if (m_affinity_kdtree)
  {
    m_affinity_kdtree->reset_affinity();
    m_affinity_kdtree->add_affinity(current_bbox, 1);
  }

  m_rankid_to_hostid[0] = 1;

  std::map<mi::Uint32, vtknvindex_host_properties*>::iterator shmit = m_hostinfo.find(1);
  if (shmit == m_hostinfo.end())
  {
    vtknvindex_host_properties* host_properties = new vtknvindex_host_properties(1, 0, host_name);
    host_properties->set_rankids(m_all_rank_ids);
    m_hostinfo[1] = host_properties;
  }
  else
  {
    vtknvindex_host_properties* host_properties = shmit->second;
    host_properties->set_rankids(m_all_rank_ids);
  }

  // collecting information for shared memory: bbox, type, size, time step.
  mi::Uint32 nb_time_steps = m_regular_vol_properties->get_nb_time_steps();

  const mi::Sint32 current_rankid = 0;

  for (mi::Uint32 time_step = 0; time_step < nb_time_steps; ++time_step)
  {
    std::stringstream ss;
    ss << "pv_nvindex_shm_instance_" << m_instance_id << "_rank_" << current_rankid << "_timestep_"
       << time_step;

#ifndef _WIN32
    ss << "_" << vtknvindex::util::get_process_user_name();
#endif

    std::map<mi::Uint32, vtknvindex_host_properties*>::iterator shmjt = m_hostinfo.find(1);
    vtknvindex_host_properties* host_properties = shmjt->second;
    mi::Uint64 shm_size = 0;
    std::string scalar_type = dataset_parameters.scalar_type;

    if (dataset_parameters.volume_type == vtknvindex_scene::VOLUME_TYPE_REGULAR)
    {
      vtknvindex_regular_volume_data* volume_data =
        static_cast<vtknvindex_regular_volume_data*>(dataset_parameters.volume_data);

      mi::math::Vector_struct<mi::Uint64, 3> pernode_volume = {
        static_cast<mi::Uint64>(current_bbox.max.x - current_bbox.min.x),
        static_cast<mi::Uint64>(current_bbox.max.y - current_bbox.min.y),
        static_cast<mi::Uint64>(current_bbox.max.z - current_bbox.min.z)
      };
      mi::Uint64 volume_size =
        static_cast<mi::Uint64>(pernode_volume.x) * pernode_volume.y * pernode_volume.z;

      const mi::Size scalar_size =
        vtknvindex_regular_volume_properties::get_scalar_size(scalar_type);
      if (scalar_size > 0)
      {
        shm_size = volume_size * scalar_size;
      }
      else
      {
        ERROR_LOG << "The scalar type: " << scalar_type << " is not supported by NVIDIA IndeX.";
        return false;
      }

      host_properties->set_shminfo(
        time_step, current_rankid, ss.str(), current_bbox, shm_size, volume_data->scalars);
    }
    else // vtknvindex_scene::VOLUME_TYPE_IRREGULAR
    {
      vtknvindex_irregular_volume_data* volume_data =
        static_cast<vtknvindex_irregular_volume_data*>(dataset_parameters.volume_data);

      if (scalar_type == "unsigned char")
        ;
      else if (scalar_type == "unsigned short")
        ;
      else if (scalar_type == "float")
        ;
      else if (scalar_type == "double")
        ;
      else
      {
        ERROR_LOG << "The scalar type: " << scalar_type << " is not supported by NVIDIA IndeX.";
        return false;
      }

      shm_size = volume_data->get_memory_size(scalar_type);

      host_properties->set_shminfo(
        time_step, current_rankid, ss.str(), current_bbox, shm_size, volume_data);
    }
  }

  return true;
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_cluster_properties::retrieve_cluster_configuration(
  const vtknvindex_dataset_parameters& dataset_parameters, mi::Sint32 current_hostid,
  bool is_index_rank)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  m_num_ranks = controller->GetNumberOfProcesses();

  // Gather scalar type and voxel range of the volume.
  {
    m_regular_vol_properties->set_scalar_type(dataset_parameters.scalar_type);

    std::vector<mi::Float32> voxel_range_vec;
    voxel_range_vec.resize(m_num_ranks * 2);
    controller->AllGather(dataset_parameters.voxel_range, &voxel_range_vec[0], 2);

    mi::math::Vector<mi::Float32, 2> final_voxel_range;
    final_voxel_range.x = *std::min_element(voxel_range_vec.begin(), voxel_range_vec.end());
    final_voxel_range.y = *std::max_element(voxel_range_vec.begin(), voxel_range_vec.end());

    m_regular_vol_properties->set_voxel_range(final_voxel_range);

    m_regular_vol_properties->set_scalar_range(mi::math::Vector<mi::Float32, 2>(
      dataset_parameters.scalar_range[0], dataset_parameters.scalar_range[1]));
  }

  if (dataset_parameters.volume_type == vtknvindex_scene::VOLUME_TYPE_IRREGULAR)
  {
    // Reduce to all the longest edge square length.
    mi::Float32 max_edge_lenght_sqr;
    vtknvindex_irregular_volume_data* ivol_data =
      static_cast<vtknvindex_irregular_volume_data*>(dataset_parameters.volume_data);

    controller->AllReduce(
      &ivol_data->max_edge_length2, &max_edge_lenght_sqr, 1, vtkCommunicator::MAX_OP);
    ivol_data->max_edge_length2 = max_edge_lenght_sqr;
  }

  // Gather all the rank ids.
  {
    m_rank_id = controller->GetLocalProcessId();
    m_all_rank_ids.resize(m_num_ranks);
    controller->AllGather(&m_rank_id, &m_all_rank_ids[0], 1);
  }

  // Gather bounds of all the pieces.
  std::vector<mi::Float32> all_rank_extents(6 * m_num_ranks);
  controller->AllGather(dataset_parameters.bounds, all_rank_extents.data(), 6);

  //// Gather local node rank sizes.
  vtknvindex_instance* index_instance = vtknvindex_instance::get();
  mi::Sint32 current_localrank = index_instance->get_cur_local_rank_id();

  std::vector<mi::Sint32> all_localrank_ids;
  all_localrank_ids.resize(m_num_ranks);
  controller->AllGather(&current_localrank, &all_localrank_ids[0], 1);

  // Gather all the host ids, generated by NVIDIA IndeX.
  std::vector<mi::Sint32> all_hostids;
  all_hostids.resize(m_num_ranks);
  controller->AllGather(&current_hostid, &all_hostids[0], 1);

  // Gather all volume data direct pointers
  std::vector<std::intptr_t> all_data_ptrs;
  all_data_ptrs.resize(m_num_ranks);
  std::intptr_t cur_data_ptr = 0;

  if (is_index_rank)
  {
    if (dataset_parameters.volume_type == vtknvindex_scene::VOLUME_TYPE_REGULAR)
    {
      vtknvindex_regular_volume_data* volume_data =
        static_cast<vtknvindex_regular_volume_data*>(dataset_parameters.volume_data);
      cur_data_ptr = reinterpret_cast<std::intptr_t>(volume_data->scalars);
    }
    else
    {
      vtknvindex_irregular_volume_data* volume_data =
        static_cast<vtknvindex_irregular_volume_data*>(dataset_parameters.volume_data);
      cur_data_ptr = reinterpret_cast<std::intptr_t>(volume_data);
    }
  }
  controller->AllGather(&cur_data_ptr, &all_data_ptrs[0], 1);

// Define this to match local rank i to GPU i. Otherwise IndeX will to the assignment internally,
// using all available GPUs.
//#define VTKNVINDEX_MATCH_GPUS_TO_RANKS
#ifdef VTKNVINDEX_MATCH_GPUS_TO_RANKS
  // Gather all gpu ids.
  mi::Sint32 gpu_id = current_localrank;

  std::vector<mi::Sint32> all_gpu_ids;
  all_gpu_ids.resize(m_num_ranks);
  controller->AllGather(&gpu_id, &all_gpu_ids[0], 1);
#endif

  // Gather host names from all the ranks.
  std::vector<std::string> host_names;
  {
    vtksys::SystemInformation sys_info;
    std::string current_host = sys_info.GetHostname() + std::string(" ");

    char* all_hosts =
      (char*)malloc(m_num_ranks * sizeof(char) * (strlen(current_host.c_str()) + 1));
    controller->AllGather(current_host.c_str(), all_hosts, current_host.length());

    std::string str(all_hosts);
    std::string buf;
    std::stringstream ss(str);

    while (ss >> buf)
      host_names.push_back(buf);

    free(all_hosts);
  }

  // Gather shared memory size for irregular volumes.
  std::vector<mi::Uint64> all_shm_sizes;
  all_shm_sizes.resize(m_num_ranks);
  if (dataset_parameters.volume_type == vtknvindex_scene::VOLUME_TYPE_IRREGULAR)
  {
    vtknvindex_irregular_volume_data* ivol_data =
      static_cast<vtknvindex_irregular_volume_data*>(dataset_parameters.volume_data);

    mi::Uint64 shm_size = ivol_data->get_memory_size(dataset_parameters.scalar_type);
    controller->AllGather(&shm_size, &all_shm_sizes[0], 1);
  }

  const mi::Uint32 nb_time_steps = m_regular_vol_properties->get_nb_time_steps();

  // Add per rank information like affinity, shared memory details etc.
  m_affinity->reset_affinity();
  if (m_affinity_kdtree)
    m_affinity_kdtree->reset_affinity();

  for (mi::Uint32 i = 0; i < m_num_ranks; ++i)
  {
    mi::Uint32 offset = i * 6;
    mi::math::Bbox<mi::Float32, 3> current_bbox(all_rank_extents[offset + 0],
      all_rank_extents[offset + 2], all_rank_extents[offset + 4], all_rank_extents[offset + 1],
      all_rank_extents[offset + 3], all_rank_extents[offset + 5]);

    // The affinity should not contain ghost cells.
    mi::math::Bbox<mi::Float32, 3> current_affinity = current_bbox;

    if (dataset_parameters.volume_type == vtknvindex_scene::VOLUME_TYPE_REGULAR)
    {
      // If the origin is not [0,0,0] we have to translate all the pieces.
      mi::math::Bbox<mi::Sint32, 3> volume_extents;
      m_regular_vol_properties->get_volume_extents(volume_extents);
      const mi::math::Vector<mi::Float32, 3> volume_extents_min(volume_extents.min);

      // Trying to reconstruct cur_bbox without ghost cells, to be used for affinity
      const mi::math::Bbox<mi::Float32, 3> vol_ext_flt(volume_extents);
      const mi::Float32 ghost_levels =
        static_cast<mi::Float32>(m_regular_vol_properties->get_ghost_levels());
      if (ghost_levels > 0.f)
      {
        // Ghost cells only exist at inner boundaries between pieces, not on the outside boundary of
        // the entire volume. Shrink the bbox used for the affinity accordingly.
        if (current_affinity.min.x > vol_ext_flt.min.x)
          current_affinity.min.x += ghost_levels;
        if (current_affinity.min.y > vol_ext_flt.min.y)
          current_affinity.min.y += ghost_levels;
        if (current_affinity.min.z > vol_ext_flt.min.z)
          current_affinity.min.z += ghost_levels;

        if (current_affinity.max.x < vol_ext_flt.max.x)
          current_affinity.max.x -= ghost_levels;
        if (current_affinity.max.y < vol_ext_flt.max.y)
          current_affinity.max.y -= ghost_levels;
        if (current_affinity.max.z < vol_ext_flt.max.z)
          current_affinity.max.z -= ghost_levels;
      }

      // Translate to origin
      current_affinity.min -= volume_extents_min;
      current_affinity.max -= volume_extents_min;
      current_affinity.max += mi::math::Vector<mi::Float32, 3>(1.f); // Convert from "value range"

      // Same for current_bbox (but without the special border/ghosting handling)
      current_bbox.min -= volume_extents_min;
      current_bbox.max -= volume_extents_min;
      current_bbox.max += mi::math::Vector<mi::Float32, 3>(1.f);
    }

    std::string host = host_names[i];
    mi::Uint32 hostid = 0;

    if (all_localrank_ids[i] == 0)
    {
      hostid = all_hostids[i];
      m_hostname_to_hostid[host] = hostid;
    }
    else
    {
      std::map<std::string, mi::Uint32>::const_iterator it = m_hostname_to_hostid.find(host);
      hostid = it->second;
    }

    if (hostid == 0)
    {
      ERROR_LOG << "Host id is 0, this should never happen.";
      return false;
    }

// Set affinity information for NVIDIA IndeX.
#ifdef VTKNVINDEX_MATCH_GPUS_TO_RANKS
    const mi::Uint32 gpu_id = all_gpu_ids[i];
#else
    // Let IndeX decide which GPU to use
    const mi::Uint32 gpu_id = static_cast<mi::Uint32>(nv::index::IAffinity_information::ANY_GPU);
#endif
    m_affinity->add_affinity(current_affinity, hostid, gpu_id);
    if (m_affinity_kdtree)
      m_affinity_kdtree->add_affinity(current_affinity, hostid, gpu_id);

    m_rankid_to_hostid[m_all_rank_ids[i]] = hostid;

    const mi::Sint32 current_rankid = m_all_rank_ids[i];

    std::map<mi::Uint32, vtknvindex_host_properties*>::iterator shmit = m_hostinfo.find(hostid);
    if (shmit == m_hostinfo.end())
    {
      vtknvindex_host_properties* host_properties =
        new vtknvindex_host_properties(hostid, current_rankid, host);

#ifdef VTKNVINDEX_MATCH_GPUS_TO_RANKS
      host_properties->set_gpuids(all_gpu_ids);
#endif
      host_properties->set_rankids(m_all_rank_ids);

      m_hostinfo[hostid] = host_properties;
    }
    else
    {
      vtknvindex_host_properties* host_properties = shmit->second;
#ifdef VTKNVINDEX_MATCH_GPUS_TO_RANKS
      host_properties->set_gpuids(all_gpu_ids);
#endif
      host_properties->set_rankids(m_all_rank_ids);
    }

    for (mi::Uint32 time_step = 0; time_step < nb_time_steps; ++time_step)
    {
      std::stringstream ss;
      ss << "pv_nvindex_shm_instance_" << m_instance_id << "_rank_" << current_rankid
         << "_timestep_" << time_step;

#ifndef _WIN32
      ss << "_" << vtknvindex::util::get_process_user_name();
#endif

      std::map<mi::Uint32, vtknvindex_host_properties*>::iterator shmjt = m_hostinfo.find(hostid);
      vtknvindex_host_properties* host_properties = shmjt->second;
      mi::Size shm_size = 0;

      // Collecting information for shared memory: bbox, type, size, time step.
      if (dataset_parameters.volume_type == vtknvindex_scene::VOLUME_TYPE_REGULAR)
      {
        const mi::math::Vector<mi::Size, 3> pernode_volume_extent(current_bbox.extent());
        const mi::Size volume_size =
          pernode_volume_extent.x * pernode_volume_extent.y * pernode_volume_extent.z;

        std::string scalar_type = dataset_parameters.scalar_type;
        if (scalar_type == "double" && !(is_index_rank && current_rankid == m_rank_id))
        {
          // Data will be converted to float when writing to shared memory. The data in
          // local memory on IndeX ranks will stay in double format.
          scalar_type = "float";
        }

        const mi::Size scalar_size =
          vtknvindex_regular_volume_properties::get_scalar_size(scalar_type);
        if (scalar_size > 0)
        {
          shm_size = volume_size * scalar_size;
        }
        else
        {
          ERROR_LOG << "The scalar type: " << scalar_type << " is not supported by NVIDIA IndeX.";
          return false;
        }
      }
      else // vtknvindex_scene::VOLUME_TYPE_IRREGULAR
      {
        shm_size = all_shm_sizes[i];
      }

      host_properties->set_shminfo(time_step, current_rankid, ss.str(), current_bbox, shm_size,
        reinterpret_cast<void*>(all_data_ptrs[i]));
    }
  }

  // Add more per host information
  for (auto& it : m_hostinfo)
  {
    vtknvindex_host_properties* host_props = it.second;
    for (mi::Uint32 time_step = 0; time_step < nb_time_steps; ++time_step)
    {
      // Neighbor information
      host_props->create_volume_neighbor_info(this, time_step);
    }
  }

  return true;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_cluster_properties::unlink_shared_memory(bool reset)
{
  // Unlink shared memory and delete host properties.
  std::map<mi::Uint32, vtknvindex_host_properties*>::iterator shmit = m_hostinfo.begin();
  for (; shmit != m_hostinfo.end(); ++shmit)
  {
    if (shmit->second)
    {
      shmit->second->shm_cleanup(reset);
    }
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_cluster_properties::print_info() const
{
  INFO_LOG << "-----------------------------------------------";
  INFO_LOG << "Cluster details for NVIDIA IndeX Plugin";
  INFO_LOG << "-----------------------------------------------";

  INFO_LOG << "Total number of MPI ranks: " << m_num_ranks;
  INFO_LOG << "Total number of hosts: " << m_hostinfo.size();
  INFO_LOG << "------------------";
  INFO_LOG << "Data properties: ";
  m_regular_vol_properties->print_info();

  std::map<mi::Uint32, vtknvindex_host_properties*>::const_iterator it = m_hostinfo.begin();
  for (; it != m_hostinfo.end(); ++it)
  {
    INFO_LOG << "------------------";
    INFO_LOG << "Host properties: ";
    it->second->print_info();
  }
  INFO_LOG << "-----------------------------------------------";
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_cluster_properties::get_instance_id() const
{
  return m_instance_id;
}

// ------------------------------------------------------------------------------------------------
vtknvindex_cluster_properties* vtknvindex_cluster_properties::get_instance(mi::Uint32 instance_id)
{
  auto it = s_instances.find(instance_id);
  if (it != s_instances.end())
  {
    return it->second;
  }
  else
  {
    return nullptr;
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_cluster_properties::set_visibility(bool visible)
{
  m_visible = visible;
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_cluster_properties::is_active_instance() const
{
  // Determine visible instance with the highest instance id.
  mi::Uint32 max_visible_instance = 0;
  for (auto& instances : s_instances)
  {
    if (instances.first > max_visible_instance && instances.second->m_visible)
    {
      max_visible_instance = instances.first;
    }
  }

  return (max_visible_instance == get_instance_id());
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_cluster_properties::activate()
{
  if (s_active_instance != get_instance_id())
  {
    s_active_instance = get_instance_id();
    return true;
  }
  else
  {
    // Already active
    return false;
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_cluster_properties::warn_if_multiple_visible_instances(
  const std::string& active_array_name)
{
  mi::Uint32 visible_count = 0;
  for (auto& instances : s_instances)
  {
    if (instances.second->m_visible)
    {
      visible_count++;
    }
  }

  if (visible_count <= 1)
  {
    return;
  }

  std::ostringstream os;
  os << "Multiple sources are marked as visible in the Pipeline Browser, but the NVIDIA IndeX "
     << "plugin will only render the one that was added last (data array '" << active_array_name
     << "').";

  static bool already = false;
  if (!already)
  {
    WARN_LOG << os.str() << " This warning will only be printed once.";
    already = true;
  }
  else
  {
    INFO_LOG << os.str();
  }
}
