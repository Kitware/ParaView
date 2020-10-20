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

#ifndef vtknvindex_affinity_h
#define vtknvindex_affinity_h

#include <fstream>
#include <map>
#include <vector>

#include "vtkBoundingBox.h" // needed for vtkBoundingBox.

#include <mi/math/bbox.h>
#include <mi/neuraylib/iserializer.h>

#include <nv/index/version.h>

#if (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR <= 327600)
#include <mi/neuraylib/istring.h>
#endif

#include <nv/index/iaffinity_information.h>

#if (NVIDIA_INDEX_LIBRARY_REVISION_MAJOR > 327600)
#define VTKNVINDEX_USE_KDTREE
#endif

class vtknvindex_host_properties;

// vtknvindex_affinity stores ParaView's spatial subdivision.
//
// The affinity information is the mapping from ParaView's spatial subdivision subregions (bounding
// boxes) to its
// cluster location/resource, i.e., hosts (id), gpus (id). This information is used by NVIDIA IndeX
// to distribute
// evenly datasets and rendering among cluster's gpu resources.

class vtknvindex_affinity : public mi::neuraylib::Base<0x9bda367e, 0x755d, 0x47a4, 0xb5, 0x2f, 0xe1,
                              0x0f, 0xd7, 0xae, 0x34, 0x90, nv::index::IDomain_specific_subdivision>
{
public:
  vtknvindex_affinity();

  // Single affinity mapping from bbox to host_id, gpu_id.
  struct affinity_struct
  {
    affinity_struct()
      : m_host_id(~0u)
      , m_gpu_id(~0u)
    {
    }

    affinity_struct(
      const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 host_id, mi::Uint32 gpu_id)
      : m_bbox(bbox)
      , m_host_id(host_id)
      , m_gpu_id(gpu_id)
    {
    }

    mi::math::Bbox<mi::Float32, 3> m_bbox;
    mi::Uint32 m_host_id;
    mi::Uint32 m_gpu_id;
  };

  // Reset all affinity information.
  void reset_affinity();

  // Create a copy of this instance.
  vtknvindex_affinity* copy() const;

  // Add ParaView's affinity information indicating where the data is located for a given bbox.
  void add_affinity(
    const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 host_id, mi::Uint32 gpu_id = ~0u);

  // Get the set affinity information for a given bbox.
  bool get_affinity(const mi::math::Bbox_struct<mi::Float32, 3>& subregion, mi::Uint32& host_id,
    mi::IString* host_name, mi::Uint32& gpu_id) const override;

  // Get the number of subregions produced by NVIDIA IndeX.
  mi::Uint32 get_nb_subregions() const override;

  // Get the bounding box associated to a subregion.
  mi::math::Bbox_struct<mi::Float32, 3> get_subregion(mi::Uint32 index) const override;

  // Print the affinity information as part of the scene dump.
  void scene_dump_affinity_info(std::ostringstream& s) const;

  // DiCE methods
  void serialize(mi::neuraylib::ISerializer* serializer) const override;
  void deserialize(mi::neuraylib::IDeserializer* deserializer) override;

private:
  std::vector<affinity_struct>
    m_spatial_subdivision; // List of bbox to gpu id/host id mapping from ParaView.
};

class vtknvindex_KDTree_affinity
  : public mi::neuraylib::Base<0x357d6811, 0x7208, 0x4ab3, 0xa6, 0x2f, 0xff, 0xd8, 0xcf, 0x75, 0x1b,
      0x2, nv::index::IDomain_specific_subdivision_topology>
{
public:
  vtknvindex_KDTree_affinity();

  // Single affinity mapping from bbox to host_id, gpu_id.
  struct affinity_struct
  {
    affinity_struct()
      : m_host_id(~0u)
      , m_gpu_id(~0u)
    {
    }

    affinity_struct(
      const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 host_id, mi::Uint32 gpu_id)
      : m_bbox(bbox)
      , m_host_id(host_id)
      , m_gpu_id(gpu_id)
    {
    }

    mi::math::Bbox<mi::Float32, 3> m_bbox;
    mi::Uint32 m_host_id;
    mi::Uint32 m_gpu_id;
  };

  // Single kd-tree node struct
  struct kd_node
  {
    kd_node()
      : m_nodeID(-1)
    {
      m_childs[0] = m_childs[1] = -1;
    }

    mi::math::Bbox<mi::Float32, 3> m_bbox;
    mi::Sint32 m_nodeID;
    mi::Sint32 m_childs[2];
  };

  // Reset all affinity information.
  void reset_affinity();

  // Create a copy of this instance.
  vtknvindex_KDTree_affinity* copy() const;

  // Add ParaView's affinity information indicating where the data is located for a given bbox.
  void add_affinity(
    const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 host_id, mi::Uint32 gpu_id = ~0u);

  // Get the set affinity information for a given bbox.
  bool get_affinity(const mi::math::Bbox_struct<mi::Float32, 3>& subregion, mi::Uint32& host_id,
    mi::IString* host_name, mi::Uint32& gpu_id) const override;

  // Get the number of subregions produced by NVIDIA IndeX.
  mi::Uint32 get_nb_subregions() const override;

  // Get the bounding box associated to a subregion.
  mi::math::Bbox_struct<mi::Float32, 3> get_subregion(mi::Uint32 index) const override;

  // Get type of topology.
  mi::Uint32 get_topology_type() const override;

  // Get the total number of nodes of the topology.
  mi::Uint32 get_nb_nodes() const override;

  // Get the bounding box of the inode.
  mi::math::Bbox_struct<mi::Float32, 3> get_node_box(mi::Uint32 inode) const override;

  // Get the number of children of an inode.
  mi::Uint32 get_node_child_count(mi::Uint32 inode) const override;

  // Get a child index of the inode. Return -1 if no child at given ichild slot.
  mi::Sint32 get_node_child(mi::Uint32 inode, mi::Uint32 ichild) const override;

  // Get index of subregion associated with inode (or -1, if no subregion).
  mi::Sint32 get_node_subregion_index(mi::Uint32 inode) const override;

  // Builds kd-tree uses for IndeX kd-tree affinity based on raw cuts.
  void build(const std::vector<vtkBoundingBox>& raw_cuts, const std::vector<int>& raw_cuts_ranks);

  // Print the affinity information as part of the scene dump.
  void scene_dump_affinity_info(std::ostringstream& s) const;

  // DiCE methods
  void serialize(mi::neuraylib::ISerializer* serializer) const override;
  void deserialize(mi::neuraylib::IDeserializer* deserializer) override;

private:
  // Recursively build kd-subtree from raw cuts.
  mi::Sint32 build_internal(const std::vector<mi::math::Bbox<mi::Float32, 3> >& bboxes,
    const std::vector<int>& ranks, size_t pos, size_t count, int& leaf_count);

  std::vector<affinity_struct>
    m_spatial_subdivision; // List of bbox to gpu id/host id mapping from ParaView.

  std::vector<kd_node> m_kdtree; // ParaView kd-tree nodes cached in an array.
};

#endif
