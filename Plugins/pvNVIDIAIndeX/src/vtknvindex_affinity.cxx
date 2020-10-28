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

#include "vtknvindex_affinity.h"

#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_host_properties.h"

#include <cassert>

// ------------------------------------------------------------------------------------------------
vtknvindex_affinity::vtknvindex_affinity()
{
  // empty
}

// ------------------------------------------------------------------------------------------------
mi::Uint32 vtknvindex_affinity::get_nb_subregions() const
{
  return static_cast<mi::Uint32>(m_spatial_subdivision.size());
}

// ------------------------------------------------------------------------------------------------
mi::math::Bbox_struct<mi::Float32, 3> vtknvindex_affinity::get_subregion(mi::Uint32 index) const
{
  return m_spatial_subdivision[index].m_bbox;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::scene_dump_affinity_info(std::ostringstream& s) const
{
  s << "## User-defined data affinity.\n"
    << "index::paraview_subdivision::nb_spatial_regions = " << m_spatial_subdivision.size() << "\n"
    << "index::paraview_subdivision::use_affinity_only = 0\n";

  for (mi::Size i = 0; i < m_spatial_subdivision.size(); ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];
    s << "index::paraview_subdivision::spatial_region_" << i << "::bbox = " << affinity.m_bbox.min.x
      << " " << affinity.m_bbox.min.y << " " << affinity.m_bbox.min.z << " "
      << affinity.m_bbox.max.x << " " << affinity.m_bbox.max.y << " " << affinity.m_bbox.max.z
      << "\n";
    s << "index::paraview_subdivision::affinity_information_" << i
      << "::host = " << affinity.m_host_id << "\n";

    if (affinity.m_gpu_id != ~0u)
      s << "index::paraview_subdivision::affinity_information_" << i
        << "::gpu = " << affinity.m_gpu_id << "\n";
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::reset_affinity()
{
  m_spatial_subdivision.clear();
}

// ------------------------------------------------------------------------------------------------
vtknvindex_affinity* vtknvindex_affinity::copy() const
{
  vtknvindex_affinity* new_affinity = new vtknvindex_affinity;
  new_affinity->m_spatial_subdivision = m_spatial_subdivision;
  return new_affinity;
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
  mi::Uint32& host_id, mi::IString* /*host_name*/, mi::Uint32& gpu_id) const
{
  const mi::math::Bbox<mi::Float32, 3> subregion(subregion_st);

  const mi::Size nb_elements = m_spatial_subdivision.size();
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];

    if (affinity.m_bbox.contains(subregion.min) && affinity.m_bbox.contains(subregion.max))
    {
      host_id = affinity.m_host_id;
      gpu_id = affinity.m_gpu_id;
      return true;
    }
  }

  ERROR_LOG << "The affinity of the queried subregion " << subregion << " was not found.";
  return false;
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::serialize(mi::neuraylib::ISerializer* serializer) const
{
  const mi::Size nb_elements = m_spatial_subdivision.size();
  serializer->write(&nb_elements);
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];
    serializer->write(&affinity.m_bbox.min.x, 6);
    serializer->write(&affinity.m_host_id);
    serializer->write(&affinity.m_gpu_id);
  }
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_affinity::deserialize(mi::neuraylib::IDeserializer* deserializer)
{
  mi::Size nb_elements = 0;
  deserializer->read(&nb_elements);
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    affinity_struct affinity;
    deserializer->read(&affinity.m_bbox.min.x, 6);
    deserializer->read(&affinity.m_host_id);
    deserializer->read(&affinity.m_gpu_id);
    m_spatial_subdivision.push_back(affinity);
  }
}

// ------------------------------------------------------------------------------------------------
vtknvindex_KDTree_affinity::vtknvindex_KDTree_affinity()
{
  // empty
}

// ------------------------------------------------------------------------------------------------
void vtknvindex_KDTree_affinity::reset_affinity()
{
  m_spatial_subdivision.clear();
  m_kdtree.clear();
}

// ------------------------------------------------------------------------------------------------
vtknvindex_KDTree_affinity* vtknvindex_KDTree_affinity::copy() const
{
  vtknvindex_KDTree_affinity* new_affinity = new vtknvindex_KDTree_affinity;
  new_affinity->m_spatial_subdivision = m_spatial_subdivision;
  new_affinity->m_kdtree = m_kdtree;
  return new_affinity;
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
  mi::IString* /*host_name*/, mi::Uint32& gpu_id) const
{
  const mi::math::Bbox<mi::Float32, 3> subregion(subregion_st);

  const mi::Size nb_elements = m_spatial_subdivision.size();
  for (mi::Size i = 0; i < nb_elements; ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];

    if (affinity.m_bbox.contains(subregion.min) && affinity.m_bbox.contains(subregion.max))
    {
      host_id = affinity.m_host_id;
      gpu_id = affinity.m_gpu_id;
      return true;
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
  return m_spatial_subdivision[index].m_bbox;
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
void vtknvindex_KDTree_affinity::scene_dump_affinity_info(std::ostringstream& s) const
{
  s << "## User-defined data affinity (kd-tree).\n"
    << "index::paraview_subdivision::nb_spatial_regions = " << m_spatial_subdivision.size() << "\n"
    << "index::paraview_subdivision::use_affinity_only = 0\n";

  for (mi::Size i = 0; i < m_spatial_subdivision.size(); ++i)
  {
    const affinity_struct& affinity = m_spatial_subdivision[i];
    s << "index::paraview_subdivision::spatial_region_" << i << "::bbox = " << affinity.m_bbox.min.x
      << " " << affinity.m_bbox.min.y << " " << affinity.m_bbox.min.z << " "
      << affinity.m_bbox.max.x << " " << affinity.m_bbox.max.y << " " << affinity.m_bbox.max.z
      << "\n";
    s << "index::paraview_subdivision::affinity_information_" << i
      << "::host = " << affinity.m_host_id << "\n";

    if (affinity.m_gpu_id != ~0u)
      s << "index::paraview_subdivision::affinity_information_" << i
        << "::gpu = " << affinity.m_gpu_id << "\n";
  }
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

  // serialize kd-tree cached data
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

  // deserialize kd-tree cached data
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
void vtknvindex_KDTree_affinity::build(
  const std::vector<vtkBoundingBox>& raw_cuts, const std::vector<int>& raw_cuts_ranks)
{
  m_kdtree.clear();

  if (raw_cuts.empty())
    return;

  const bool is_power_of_two = ((raw_cuts.size() & (raw_cuts.size() - 1)) == 0);
  if (!is_power_of_two)
  {
    ERROR_LOG << "vtknvindex_KDTree_affinity::build() got unexpected number of raw cuts: "
              << raw_cuts.size();
    return;
  }

  assert(raw_cuts.size() == raw_cuts_ranks.size());

  std::vector<mi::math::Bbox<mi::Float32, 3> > bboxes(raw_cuts.size());
  for (size_t i = 0; i < raw_cuts.size(); ++i)
  {
    mi::Float64 bounds[6];
    raw_cuts[i].GetBounds(bounds);
    bboxes[i] = mi::math::Bbox<mi::Float32, 3>(
      bounds[0], bounds[2], bounds[4], bounds[1], bounds[3], bounds[5]);

    // ERROR_LOG << "cut " << i << ": " << bboxes[i];
  }

  int leaf_count = 0;
  build_internal(bboxes, raw_cuts_ranks, 0, bboxes.size(), leaf_count);
}

// ------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_KDTree_affinity::build_internal(
  const std::vector<mi::math::Bbox<mi::Float32, 3> >& bboxes, const std::vector<int>& ranks,
  size_t pos, size_t count, int& leaf_count)
{
  // INFO_LOG << "build_internal " << pos << " / " << count;

  if (count == 0)
    return -1;

  bool same_rank = true;
  for (size_t i = 0; i < count; ++i)
  {
    if (ranks[pos + i] != ranks[pos])
      same_rank = false;
  }

  m_kdtree.push_back(kd_node());
  const mi::Uint32 node_idx = static_cast<mi::Uint32>(m_kdtree.size() - 1);

  if (same_rank)
  {
    // There is just one cut or all cuts have the same rank: Create a leaf node
    kd_node& node = m_kdtree[node_idx];

    node.m_nodeID = leaf_count;
    leaf_count++;

    for (size_t i = 0; i < count; ++i)
    {
      node.m_bbox.insert(bboxes[pos + i]);
      // INFO_LOG << "      + box " << pos + i << " = " << bboxes[pos + i];
    }

    // INFO_LOG << "  creating leaf " << leaf_count << ", bbox " << node.m_bbox;
  }
  else
  {
    // Create an inner node with children
    // INFO_LOG << "  creating children ....";

    mi::Sint32 child0 = build_internal(bboxes, ranks, pos, count / 2, leaf_count);
    mi::Sint32 child1 = build_internal(bboxes, ranks, pos + count / 2, count / 2, leaf_count);

    kd_node& node = m_kdtree[node_idx];
    node.m_childs[0] = child0;
    node.m_childs[1] = child1;

    if (child0 >= 0)
      node.m_bbox.insert(m_kdtree[child0].m_bbox);
    if (child1 >= 0)
      node.m_bbox.insert(m_kdtree[child1].m_bbox);

    // INFO_LOG << "  creating inner node from above children with bbox " << node.m_bbox;
  }

  return node_idx;
}
