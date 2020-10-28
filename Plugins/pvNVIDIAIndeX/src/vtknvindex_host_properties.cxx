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

#include "vtknvindex_host_properties.h"

#include <iostream>
#include <sstream>

#ifdef _WIN32
#else // _WIN32
#include <sys/mman.h>
#endif // _WIN32

#include "vtkMultiProcessController.h"

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_regular_volume_properties.h"
#include "vtknvindex_sparse_volume_importer.h"
#include "vtknvindex_utilities.h"

namespace
{

inline mi::Size calc_offset(mi::Uint32 x, mi::Uint32 y, mi::Uint32 z,
  const mi::math::Vector<mi::Uint32, 3>& buffer_dims, mi::Size fmt_size)
{
  const mi::Size offset = static_cast<mi::Size>(x) + static_cast<mi::Size>(y) * buffer_dims.x +
    static_cast<mi::Size>(z) * buffer_dims.x * buffer_dims.y;
  return offset * fmt_size;
}

// Copy a brick between two buffers. All bboxes are in the same global coordinate system.
template <bool CONVERT_DOUBLE_TO_FLOAT = false>
void copy_brick(const mi::math::Bbox<mi::Sint32, 3>& copy_bbox_global, mi::Uint8* dst_buffer,
  const mi::math::Bbox<mi::Sint32, 3>& dst_buffer_bbox_global, const mi::Uint8* src_buffer,
  const mi::math::Bbox<mi::Sint32, 3>& src_buffer_bbox_global, mi::Size voxel_fmt_size)
{
  using namespace vtknvindex::util;

  // Destination buffer bbox might be smaller than requested copy_bbox, clamp accordingly.
  // Note: No clamping for source buffer bbox
  const Bbox3i copy_bbox_global_clamped(
    mi::math::clamp(copy_bbox_global.min, dst_buffer_bbox_global.min, dst_buffer_bbox_global.max),
    mi::math::clamp(copy_bbox_global.max, dst_buffer_bbox_global.min, dst_buffer_bbox_global.max));

#if 0
  ERROR_LOG << "*** copy_to_brick copy_bbox=" << copy_bbox_global
            << ", clamped_bbox=" << copy_bbox_global_clamped
            << ", dst_buffer_bbox=" << dst_buffer_bbox_global
            << ", src_buffer_bbox=" << src_buffer_bbox_global;
#endif

  const Bbox3u read_bbox(Vec3u(copy_bbox_global_clamped.min - src_buffer_bbox_global.min),
    Vec3u(copy_bbox_global_clamped.max - src_buffer_bbox_global.min));

  const Vec3i write_delta(src_buffer_bbox_global.min - dst_buffer_bbox_global.min);

  const Bbox3u write_bbox(
    Vec3u(Vec3i(read_bbox.min) + write_delta), Vec3u(Vec3i(read_bbox.max) + write_delta));

  const Vec3u dst_buffer_dims(dst_buffer_bbox_global.extent());
  const Vec3u src_buffer_dims(src_buffer_bbox_global.extent());

  const mi::Size len_x = read_bbox.max.x - read_bbox.min.x;

  for (mi::Uint32 z = read_bbox.min.z; z < read_bbox.max.z; ++z)
  {
    for (mi::Uint32 y = read_bbox.min.y; y < read_bbox.max.y; ++y)
    {
      const mi::Size src_offset =
        calc_offset(read_bbox.min.x, y, z, src_buffer_dims, voxel_fmt_size);
      const mi::Size dst_offset =
        calc_offset(read_bbox.min.x + write_delta.x, y + write_delta.y, z + write_delta.z,
          dst_buffer_dims, CONVERT_DOUBLE_TO_FLOAT ? sizeof(mi::Float32) : voxel_fmt_size);

      if (CONVERT_DOUBLE_TO_FLOAT)
      {
        const mi::Float64* src_double =
          reinterpret_cast<const mi::Float64*>(src_buffer + src_offset);
        mi::Float32* dst_float = reinterpret_cast<mi::Float32*>(dst_buffer + dst_offset);
        for (mi::Size i = 0; i < len_x; ++i)
        {
          dst_float[i] = static_cast<mi::Float32>(src_double[i]);
        }
      }
      else
      {
        memcpy(dst_buffer + dst_offset, src_buffer + src_offset, len_x * voxel_fmt_size);
      }
    }
  }
}

void copy_brick_double_to_float(const mi::math::Bbox<mi::Sint32, 3>& copy_bbox_global,
  mi::Uint8* dst_buffer, const mi::math::Bbox<mi::Sint32, 3>& dst_buffer_bbox_global,
  const mi::Uint8* src_buffer, const mi::math::Bbox<mi::Sint32, 3>& src_buffer_bbox_global)
{
  copy_brick<true>(copy_bbox_global, dst_buffer, dst_buffer_bbox_global, src_buffer,
    src_buffer_bbox_global, sizeof(mi::Float64));
}

// In contrast to Bbox::intersects(), this does not consider bboxes intersecting when only their
// boundaries intersect.
inline bool bbox_interior_intersects(
  const mi::math::Bbox<mi::Float32, 3>& box, const mi::math::Bbox<mi::Float32, 3>& other)
{
  for (mi::Size i = 0; i < 3; ++i)
  {
    if ((box.min[i] >= other.max[i]) || (box.max[i] <= other.min[i]))
    {
      return false;
    }
  }
  return true;
}

} // namespace

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_neighbor_data::Neighbor_info::copy(mi::Uint8* dst_buffer,
  const mi::math::Bbox<mi::Sint32, 3>& dst_buffer_bbox_global, mi::Size voxel_fmt_size) const
{
  const mi::Uint8* src_buffer;
  const mi::math::Bbox<mi::Sint32, 3>* src_buffer_bbox_global;

  if (data_buffer)
  {
    src_buffer = data_buffer;
    src_buffer_bbox_global = &data_bbox;
  }
  else if (border_data_buffer)
  {
    // The size of the fetched data is border_bbox, not data_bbox
    src_buffer = border_data_buffer.get();
    src_buffer_bbox_global = &border_bbox;
  }
  else
  {
    return; // no data available
  }

  copy_brick(border_bbox, dst_buffer, dst_buffer_bbox_global, src_buffer, *src_buffer_bbox_global,
    voxel_fmt_size);
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_volume_neighbor_data::Neighbor_info::is_data_available() const
{
  return (data_buffer || border_data_buffer);
}

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_neighbor_data::vtknvindex_volume_neighbor_data(
  const mi::math::Bbox<mi::Float32, 3>& piece_bbox, vtknvindex_cluster_properties* cluster_props,
  mi::Uint32 time_step)
{
  const vtknvindex_regular_volume_properties* regular_volume_properties =
    cluster_props->get_regular_volume_properties();

  mi::math::Vector<mi::Uint32, 3> volume_size;
  regular_volume_properties->get_volume_size(volume_size);
  const mi::Sint32 border_size = cluster_props->get_config_settings()->get_subcube_border();
  const mi::Uint32 ghost_levels = regular_volume_properties->get_ghost_levels();

  if (static_cast<mi::Sint32>(ghost_levels) >= border_size)
  {
    // Border already handled by VTK ghosting, nothing to do
    return;
  }

  // Handle the case where VTK ghost levels are set to a smaller value than required by IndeX
  const mi::Uint32 remaining_border_size = border_size - ghost_levels;

  mi::math::Vector<mi::Sint32, 3> dir;
  for (dir.z = -1; dir.z <= 1; ++dir.z)
  {
    for (dir.y = -1; dir.y <= 1; ++dir.y)
    {
      for (dir.x = -1; dir.x <= 1; ++dir.x)
      {
        if (dir.x == 0 && dir.y == 0 && dir.z == 0)
        {
          continue; // current piece, not a neighbor, skip
        }

        // Bbox containing all the required border data, located in the neighbor piece.
        mi::math::Bbox<mi::Sint32, 3> border_bbox(piece_bbox);

        // Similar, but fixed to a border size of 1. This is used to intersect with the bbox of
        // the neighbor, making sure to handle the case where border_bbox is larger than the bbox
        // of the neighbor.
        mi::math::Bbox<mi::Float32, 3> query_bbox = piece_bbox;

        bool outer_boundary = false;
        for (int i = 0; i < 3; ++i)
        {
          if (dir[i] < 0)
          {
            border_bbox.max[i] = border_bbox.min[i];
            border_bbox.min[i] -= remaining_border_size;

            // Use fixed border size of 1 for query to handle the case when size of the neighbor
            // piece is smaller than the border size (data will be clamped later).
            query_bbox.max[i] = query_bbox.min[i];
            query_bbox.min[i] -= 1;

            if (query_bbox.min[i] < 0)
            {
              outer_boundary = true;
              break;
            }
          }
          else if (dir[i] > 0)
          {
            border_bbox.min[i] = border_bbox.max[i];
            border_bbox.max[i] += remaining_border_size;

            query_bbox.min[i] = query_bbox.max[i];
            query_bbox.max[i] += 1;

            if (query_bbox.max[i] > volume_size[i])
            {
              outer_boundary = true;
              break;
            }
          }
        }

        if (outer_boundary)
        {
          // The outer boundary of the volume will be clamped anyway, ignore
          continue;
        }

        // Retrieve neighbor information, i.e. from pieces that intersect with query_bbox.
        //
        // This may return multiple results if the pieces are not uniformly sized and therefore a
        // single piece can have multiple neighbors per direction.
        //
        // If pieces are not disjoint (without considering ghosting), it can happen that the same
        // border data is fetched/copied multiple times, since multiple piece might cover parts of
        // query_bbox.
        //
        const std::vector<vtknvindex_host_properties::shm_info*> shm_info_vector =
          cluster_props->get_shminfo_intersect(query_bbox, time_step);

        if (shm_info_vector.empty())
        {
          ERROR_LOG << "Could not access volume information for neighbor " << query_bbox << " of "
                    << piece_bbox << ".";
        }

        Neighbor_vector neighbors;
        for (vtknvindex_host_properties::shm_info* shm_info : shm_info_vector)
        {
          if (!shm_info)
          {
            ERROR_LOG << "Received null pointer for shm  information for neighbor " << query_bbox
                      << " of " << piece_bbox << ".";
            break;
          }

          const mi::math::Bbox<mi::Sint32, 3> data_bbox(shm_info->m_shm_bbox);

          // Clip border_bbox against the bbox of the actually available neighbor data, to handle
          // the case when border_bbox is larger than the bbox of the neighbor, e.g. when the border
          // size is set to 2 but the volume size of the neighbor is just 1.
          //
          // This also handles the case when there are multiple neighbors per direction, and no
          // single neighbor alone has all the necessary data.
          //
          const mi::math::Bbox<mi::Sint32, 3> border_bbox_clipped(
            mi::math::clamp(border_bbox.min, data_bbox.min, data_bbox.max),
            mi::math::clamp(border_bbox.max, data_bbox.min, data_bbox.max));

          Neighbor_info* ni = new Neighbor_info();
          ni->direction = dir;
          ni->border_bbox = border_bbox_clipped;
          ni->query_bbox = mi::math::Bbox<mi::Sint32, 3>(query_bbox);
          ni->data_bbox = data_bbox;
          ni->data_buffer = nullptr; // will be filled by fetch_data()
          ni->host_id = shm_info->m_host_id;
          ni->rank_id = shm_info->m_rank_id;

          if (border_bbox_clipped == border_bbox)
          {
            // Optimization for non-disjoint pieces: If this neighbor has the entire queried data
            // available, then only use it and ignore all other neighbors, which will probably only
            // have smaller parts of it. This won't prevent all duplicate copies, but catches the
            // most common case.
            for (auto& it : neighbors)
            {
              delete it;
            }
            neighbors.clear();
            neighbors.push_back(ni);
            break;
          }
          else
          {
            neighbors.push_back(ni);
          }
        }

        // Append the newly created neighbors
        m_neighbors.insert(m_neighbors.end(), neighbors.begin(), neighbors.end());
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_neighbor_data::vtknvindex_volume_neighbor_data()
{
  // empty
}

//-------------------------------------------------------------------------------------------------
vtknvindex_volume_neighbor_data::~vtknvindex_volume_neighbor_data()
{
  for (auto& it : m_neighbors)
  {
    delete it;
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volume_neighbor_data::fetch_data(
  vtknvindex_host_properties* host_props, mi::Uint32 time_step)
{
  const mi::Uint32 current_host = host_props->get_host_id();

  for (Neighbor_info* neighbor : m_neighbors)
  {
    if (neighbor->data_buffer)
    {
      continue;
    }

    if (neighbor->host_id == current_host)
    {
      // Data is available in local or shared memory
      neighbor->data_buffer = host_props->get_subset_data_buffer(
        mi::math::Bbox<mi::Float32, 3>(neighbor->query_bbox), time_step);
    }
    else if (neighbor->border_data_buffer)
    {
      // Data was fetched from a remote host, nothing to do
    }
    else
    {
      ERROR_LOG << "Border data for " << neighbor->data_bbox << " from remote host "
                << neighbor->host_id << " is missing on " << current_host << ", fetch failed?";
    }
  }
}

// ------------------------------------------------------------------------------------------------
vtknvindex_host_properties::vtknvindex_host_properties()
  : m_hostid(0)
  , m_nvrankid(-1)
{
  // empty
}

// ------------------------------------------------------------------------------------------------
vtknvindex_host_properties::vtknvindex_host_properties(
  mi::Uint32 hostid, mi::Sint32 rankid, std::string hostname)
  : m_hostid(hostid)
  , m_nvrankid(rankid)
  , m_hostname(hostname)
{
  // empty
}

// ------------------------------------------------------------------------------------------------
vtknvindex_host_properties::~vtknvindex_host_properties()
{
  // empty
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::shm_cleanup(bool reset)
{
  for (auto& timestep : m_shmlist)
  {
    bool all_shm_read = true;
    if (!reset)
    {
      for (shm_info& current_shm : timestep.second)
      {
        if (current_shm.m_subset_ptr == nullptr && !current_shm.m_read_flag)
        {
          all_shm_read = false;
          break;
        }
      }

      if (!all_shm_read)
      {
        // Not all shared memory have been read. Can't free individual ones after they have been
        // read, because they might still be needed for boundary data access from other importers on
        // the same host (fetching boundary data from remote hosts is not affected, as it fetches
        // directly from the ranks that have the data).
        continue;
      }
    }

    for (shm_info& current_shm : timestep.second)
    {
      if (current_shm.m_subset_ptr == nullptr)
      {
        // Data is not in local memory
        bool was_unmapped = false;

        // Unmap the shared memory if it is currently mapped
        if (current_shm.m_mapped_subset_ptr != nullptr)
        {
          vtknvindex::util::unmap_shm(current_shm.m_mapped_subset_ptr, current_shm.m_size);
          current_shm.m_mapped_subset_ptr = nullptr;
          was_unmapped = true;
        }

        // Unlink (delete) shared memory object.
        if (was_unmapped || reset)
        {
#ifdef _WIN32
// TODO: Unlink using windows functions
#else  // _WIN32
          if (shm_unlink(current_shm.m_shm_name.c_str()) == 0)
          {
            INFO_LOG << "Freed shared memory: " << current_shm.m_shm_name;
          }
#endif // _WIN32
        }
      }

      // Also remove any fetched neighbor data
      current_shm.m_neighbors.reset();
    }
  }

  if (reset)
  {
    m_shmlist.clear();
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::set_shminfo(mi::Uint32 time_step, mi::Sint32 rank_id,
  std::string shmname, mi::math::Bbox<mi::Float32, 3> shmbbox, mi::Uint64 shmsize, void* subset_ptr)
{
  std::map<mi::Uint32, std::vector<shm_info> >::iterator shmit = m_shmlist.find(time_step);
  if (shmit == m_shmlist.end())
  {
    std::vector<shm_info> shmlist;
    shmlist.push_back(shm_info(rank_id, m_hostid, shmname, shmbbox, shmsize, subset_ptr));
    m_shmlist[time_step] = std::move(shmlist);
  }
  else
  {
    shmit->second.push_back(shm_info(rank_id, m_hostid, shmname, shmbbox, shmsize, subset_ptr));
  }

  // TODO : Change this to reflect the actual subcube size.
  mi::Float32 subcube_size = 510;
  mi::math::Vector_struct<mi::Float32, 3> shmvolume;
  shmvolume.x = shmbbox.max.x - shmbbox.min.x;
  shmvolume.y = shmbbox.max.y - shmbbox.min.y;
  shmvolume.z = shmbbox.max.z - shmbbox.min.z;
  mi::Float32 shm_reference_count = ceil((shmvolume.x / subcube_size)) *
    ceil((shmvolume.y / subcube_size)) * ceil((shmvolume.z / subcube_size));

  m_shmref[shmname] = static_cast<mi::Uint32>(shm_reference_count);
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::set_read_flag(mi::Uint32 time_step, std::string shmname)
{
  auto shmit = m_shmlist.find(time_step);
  if (shmit != m_shmlist.end())
  {
    for (shm_info& info : shmit->second)
    {
      if (info.m_shm_name == shmname)
      {
        info.m_read_flag = true;
        return;
      }
    }
  }

  ERROR_LOG << "Could not set read flag for shared memory information '" << shmname
            << "' in time step " << time_step << ".";
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_host_properties::get_shminfo(const mi::math::Bbox<mi::Float32, 3>& bbox,
  std::string& shmname, mi::math::Bbox<mi::Float32, 3>& shmbbox, mi::Uint64& shmsize,
  void** subset_ptr, mi::Uint32 time_step)
{
  std::map<mi::Uint32, std::vector<shm_info> >::iterator shmit = m_shmlist.find(time_step);
  if (shmit == m_shmlist.end())
  {
    ERROR_LOG << "The shared memory information in vtknvindex_host_properties::get_shminfo is not "
                 "available for the time step: "
              << time_step << ".";
    return false;
  }

  std::vector<shm_info>& shmlist = shmit->second;
  if (shmlist.empty())
  {
    ERROR_LOG << "The shared memory list in vtknvindex_host_properties::get_shminfo is empty.";
    return false;
  }

  for (mi::Uint32 i = 0; i < shmlist.size(); ++i)
  {
    const shm_info& current_shm = shmlist[i];

    if (current_shm.m_shm_bbox.contains(bbox.min) && current_shm.m_shm_bbox.contains(bbox.max))
    {
      shmname = current_shm.m_shm_name;
      shmbbox = current_shm.m_shm_bbox;
      shmsize = current_shm.m_size;
      *subset_ptr = current_shm.m_subset_ptr;

      return true;
    }
  }
  return false;
}

// ------------------------------------------------------------------------------------------------
vtknvindex_host_properties::shm_info* vtknvindex_host_properties::get_shminfo(
  const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 time_step)
{
  std::map<mi::Uint32, std::vector<shm_info> >::iterator shmit = m_shmlist.find(time_step);
  if (shmit == m_shmlist.end())
  {
    ERROR_LOG << "The shared memory information in vtknvindex_host_properties::get_shminfo is not "
                 "available for the time step: "
              << time_step << ".";

    return NULL;
  }

  std::vector<shm_info>& shmlist = shmit->second;

  if (shmlist.empty())
  {
    ERROR_LOG << "The shared memory list in vtknvindex_host_properties::get_shminfo is empty.";
    return NULL;
  }

  for (mi::Uint32 i = 0; i < shmlist.size(); ++i)
  {
    shm_info* current_shm = &shmlist[i];

    if (current_shm->m_shm_bbox.contains(bbox.min) && current_shm->m_shm_bbox.contains(bbox.max))
    {
      return current_shm;
    }
  }
  return NULL;
}

// ------------------------------------------------------------------------------------------------
std::vector<vtknvindex_host_properties::shm_info*>
vtknvindex_host_properties::get_shminfo_intersect(
  const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 time_step)
{
  std::vector<vtknvindex_host_properties::shm_info*> result;

  auto shmit = m_shmlist.find(time_step);
  if (shmit == m_shmlist.end())
  {
    ERROR_LOG << "The shared memory information in "
                 "vtknvindex_host_properties::get_shminfo_intersect is not "
                 "available for the time step: "
              << time_step << ".";

    return result;
  }

  std::vector<shm_info>& shmlist = shmit->second;
  if (shmlist.empty())
  {
    ERROR_LOG
      << "The shared memory list in vtknvindex_host_properties::get_shminfo_intersect is empty.";
    return result;
  }

  for (shm_info& current_shm : shmlist)
  {
    // Important to use interior intersection here, i.e. bboxes that are just touching on their
    // boundary are not considered intersecting.
    if (bbox_interior_intersects(current_shm.m_shm_bbox, bbox))
    {
      result.push_back(&current_shm);
    }
  }

  return result;
}

// ------------------------------------------------------------------------------------------------
const mi::Uint8* vtknvindex_host_properties::get_subset_data_buffer(
  const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 time_step,
  const vtknvindex_host_properties::shm_info** shm_info_out, bool support_time_steps)
{
  vtknvindex_host_properties::shm_info* shm_info = get_shminfo(bbox, time_step);
  if (shm_info_out)
  {
    *shm_info_out = shm_info;
  }

  if (!shm_info)
  {
    return nullptr;
  }

  std::lock_guard<std::mutex> lock(m_mutex);

  if (shm_info->m_subset_ptr)
  {
    // Data is locally available in this rank, return directly
    if (support_time_steps)
    {
      void** subdivision_pointers = static_cast<void**>(shm_info->m_subset_ptr);
      // TODO: multiple timesteps only supported in non-MPI mode?
      return reinterpret_cast<const mi::Uint8*>(subdivision_pointers[time_step]);
    }
    else
    {
      return reinterpret_cast<const mi::Uint8*>(shm_info->m_subset_ptr);
    }
  }

  // Data must be in shared memory, map it if not already the case
  if (!shm_info->m_mapped_subset_ptr)
  {
    shm_info->m_mapped_subset_ptr =
      vtknvindex::util::get_vol_shm(shm_info->m_shm_name, shm_info->m_size);
  }

  return reinterpret_cast<mi::Uint8*>(shm_info->m_mapped_subset_ptr);
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::set_gpuids(std::vector<mi::Sint32> gpuids)
{
  m_gpuids = gpuids;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::set_rankids(std::vector<mi::Sint32> rankids)
{
  m_rankids = rankids;
}

// ------------------------------------------------------------------------------------------------
const std::string& vtknvindex_host_properties::get_hostname() const
{
  return m_hostname;
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_host_properties::get_host_id() const
{
  return m_hostid;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::create_volume_neighbor_info(
  vtknvindex_cluster_properties* cluster_props, mi::Uint32 time_step)
{
  auto it = m_shmlist.find(time_step);
  if (it == m_shmlist.end())
  {
    return;
  }

  for (shm_info& info : it->second)
  {
    info.m_neighbors = std::unique_ptr<vtknvindex_volume_neighbor_data>(
      new vtknvindex_volume_neighbor_data(info.m_shm_bbox, cluster_props, time_step));
  }
}

// ------------------------------------------------------------------------------------------------

namespace
{

struct Border_fetch_info
{
  mi::math::Bbox<mi::Float32, 3> base_bbox;  // piece for which neighbor data needs to be fetched
  mi::math::Bbox<mi::Sint32, 3> border_bbox; // what needs to be fetched
  mi::math::Bbox<mi::Sint32, 3>
    data_bbox; // full bbox of the neighbor piece containing the border data
  mi::Sint32 src_rank_id;
  mi::Sint32 dst_rank_id;
};

} // namespace

void vtknvindex_host_properties::fetch_remote_volume_border_data(
  vtkMultiProcessController* controller, mi::Uint32 time_step, const void* local_piece_data,
  const std::string& scalar_type)
{
  const mi::Size num_procs = controller->GetNumberOfProcesses();
  if (num_procs == 1)
  {
    return; // nothing to do
  }
  const int rank_id = controller->GetLocalProcessId();
  const bool is_index_rank = (rank_id == m_nvrankid);

  // Build list of all neighbor border data needed on this host that is located on remote hosts
  // (i.e. not reachable via shared memory). Only IndeX ranks will receive border data, but all
  // ranks will send.
  std::vector<Border_fetch_info> local_fetch_list;
  if (is_index_rank)
  {
    auto it = m_shmlist.find(time_step);
    if (it != m_shmlist.end())
    {
      for (const shm_info& info : it->second)
      {
        if (info.m_neighbors)
        {
          for (const vtknvindex_volume_neighbor_data::Neighbor_info* neighbor : *info.m_neighbors)
          {
            if (neighbor->host_id != m_hostid)
            {
              local_fetch_list.push_back({ info.m_shm_bbox, neighbor->border_bbox,
                neighbor->data_bbox, neighbor->rank_id, rank_id });
            }
          }
        }
      }
    }
  }

  // Share how many entries each rank has in its local fetch list
  std::vector<vtkIdType> fetch_list_lengths(num_procs);
  const vtkIdType local_fetch_list_length = local_fetch_list.size();
  controller->AllGather(&local_fetch_list_length, fetch_list_lengths.data(), 1);

  // Prepare information for the global fetch list
  mi::Size global_fetch_list_length = 0;
  for (size_t i = 0; i < fetch_list_lengths.size(); ++i)
  {
    // Sum up size of all local fetch lists
    global_fetch_list_length += fetch_list_lengths[i];
  }

  if (global_fetch_list_length == 0)
  {
    return; // nothing to do, early out
  }

  std::vector<vtkIdType> fetch_list_offsets(num_procs);
  std::vector<vtkIdType> fetch_list_data_sizes(num_procs);
  mi::Size pos = 0;
  for (mi::Size i = 0; i < num_procs; ++i)
  {
    // Compute data size of each local fetch list
    fetch_list_data_sizes[i] = fetch_list_lengths[i] * sizeof(Border_fetch_info);

    // Compute offset in the global fetch list
    fetch_list_offsets[i] = pos;

    pos += fetch_list_data_sizes[i];
  }

  // Gather all local fetch lists and merge them into the global fetch list that is then available
  // on every rank
  std::vector<Border_fetch_info> global_fetch_list(global_fetch_list_length);
  controller->AllGatherV(reinterpret_cast<const unsigned char*>(local_fetch_list.data()),
    reinterpret_cast<unsigned char*>(global_fetch_list.data()),
    local_fetch_list.size() * sizeof(Border_fetch_info), fetch_list_data_sizes.data(),
    fetch_list_offsets.data());

  const int COMM_TAG = 200; // recommended to use custom tag number over 100

  mi::Size scalar_size = vtknvindex_regular_volume_properties::get_scalar_size(scalar_type);
  if (scalar_type == "double")
  {
    // Convert to float when writing to the send buffer
    scalar_size = sizeof(mi::Float32);
  }
  else if (scalar_size == 0)
  {
    return;
  }

  // Iterate over global fetch list on all ranks, calling send or receive for entries that
  // reference the current rank
  for (size_t i = 0; i < global_fetch_list.size(); ++i)
  {
    const Border_fetch_info& f = global_fetch_list[i];
    const mi::Size buffer_size =
      static_cast<mi::Size>(mi::math::Bbox<mi::Sint64, 3>(f.border_bbox).volume()) * scalar_size;

    if (f.src_rank_id == rank_id)
    {
      //
      // Send
      //
      const shm_info* local_info =
        get_shminfo(mi::math::Bbox<mi::Float32, 3>(f.data_bbox), time_step);

      auto buffer = std::unique_ptr<mi::Uint8[]>(new mi::Uint8[buffer_size]);

      if (local_info)
      {
        if (scalar_type == "double")
        {
          copy_brick_double_to_float(f.border_bbox, buffer.get(), f.border_bbox,
            reinterpret_cast<const mi::Uint8*>(local_piece_data), f.data_bbox);
        }
        else
        {
          copy_brick(f.border_bbox, buffer.get(), f.border_bbox,
            reinterpret_cast<const mi::Uint8*>(local_piece_data), f.data_bbox, scalar_size);
        }
      }
      else
      {
        ERROR_LOG << "Rank " << rank_id << " could not satisfy a request from rank "
                  << f.dst_rank_id << " for " << f.border_bbox << " of " << f.data_bbox
                  << " because "
                  << "no information is available about this volume piece";
      }

      const int result = controller->Send(buffer.get(), buffer_size, f.dst_rank_id, COMM_TAG);
      if (!result)
      {
        ERROR_LOG << "MPI send failed for border data " << f.border_bbox << " from rank "
                  << f.src_rank_id << " to rank " << f.dst_rank_id << ", buffer size "
                  << buffer_size << ", result " << result << ".";
      }
    }
    else if (f.dst_rank_id == rank_id)
    {
      //
      // Receive
      //
      auto buffer = std::unique_ptr<mi::Uint8[]>(new mi::Uint8[buffer_size]);

      const int result = controller->Receive(buffer.get(), buffer_size, f.src_rank_id, COMM_TAG);
      if (!result || static_cast<mi::Size>(controller->GetCount()) != buffer_size)
      {
        ERROR_LOG << "MPI receive failed for border data " << f.border_bbox << " from rank "
                  << f.src_rank_id << " to rank " << f.dst_rank_id << ", buffer size "
                  << buffer_size << ", received size " << controller->GetCount() << ", result "
                  << result << ".";
      }

      // Get info of the piece for which (not from which) neighbor data is being fetched
      const shm_info* base_info = get_shminfo(f.base_bbox, time_step);
      bool found = false;
      if (base_info && base_info->m_neighbors)
      {
        for (auto it : *base_info->m_neighbors)
        {
          // Need to compare both border_bbox and data_bbox here, because there may be multiple
          // requests for the same border_bbox in case pieces are not disjoint.
          if (it->border_bbox == f.border_bbox && it->data_bbox == f.data_bbox)
          {
            it->border_data_buffer = std::move(buffer);
            found = true;
            break;
          }
        }
      }

      if (!found)
      {
        ERROR_LOG << "Rank " << rank_id << " failed to store border data " << f.border_bbox
                  << " of " << f.data_bbox << " from rank " << f.src_rank_id
                  << ", data size: " << buffer_size;
      }
    }
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::print_info() const
{
  INFO_LOG << "Host name: " << m_hostname << ", hostid: " << m_hostid;

  std::ostringstream os_ranks;
  for (auto id : m_rankids)
    os_ranks << id << " ";
  INFO_LOG << "Rank ids: " << os_ranks.str();

  if (!m_gpuids.empty())
  {
    std::ostringstream os_gpus;
    for (mi::Uint32 id : m_gpuids)
      os_gpus << id << " ";
    INFO_LOG << "GPU ids: " << os_gpus.str();
  }

  INFO_LOG << "Shared memory pieces [shmname, shmbbox]:";
  for (auto& shmit : m_shmlist)
  {
    for (const shm_info& info : shmit.second)
    {
      INFO_LOG << "Time step: " << shmit.first << " " << info.m_shm_name << ":" << info.m_shm_bbox;
    }
  }
}
