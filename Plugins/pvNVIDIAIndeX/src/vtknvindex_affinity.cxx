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

#include "vtkKdNode.h"

#include "vtknvindex_affinity.h"

#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_host_properties.h"

// ------------------------------------------------------------------------------------------------
vtknvindex_affinity::vtknvindex_affinity()
{
  // empty
}

// ------------------------------------------------------------------------------------------------
vtknvindex_affinity::~vtknvindex_affinity()
{
  // empty
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_affinity::get_nb_subregions() const
{
  return static_cast<mi::Uint32>(m_spatial_subdivision.size());
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::set_hostinfo(std::map<mi::Uint32, vtknvindex_host_properties*>& host_info)
{
  m_host_info = host_info;
  std::map<mi::Uint32, vtknvindex_host_properties*>::iterator it;

  // Prepare a vector that is used to return correct gpu ids later.
  // Based on a round-robin scheme.
  for (it = m_host_info.begin(); it != m_host_info.end(); it++)
    m_roundrobin_ids[it->first] = 0;
}

// ------------------------------------------------------------------------------------------------
mi::math::Bbox_struct<mi::Float32, 3> vtknvindex_affinity::get_subregion(mi::Uint32 index) const
{
  const affinity_struct& spatial_subdivision = m_spatial_subdivision[index];
  return spatial_subdivision.m_bbox;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::scene_dump_affinity_info(std::ostringstream& s)
{
  s << "\n"
    << "## User-defined data affinity."
    << "\n";
  s << "## Set the number of user-defined affinity information in the project file."
    << "\n";

  s << "index::domain_subdivision::nb_spatial_regions = " << m_final_spatial_subdivision.size()
    << "\n";
  s << "index::domain_subdivision::use_affinity_only = 0"
    << "\n";

  mi::Uint32 nb_elements = static_cast<mi::Uint32>(m_final_spatial_subdivision.size());
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    affinity_struct& affinity = m_final_spatial_subdivision[i];
    s << "index::domain_subdivision::spatial_region_" << i << "::bbox = " << affinity.m_bbox.min.x
      << " " << affinity.m_bbox.min.y << " " << affinity.m_bbox.min.z << " "
      << affinity.m_bbox.max.x << " " << affinity.m_bbox.max.y << " " << affinity.m_bbox.max.z
      << "\n";
    s << "index::domain_subdivision::affinity_information_" << i
      << "::host = " << affinity.m_host_id << "\n";
    s << "index::domain_subdivision::affinity_information_" << i << "::gpu = " << affinity.m_gpu_id
      << "\n";
  }

  s << "index::paraview_subdivision::nb_spatial_regions = " << m_spatial_subdivision.size() << "\n";
  s << "index::paraview_subdivision::use_affinity_only = 0"
    << "\n";

  nb_elements = static_cast<mi::Uint32>(m_spatial_subdivision.size());
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    affinity_struct& affinity = m_spatial_subdivision[i];
    s << "index::paraview_subdivision::spatial_region_" << i << "::bbox = " << affinity.m_bbox.min.x
      << " " << affinity.m_bbox.min.y << " " << affinity.m_bbox.min.z << " "
      << affinity.m_bbox.max.x << " " << affinity.m_bbox.max.y << " " << affinity.m_bbox.max.z
      << "\n";
    s << "index::paraview_subdivision::affinity_information_" << i
      << "::host = " << affinity.m_host_id << "\n";
    s << "index::paraview_subdivision::affinity_information_" << i
      << "::gpu = " << affinity.m_gpu_id << "\n";
  }
}

void vtknvindex_affinity::reset_affinity()
{
  m_spatial_subdivision.clear();
  m_final_spatial_subdivision.clear();
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::add_affinity(
  const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 host_id, mi::Uint32 gpu_id)
{
  const affinity_struct affinity(bbox, host_id, gpu_id);
  m_spatial_subdivision.push_back(affinity);
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_affinity::get_affinity(const mi::math::Bbox_struct<mi::Float32, 3>& subregion_st,
  mi::Uint32& host_id, mi::IString* host_name, mi::Uint32& gpu_id) const
{
  const mi::math::Bbox<mi::Float32, 3> subregion(subregion_st);

  const mi::Uint32 nb_elements = static_cast<mi::Uint32>(m_spatial_subdivision.size());
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];

    if (affinity.m_bbox.contains(subregion.min) && affinity.m_bbox.contains(subregion.max))
    {
      host_id = affinity.m_host_id;

      auto it = m_host_info.find(affinity.m_host_id);
      if (it != m_host_info.end())
      {
        const vtknvindex_host_properties* host_properties = it->second;
        host_name->set_c_str(host_properties->get_hostname().c_str());
      }

      if (get_gpu_id(host_id, gpu_id))
      {
        // This is stored here only for the scene dump.
        affinity_struct affinity_dump(subregion, host_id, gpu_id);
        m_final_spatial_subdivision.push_back(affinity_dump);

        return true;
      }
      return false;
    }
  }

  ERROR_LOG << "The affinity of the queried subregion " << subregion << " was not found.";
  return false;
}

// ------------------------------------------------------------------------------------------------
mi::base::Uuid vtknvindex_affinity::get_class_id() const
{
  return IID();
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::serialize(mi::neuraylib::ISerializer* serializer) const
{
  const mi::Uint32 nb_elements = static_cast<mi::Uint32>(m_spatial_subdivision.size());
  serializer->write(&nb_elements, 1);
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];
    serializer->write(&affinity.m_bbox.min.x, 6);
    serializer->write(&affinity.m_host_id, 1);
    serializer->write(&affinity.m_gpu_id, 1);
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  mi::Uint32 nb_elements = 0;
  deserializer->read(&nb_elements, 1);
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    affinity_struct affinity;
    deserializer->read(&affinity.m_bbox.min.x, 6);
    deserializer->read(&affinity.m_host_id, 1);
    deserializer->read(&affinity.m_gpu_id, 1);
    m_spatial_subdivision.push_back(affinity);
  }
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_affinity::get_gpu_id(mi::Sint32 host_id, mi::Uint32& gpu_id) const
{
  std::map<mi::Uint32, vtknvindex_host_properties*>::const_iterator it = m_host_info.find(host_id);
  if (it == m_host_info.end())
  {
    ERROR_LOG << "The host properties for the host (id: " << host_id << ") are not available.";
    return false;
  }

  const vtknvindex_host_properties* host_properties = it->second;
  std::vector<mi::Sint32> host_gpu_ids;
  host_properties->get_gpuids(host_gpu_ids);

  mi::Sint32 idx = m_roundrobin_ids[host_id] % host_gpu_ids.size();
  m_roundrobin_ids[host_id]++;
  gpu_id = host_gpu_ids[idx];

  return true;
}

// ------------------------------------------------------------------------------------------------
vtknvindex_KDTree_affinity::vtknvindex_KDTree_affinity()
{
  // empty
}

// ------------------------------------------------------------------------------------------------
vtknvindex_KDTree_affinity::~vtknvindex_KDTree_affinity()
{
  // empty
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_KDTree_affinity::set_hostinfo(
  std::map<mi::Uint32, vtknvindex_host_properties*>& host_info)
{
  m_host_info = host_info;
  std::map<mi::Uint32, vtknvindex_host_properties*>::iterator it;

  // Prepare a vector that is used to return correct gpu ids later.
  // Based on a round-robin scheme.
  for (it = m_host_info.begin(); it != m_host_info.end(); it++)
    m_roundrobin_ids[it->first] = 0;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_KDTree_affinity::reset_affinity()
{
  m_spatial_subdivision.clear();
  m_final_spatial_subdivision.clear();
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_KDTree_affinity::add_affinity(
  const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 host_id, mi::Uint32 gpu_id)
{
  const affinity_struct affinity(bbox, host_id, gpu_id);
  m_spatial_subdivision.push_back(affinity);
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_KDTree_affinity::get_affinity(
  const mi::math::Bbox_struct<mi::Float32, 3>& subregion_st, mi::Uint32& host_id,
  mi::IString* host_name, mi::Uint32& gpu_id) const
{
  const mi::math::Bbox<mi::Float32, 3> subregion(subregion_st);

  const mi::Uint32 nb_elements = static_cast<mi::Uint32>(m_spatial_subdivision.size());
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];

    if (affinity.m_bbox.contains(subregion.min) && affinity.m_bbox.contains(subregion.max))
    {
      host_id = affinity.m_host_id;

      auto it = m_host_info.find(affinity.m_host_id);
      if (it != m_host_info.end())
      {
        const vtknvindex_host_properties* host_properties = it->second;
        host_name->set_c_str(host_properties->get_hostname().c_str());
      }

      if (get_gpu_id(host_id, gpu_id))
      {
        // This is stored here only for the scene dump.
        affinity_struct affinity_dump(subregion, host_id, gpu_id);
        m_final_spatial_subdivision.push_back(affinity_dump);

        return true;
      }
      return false;
    }
  }

  ERROR_LOG << "The affinity of the queried subregion " << subregion << " was not found.";
  return false;
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_KDTree_affinity::get_nb_subregions() const
{
  return static_cast<mi::Uint32>(m_spatial_subdivision.size());
}

// ------------------------------------------------------------------------------------------------
mi::math::Bbox_struct<mi::Float32, 3> vtknvindex_KDTree_affinity::get_subregion(
  mi::Uint32 index) const
{
  const affinity_struct& spatial_subdivision = m_spatial_subdivision[index];
  return spatial_subdivision.m_bbox;
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_KDTree_affinity::get_topology_type() const
{
  return IDomain_specific_subdivision_topology::TOPO_KD_TREE;
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_KDTree_affinity::get_nb_nodes() const
{
  return static_cast<mi::Uint32>(m_kdtree.size());
}

// ------------------------------------------------------------------------------------------------
mi::math::Bbox_struct<mi::Float32, 3> vtknvindex_KDTree_affinity::get_node_box(
  mi::Uint32 inode) const
{
  return m_kdtree[inode].m_bbox;
  // const affinity_struct& spatial_subdivision = m_spatial_subdivision[inode];
  // return spatial_subdivision.m_bbox;
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_KDTree_affinity::get_node_child_count(mi::Uint32) const
{
  return 2;
}

// ------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_KDTree_affinity::get_node_child(mi::Uint32 inode, mi::Uint32 ichild) const
{
  return m_kdtree[inode].m_childs[ichild];
}

// ------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_KDTree_affinity::get_node_subregion_index(mi::Uint32 inode) const
{
  return m_kdtree[inode].m_nodeID;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_KDTree_affinity::scene_dump_affinity_info(std::ostringstream& s)
{
  s << "\n"
    << "## User-defined data affinity."
    << "\n";
  s << "## Set the number of user-defined affinity information in the project file."
    << "\n";

  s << "index::domain_subdivision::nb_spatial_regions = " << m_final_spatial_subdivision.size()
    << "\n";
  s << "index::domain_subdivision::use_affinity_only = 0"
    << "\n";

  mi::Uint32 nb_elements = static_cast<mi::Uint32>(m_final_spatial_subdivision.size());
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    affinity_struct& affinity = m_final_spatial_subdivision[i];
    s << "index::domain_subdivision::spatial_region_" << i << "::bbox = " << affinity.m_bbox.min.x
      << " " << affinity.m_bbox.min.y << " " << affinity.m_bbox.min.z << " "
      << affinity.m_bbox.max.x << " " << affinity.m_bbox.max.y << " " << affinity.m_bbox.max.z
      << "\n";
    s << "index::domain_subdivision::affinity_information_" << i
      << "::host = " << affinity.m_host_id << "\n";
    s << "index::domain_subdivision::affinity_information_" << i << "::gpu = " << affinity.m_gpu_id
      << "\n";
  }

  s << "index::paraview_subdivision::nb_spatial_regions = " << m_spatial_subdivision.size() << "\n";
  s << "index::paraview_subdivision::use_affinity_only = 0"
    << "\n";

  nb_elements = static_cast<mi::Uint32>(m_spatial_subdivision.size());
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    affinity_struct& affinity = m_spatial_subdivision[i];
    s << "index::paraview_subdivision::spatial_region_" << i << "::bbox = " << affinity.m_bbox.min.x
      << " " << affinity.m_bbox.min.y << " " << affinity.m_bbox.min.z << " "
      << affinity.m_bbox.max.x << " " << affinity.m_bbox.max.y << " " << affinity.m_bbox.max.z
      << "\n";
    s << "index::paraview_subdivision::affinity_information_" << i
      << "::host = " << affinity.m_host_id << "\n";
    s << "index::paraview_subdivision::affinity_information_" << i
      << "::gpu = " << affinity.m_gpu_id << "\n";
  }
}

// ------------------------------------------------------------------------------------------------
mi::base::Uuid vtknvindex_KDTree_affinity::get_class_id() const
{
  return IID();
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_KDTree_affinity::serialize(mi::neuraylib::ISerializer* serializer) const
{
  // serialize affinity information
  const mi::Uint32 nb_elements = static_cast<mi::Uint32>(m_spatial_subdivision.size());
  serializer->write(&nb_elements, 1);
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];
    serializer->write(&affinity.m_bbox.min.x, 6);
    serializer->write(&affinity.m_host_id, 1);
    serializer->write(&affinity.m_gpu_id, 1);
  }

  // serialize kdtree cached data
  const mi::Uint32 nb_nodes = static_cast<mi::Uint32>(m_kdtree.size());
  serializer->write(&nb_nodes, 1);
  for (mi::Size i = 0; i < nb_nodes; ++i)
  {
    const kd_node& node = m_kdtree[i];
    serializer->write(&node.m_bbox.min.x, 6);
    serializer->write(&node.m_nodeID, 1);
    serializer->write(&node.m_childs[0], 2);
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_KDTree_affinity::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  // deserialize affinity information
  mi::Uint32 nb_elements = 0;
  deserializer->read(&nb_elements, 1);
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    affinity_struct affinity;
    deserializer->read(&affinity.m_bbox.min.x, 6);
    deserializer->read(&affinity.m_host_id, 1);
    deserializer->read(&affinity.m_gpu_id, 1);
    m_spatial_subdivision.push_back(affinity);
  }

  // deserialize kdtree cached data
  mi::Uint32 nb_nodes = 0;
  deserializer->read(&nb_nodes, 1);
  for (mi::Size i = 0; i < nb_nodes; ++i)
  {
    kd_node node;
    deserializer->read(&node.m_bbox.min.x, 6);
    deserializer->read(&node.m_nodeID, 1);
    deserializer->read(&node.m_childs[0], 2);
    m_kdtree.push_back(node);
  }
}

// ------------------------------------------------------------------------------------------------
bool vtknvindex_KDTree_affinity::get_gpu_id(mi::Sint32 host_id, mi::Uint32& gpu_id) const
{
  std::map<mi::Uint32, vtknvindex_host_properties*>::const_iterator it = m_host_info.find(host_id);
  if (it == m_host_info.end())
  {
    ERROR_LOG << "The host properties for the host (id: " << host_id << ") are not available.";
    return false;
  }

  const vtknvindex_host_properties* host_properties = it->second;
  std::vector<mi::Sint32> host_gpu_ids;
  host_properties->get_gpuids(host_gpu_ids);

  mi::Sint32 idx = m_roundrobin_ids[host_id] % host_gpu_ids.size();
  m_roundrobin_ids[host_id]++;
  gpu_id = host_gpu_ids[idx];

  return true;
}

// ------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_KDTree_affinity::build_node(vtkKdNode* vtk_node)
{
  if (vtk_node == nullptr)
    return -1;

  m_kdtree.push_back(kd_node());
  mi::Uint32 node_idx = static_cast<mi::Uint32>(m_kdtree.size() - 1);

  m_kdtree[node_idx].m_nodeID = vtk_node->GetID();

  double bbox[6];
  vtk_node->GetBounds(bbox);
  m_kdtree[node_idx].m_bbox.min.x = static_cast<mi::Float32>(bbox[0]);
  m_kdtree[node_idx].m_bbox.min.y = static_cast<mi::Float32>(bbox[2]);
  m_kdtree[node_idx].m_bbox.min.z = static_cast<mi::Float32>(bbox[4]);
  m_kdtree[node_idx].m_bbox.max.x = static_cast<mi::Float32>(bbox[1]);
  m_kdtree[node_idx].m_bbox.max.y = static_cast<mi::Float32>(bbox[3]);
  m_kdtree[node_idx].m_bbox.max.z = static_cast<mi::Float32>(bbox[5]);

  // has childs
  if (m_kdtree[node_idx].m_nodeID == -1)
  {
    const mi::Sint32 left = build_node(vtk_node->GetLeft());
    const mi::Sint32 right = build_node(vtk_node->GetRight());
    m_kdtree[node_idx].m_childs[0] = left;
    m_kdtree[node_idx].m_childs[1] = right;
  }
  else
  {
    m_kdtree[node_idx].m_childs[0] = m_kdtree[node_idx].m_childs[1] = -1;
  }

  // ERROR_LOG << "NODE: " << node_idx << ", ID: " << m_kdtree[node_idx].m_nodeID
  //        << ", CH0: " << m_kdtree[node_idx].m_childs[0]
  //        << ", CH1: " << m_kdtree[node_idx].m_childs[1];

  return node_idx;
}
