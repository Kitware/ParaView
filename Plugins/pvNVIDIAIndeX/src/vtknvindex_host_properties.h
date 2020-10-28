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

#ifndef vtknvindex_host_properties_h
#define vtknvindex_host_properties_h

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <mi/math/bbox.h>
#include <mi/neuraylib/dice.h>

class vtknvindex_cluster_properties;
class vtknvindex_host_properties;

class vtkMultiProcessController;

// Enable data access to neighboring pieces (via shared memory) that are required for proper
// boundary handling of volume data. When the current piece already contains the necessary border
// (e.g. when VTK ghosting is enabled) then no neighbors will be created here.
class vtknvindex_volume_neighbor_data
{
public:
  struct Neighbor_info
  {
    // Direction from current piece to the neighboring piece: -1, 0, 1
    mi::math::Vector<mi::Sint32, 3> direction;

    // Bbox of data that needs to be retrieved from the neighbor piece to satisfy the boundary of
    // the current piece
    mi::math::Bbox<mi::Sint32, 3> border_bbox;

    // Same as border_bbox, but for a fixed border size of 1. Used for intersection queries.
    mi::math::Bbox<mi::Sint32, 3> query_bbox;

    // Bbox of the neighboring piece (matches shm_info::m_shm_bbox)
    mi::math::Bbox<mi::Sint32, 3> data_bbox;

    // Data buffer of the neighbor piece, if available in either in local or shared memory. Its size
    // matches data_bbox.
    const mi::Uint8* data_buffer;

    // Data buffer of just the neighbor border data, if fetched from a remote node. Its size matches
    // border_bbox.
    std::unique_ptr<mi::Uint8[]> border_data_buffer;

    // Host where the data of the neighbor is located
    mi::Uint32 host_id;

    // Rank where the data of the neighbor is located
    mi::Sint32 rank_id;

    // Write the border data to the destination buffer
    void copy(mi::Uint8* dst_buffer, const mi::math::Bbox<mi::Sint32, 3>& dst_buffer_bbox_global,
      mi::Size voxel_fmt_size) const;

    // Data is available either completely in local or shared memory, or as fetched border data.
    bool is_data_available() const;
  };

  typedef std::vector<Neighbor_info*> Neighbor_vector;

  // Create entries for the neighbors of the given piece, but does not set the data buffers.
  vtknvindex_volume_neighbor_data(const mi::math::Bbox<mi::Float32, 3>& piece_bbox,
    vtknvindex_cluster_properties* cluster_props, mi::Uint32 time_step);

  vtknvindex_volume_neighbor_data();

  ~vtknvindex_volume_neighbor_data();

  // Sets up data_buffer of all neighbor entries for which the data is available in local or shared
  // memory.
  void fetch_data(vtknvindex_host_properties* host_props, mi::Uint32 time_step);

  // Allow iterating over the neighbors
  Neighbor_vector::const_iterator begin() const { return m_neighbors.begin(); }
  Neighbor_vector::const_iterator end() const { return m_neighbors.end(); }
  std::size_t size() const { return m_neighbors.size(); }

private:
  vtknvindex_volume_neighbor_data(const vtknvindex_volume_neighbor_data&) = delete;
  void operator=(const vtknvindex_volume_neighbor_data&) = delete;

  Neighbor_vector m_neighbors;
};

// The class vtknvindex_host_properties represents the per host information such as the ranks
// assigned to the host,
// the shared memory information and the gpus to ranks assignments.

class vtknvindex_host_properties : public mi::neuraylib::Base<0xec688118, 0x13f0, 0x4e4f, 0x81,
                                     0x35, 0xb0, 0xac, 0x94, 0xf6, 0x7a, 0x59>
{
public:
  // Shared memory name and bounding box of the
  // data, which the shared memory represents.
  struct shm_info
  {
    shm_info(mi::Sint32 rank_id, mi::Uint32 host_id, std::string shm_name,
      mi::math::Bbox<mi::Float32, 3> bbox, mi::Uint64 shm_size, void* subset_ptr = nullptr)
      : m_rank_id(rank_id)
      , m_host_id(host_id)
      , m_shm_name(shm_name)
      , m_shm_bbox(bbox)
      , m_size(shm_size)
      , m_subset_ptr(subset_ptr)
      , m_mapped_subset_ptr(nullptr)
      , m_read_flag(false)
    {
    }

    shm_info()
      : m_rank_id(-1)
      , m_host_id(0)
      , m_subset_ptr(nullptr)
      , m_mapped_subset_ptr(nullptr)
    {
    }
    mi::Sint32 m_rank_id;
    mi::Uint32 m_host_id;
    std::string m_shm_name;
    mi::math::Bbox<mi::Float32, 3> m_shm_bbox;
    mi::Uint64 m_size;
    void* m_subset_ptr;        // Non-null if data is local, otherwise it is in shared memory
    void* m_mapped_subset_ptr; // Non-null if shared memory buffer is mapped
    bool m_read_flag;          // True if the data was read by at least one importer

    std::unique_ptr<vtknvindex_volume_neighbor_data> m_neighbors;
  };

  vtknvindex_host_properties();
  vtknvindex_host_properties(mi::Uint32 hostid, mi::Sint32 rankid, std::string hostname);

  ~vtknvindex_host_properties();

  // Unmap and unlink any shared memory.
  // If "reset" is true, also clear m_shmlist, i.e. remove all information.
  void shm_cleanup(bool reset);

  // Set the shared memory data for the current bounding box and the time step.
  void set_shminfo(mi::Uint32 time_step, mi::Sint32 rank_id, std::string shmname,
    mi::math::Bbox<mi::Float32, 3> shmbbox, mi::Uint64 shmsize, void* subset_ptr = NULL);

  void set_read_flag(mi::Uint32 time_step, std::string shmname);

  // Get the shared memory data for the current bounding box and the time step.
  bool get_shminfo(const mi::math::Bbox<mi::Float32, 3>& bbox, std::string& shmname,
    mi::math::Bbox<mi::Float32, 3>& shmbbox, mi::Uint64& shmsize, void** subset_ptr,
    mi::Uint32 time_step);

  // Get the shared memory info for the current bounding box and the time step.
  shm_info* get_shminfo(const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 time_step);

  // Get all shared memory infos that intersect (interior) with the given bounding box.
  std::vector<shm_info*> get_shminfo_intersect(
    const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 time_step);

  // Returns the subset (piece) data (and optionally its shm_info), mapping shared memory if
  // necessary. Data will stay mapped until shm_cleanup() gets called.
  const mi::Uint8* get_subset_data_buffer(const mi::math::Bbox<mi::Float32, 3>& bbox,
    mi::Uint32 time_step, const vtknvindex_host_properties::shm_info** shm_info_out = nullptr,
    bool support_time_steps = true);

  // Set/get the GPU ids of the current host.
  void set_gpuids(std::vector<mi::Sint32> gpuids);

  // Set/get the MPI rank ids of the current host.
  void set_rankids(std::vector<mi::Sint32> rankids);

  // Get host name of the current host
  const std::string& get_hostname() const;

  // Get host id of the current host
  mi::Uint32 get_host_id() const;

  // Print the host details.
  void print_info() const;

  // Create neighbor information for all volume pieces on this host.
  void create_volume_neighbor_info(
    vtknvindex_cluster_properties* cluster_props, mi::Uint32 time_step);

  // Sends and receives the volume border data that is required or available on other hosts via MPI.
  void fetch_remote_volume_border_data(vtkMultiProcessController* controller, mi::Uint32 time_step,
    const void* local_piece_data, const std::string& scalar_type);

private:
  vtknvindex_host_properties(const vtknvindex_host_properties&) = delete;
  void operator=(const vtknvindex_host_properties&) = delete;

  mi::Uint32 m_hostid;               // Host id of the machine, matches IndeX host id.
  mi::Sint32 m_nvrankid;             // MPI rankid running NVIDIA IndeX.
  std::string m_hostname;            // Host name of the machine.
  std::vector<mi::Sint32> m_rankids; // List of MPI rankids running on this host.
  std::vector<mi::Sint32> m_gpuids;  // The GPU on which this rank is running on.
  std::map<std::string, mi::Uint32>
    m_shmref; // The reference count on a particular shared memory piece.
  std::map<mi::Uint32, std::vector<shm_info> >
    m_shmlist; // List of shared memory pieces on the present host.

  std::mutex m_mutex;
};

#endif // vtknvindex_host_properties_h
