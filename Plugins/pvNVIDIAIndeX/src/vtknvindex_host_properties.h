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

#ifndef vtknvindex_host_properties_h
#define vtknvindex_host_properties_h

#include <map>
#include <string>
#include <vector>

#include <mi/base/interface_implement.h>
#include <mi/math/bbox.h>
#include <mi/neuraylib/ideserializer.h>
#include <mi/neuraylib/iserializer.h>

// The class vtknvindex_host_properties represents the per host information such as the ranks
// assigned to the host,
// the shared memory information and the gpus to ranks assignments.

class vtknvindex_host_properties_base : public mi::base::Interface_declare<0xec688118, 0x13f0,
                                          0x4e4f, 0x81, 0x35, 0xb0, 0xac, 0x94, 0xf6, 0x7a, 0x59>
{
};

class vtknvindex_host_properties
  : public mi::base::Interface_implement<vtknvindex_host_properties_base>
{
public:
  // Shared memory name and bounding box of the
  // data, which the shared memory represents.
  struct shm_info
  {
    shm_info() { m_raw_mem_pointer = NULL; }

    shm_info(std::string shm_name, mi::math::Bbox<mi::Float32, 3> bbox, mi::Uint64 shm_size,
      void* raw_mem_pointer = NULL)
      : m_shm_name(shm_name)
      , m_shm_bbox(bbox)
      , m_size(shm_size)
      , m_raw_mem_pointer(raw_mem_pointer)
    {
    }

    std::string m_shm_name;
    mi::math::Bbox<mi::Float32, 3> m_shm_bbox;
    mi::Uint64 m_size;
    void* m_raw_mem_pointer;
  };

  vtknvindex_host_properties();
  vtknvindex_host_properties(mi::Uint32 hostid, mi::Sint32 rankid, std::string hostname);

  ~vtknvindex_host_properties();

  void shm_cleanup(bool reset);

  // Set the shared memory data for the current bounding box and the time step.
  void set_shminfo(mi::Uint32 time_step, std::string shmname,
    mi::math::Bbox<mi::Float32, 3> shmbbox, mi::Uint64 shmsize, void* raw_mem_pointer = NULL);

  // free temporary raw pointer
  void free_shm_pointer(mi::Uint32 time_step);

  // Get the shared memory data for the current bounding box and the time step.
  bool get_shminfo(const mi::math::Bbox<mi::Float32, 3>& bbox, std::string& shmname,
    mi::math::Bbox<mi::Float32, 3>& shmbbox, mi::Uint64& shmsize, void** raw_mem_pointer,
    mi::Uint32 time_step);

  // Get the shared memory data that intersects the current bounding box and the time step.
  bool get_shminfo_isect(const mi::math::Bbox<mi::Float32, 3>& bbox, std::string& shmname,
    mi::math::Bbox<mi::Float32, 3>& shmbbox, mi::Uint64& shmsize, void** raw_mem_pointer,
    mi::Uint32 time_step);

  // Get the shared memory info for the current bounding box and the time step.
  shm_info* get_shminfo(const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 time_step);

  // Set/get the GPU ids of the present host.
  void set_gpuids(std::vector<mi::Sint32> gpuids);
  void get_gpuids(std::vector<mi::Sint32>& gpuids) const;

  // Set/get the MPI rank ids of the current host.
  void set_rankids(std::vector<mi::Sint32> rankids);
  void get_rankids(std::vector<mi::Sint32>& rankids) const;

  // Print the host details.
  void print_info() const;

  // DiCE database element methods.
  virtual void serialize(mi::neuraylib::ISerializer* serializer) const;
  virtual void deserialize(mi::neuraylib::IDeserializer* deserializer);
  virtual mi::base::Uuid get_class_id() const;
  virtual const char* get_class_name() const;

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

  mi::base::Lock s_ptr_lock;
};

#endif // vtknvindex_host_properties_h
