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

#include <iostream>

#ifdef _WIN32
#else // _WIN32
#include <sys/mman.h>
#endif // _WIN32

#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_host_properties.h"

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
  std::map<mi::Uint32, std::vector<shm_info> >::iterator shmit = m_shmlist.begin();
  for (; shmit != m_shmlist.end(); ++shmit)
  {
    std::vector<shm_info> shmlist = shmit->second;

    for (mi::Uint32 i = 0; i < shmlist.size(); ++i)
    {
      shm_info current_shm = shmlist[i];
#ifdef _WIN32
// TODO: Unlink using windows functions
#else  // _WIN32
      shm_unlink(current_shm.m_shm_name.c_str());
#endif // _WIN32
    }
  }

  if (reset)
    m_shmlist.clear();
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::set_shminfo(mi::Uint32 time_step, std::string shmname,
  mi::math::Bbox<mi::Float32, 3> shmbbox, mi::Uint64 shmsize, void* raw_mem_pointer)
{
  std::map<mi::Uint32, std::vector<shm_info> >::iterator shmit = m_shmlist.find(time_step);
  if (shmit == m_shmlist.end())
  {
    std::vector<shm_info> shmlist;
    shmlist.push_back(shm_info(shmname, shmbbox, shmsize, raw_mem_pointer));
    m_shmlist[time_step] = shmlist;
  }
  else
  {
    shmit->second.push_back(shm_info(shmname, shmbbox, shmsize, raw_mem_pointer));
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

void vtknvindex_host_properties::free_shm_pointer(mi::Uint32 time_step)
{
  std::map<mi::Uint32, std::vector<shm_info> >::iterator shmit = m_shmlist.find(time_step);
  if (shmit != m_shmlist.end())
  {
    std::vector<shm_info>& shmlist = shmit->second;
    for (mi::Uint32 i = 0; i < shmlist.size(); i++)
    {
      mi::base::Lock::Block block(&s_ptr_lock);

      if (shmlist[i].m_raw_mem_pointer != NULL)
      {
        free(shmlist[i].m_raw_mem_pointer);
        shmlist[i].m_raw_mem_pointer = NULL;
      }
    }
  }
}

// ------------------------------------------------------------------------------------------------
// Returns \c true if the point is inside or on the boundary of the bounding box.
namespace
{
bool contains(const mi::math::Bbox<mi::Float32, 3>& bb, const mi::math::Vector<mi::Float32, 3>& vec)
{
  for (mi::Size i = 0; i < 3; i++)
  {
    if (vec[i] < bb.min[i] || vec[i] > bb.max[i])
    {
      return false;
    }
  }
  return true;
}
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_host_properties::get_shminfo(const mi::math::Bbox<mi::Float32, 3>& bbox,
  std::string& shmname, mi::math::Bbox<mi::Float32, 3>& shmbbox, mi::Uint64& shmsize,
  void** raw_mem_pointer, mi::Uint32 time_step)
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
    shm_info current_shm = shmlist[i];

    if (contains(current_shm.m_shm_bbox, bbox.min) && contains(current_shm.m_shm_bbox, bbox.max))
    {
      shmname = current_shm.m_shm_name;
      shmbbox = current_shm.m_shm_bbox;
      shmsize = current_shm.m_size;
      *raw_mem_pointer = current_shm.m_raw_mem_pointer;
      return true;
    }
  }
  return false;
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_host_properties::get_shminfo_isect(const mi::math::Bbox<mi::Float32, 3>& bbox,
  std::string& shmname, mi::math::Bbox<mi::Float32, 3>& shmbbox, mi::Uint64& shmsize,
  void** raw_mem_pointer, mi::Uint32 time_step)
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
    shm_info current_shm = shmlist[i];

    if (current_shm.m_shm_bbox.intersects(bbox))
    {
      shmname = current_shm.m_shm_name;
      shmbbox = current_shm.m_shm_bbox;
      shmsize = current_shm.m_size;
      *raw_mem_pointer = current_shm.m_raw_mem_pointer;
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
void vtknvindex_host_properties::set_gpuids(std::vector<mi::Sint32> gpuids)
{
  m_gpuids = gpuids;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::get_gpuids(std::vector<mi::Sint32>& gpuids) const
{
  gpuids = m_gpuids;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::set_rankids(std::vector<mi::Sint32> rankids)
{
  m_rankids = rankids;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::get_rankids(std::vector<mi::Sint32>& rankids) const
{
  rankids = m_rankids;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::print_info() const
{
  INFO_LOG << "Host name: " << m_hostname << " : hostid: " << m_hostid;
  INFO_LOG << "------------------";

  INFO_LOG << "Rank ids: " << m_rankids.size();
  std::cout << "        PVPLN  init info : ";
  for (mi::Uint32 i = 0; i < m_rankids.size(); ++i)
    std::cout << m_rankids[i] << ", ";
  std::cout << std::endl;

  INFO_LOG << "GPU ids: " << m_gpuids.size();
  std::cout << "        PVPLN  init info : ";
  for (mi::Uint32 i = 0; i < m_gpuids.size(); ++i)
    std::cout << m_gpuids[i] << ", ";
  std::cout << std::endl;

  INFO_LOG << "Shared memory pieces [shmname, shmbbox]";
  std::map<mi::Uint32, std::vector<shm_info> >::const_iterator shmit = m_shmlist.begin();
  for (; shmit != m_shmlist.end(); ++shmit)
  {
    std::vector<shm_info> shmlist = shmit->second;
    for (mi::Uint32 i = 0; i < shmlist.size(); ++i)
    {
      INFO_LOG << "Time step: " << shmit->first << " " << shmlist[i].m_shm_name << ":"
               << shmlist[i].m_shm_bbox;
    }
  }
}

// ------------------------------------------------------------------------------------------------
const char* vtknvindex_host_properties::get_class_name() const
{
  return "vtknvindex_cluster_properties";
}

// ------------------------------------------------------------------------------------------------
mi::base::Uuid vtknvindex_host_properties::get_class_id() const
{
  return IID();
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::serialize(mi::neuraylib::ISerializer* serializer) const
{
  // Serialize rankid to host id.
  {
    const mi::Size nb_elements = m_shmref.size();
    serializer->write(&nb_elements, 1);

    std::map<std::string, mi::Uint32>::const_iterator itr = m_shmref.begin();
    for (; itr != m_shmref.end(); ++itr)
    {
      mi::Uint32 shmname_size = mi::Uint32(itr->first.size());
      serializer->write(&shmname_size, 1);
      serializer->write(reinterpret_cast<const mi::Uint8*>(itr->first.c_str()), shmname_size);

      serializer->write(&itr->second, 1);
    }
  }

  // Serialize gpu ids.
  {
    const mi::Size nb_elements = m_gpuids.size();
    serializer->write(&nb_elements, 1);

    for (mi::Uint32 i = 0; i < nb_elements; ++i)
      serializer->write(&m_gpuids[i], 1);
  }

  // Serialize shmlist.
  {
    const mi::Size nb_elements = m_shmlist.size();
    serializer->write(&nb_elements, 1);

    std::map<mi::Uint32, std::vector<shm_info> >::const_iterator shmit = m_shmlist.begin();
    for (; shmit != m_shmlist.end(); ++shmit)
    {
      std::vector<shm_info> shmlist = shmit->second;

      const mi::Size shmlist_size = shmlist.size();
      serializer->write(&shmlist_size, 1);

      for (mi::Uint32 i = 0; i < shmlist_size; ++i)
      {
        // shm bbox
        const shm_info& shm = shmlist[i];
        serializer->write(&shm.m_shm_bbox.min.x, 6);

        // shm name
        mi::Uint32 shmname_size = mi::Uint32(shm.m_shm_name.size());
        serializer->write(&shmname_size, 1);
        serializer->write(reinterpret_cast<const mi::Uint8*>(shm.m_shm_name.c_str()), shmname_size);

        // shm size
        serializer->write(&shm.m_size, 1);

        // shm raw pointer
        mi::Uint32 shm_ptr_size = sizeof(void*);
        serializer->write(&shm_ptr_size, 1);
        serializer->write(
          reinterpret_cast<const mi::Uint8*>(&(shm.m_raw_mem_pointer)), shm_ptr_size);
      }

      // Serialize the time step.
      serializer->write(&shmit->first, 1);
    }
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_host_properties::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  // Deserialize rank id to host id.
  {
    mi::Size nb_elements = 0;
    deserializer->read(&nb_elements, 1);

    for (mi::Uint32 i = 0; i < nb_elements; ++i)
    {
      mi::Uint32 shmname_size = 0;
      deserializer->read(&shmname_size, 1);
      std::string shmname;
      shmname.resize(shmname_size);
      deserializer->read(reinterpret_cast<mi::Uint8*>(&shmname[0]), shmname_size);

      mi::Uint32 refcount = 0;
      deserializer->read(&refcount, 1);
      m_shmref[shmname] = refcount;
    }
  }

  // Deserialize the gpu ids.
  {
    mi::Size nb_elements = 0;
    deserializer->read(&nb_elements, 1);

    for (mi::Uint32 i = 0; i < nb_elements; ++i)
    {
      mi::Sint32 gpuid = 0;
      deserializer->read(&gpuid, 1);
      m_gpuids.push_back(gpuid);
    }
  }

  // Deserialize shmlist.
  {
    mi::Size nb_elements = 0;
    deserializer->read(&nb_elements, 1);

    for (mi::Uint32 i = 0; i < nb_elements; ++i)
    {
      mi::Size shmlist_size = 0;
      deserializer->read(&shmlist_size, 1);

      std::vector<shm_info> shmlist;

      for (mi::Uint32 j = 0; j < shmlist_size; ++j)
      {
        // shm bbox
        shm_info shm;
        deserializer->read(&shm.m_shm_bbox.min.x, 6);

        // shm name
        mi::Uint32 shmname_size = 0;
        deserializer->read(&shmname_size, 1);
        shm.m_shm_name.resize(shmname_size);
        deserializer->read(reinterpret_cast<mi::Uint8*>(&shm.m_shm_name[0]), shmname_size);

        // shm size
        deserializer->read(&shm.m_size, 1);

        // shm raw pointer
        mi::Uint32 shm_ptr_size = 0;
        deserializer->read(&shm_ptr_size, 1);
        deserializer->read(reinterpret_cast<mi::Uint8*>(&shm.m_raw_mem_pointer), shm_ptr_size);

        shmlist.push_back(shm);
      }

      // Deserialize the time step.
      mi::Uint32 time_step = 0;
      deserializer->read(&time_step, 1);

      m_shmlist[time_step] = shmlist;
    }
  }
}
