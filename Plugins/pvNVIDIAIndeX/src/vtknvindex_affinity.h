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

#ifndef vtknvindex_affinity_h
#define vtknvindex_affinity_h

#include <fstream>
#include <map>
#include <vector>

#include <mi/base/interface_declare.h>
#include <mi/dice.h>
#include <mi/math/bbox.h>
#include <mi/neuraylib/iserializer.h>
#include <nv/index/iaffinity_information.h>

class vtknvindex_host_properties;

// vtknvindex_affinity stores ParaView's spatial subdivision.
//
// The affinity information is the mapping from ParaView's spatial subdivision subregions (bounding
// boxes) to its
// cluster location/resource, i.e., hosts (id), gpus (id). This information is used by NVIDIA IndeX
// to distribute
// evenly datasets and rendering among cluster's gpu resources.

class vtknvindex_affinity_base
  : public mi::base::Interface_declare<0x9bda367e, 0x755d, 0x47a4, 0xb5, 0x2f, 0xe1, 0x0f, 0xd7,
      0xae, 0x34, 0x90, nv::index::IDomain_specific_subdivision>
{
};

class vtknvindex_affinity : public mi::base::Interface_implement<vtknvindex_affinity_base>
{
public:
  vtknvindex_affinity();
  virtual ~vtknvindex_affinity();

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

  // Set the host properties.
  void set_hostinfo(std::map<mi::Uint32, vtknvindex_host_properties*>& host_info);

  // Reset all affinity information.
  void reset_affinity();

  // Add ParaView's affinity information indicating where the data is located for a given bbox.
  void add_affinity(
    const mi::math::Bbox<mi::Float32, 3>& bbox, mi::Uint32 host_id = ~0u, mi::Uint32 gpu_id = ~0u);

  // Get the set affinity information for a given bbox.
  bool get_affinity(const mi::math::Bbox_struct<mi::Float32, 3>& subregion, mi::Uint32& host_id,
    mi::Uint32& gpu_id) const override;

  // Get the number of subregions produced by NVIDIA IndeX.
  mi::Uint32 get_nb_subregions() const override;

  // Get the bounding box associated to a subregion.
  mi::math::Bbox_struct<mi::Float32, 3> get_subregion(mi::Uint32 index) const override;

  // Print the affinity information as part of the scene dump.
  void scene_dump_affinity_info(std::ostringstream& s);

  // DiCE methods
  mi::base::Uuid get_class_id() const override;
  void serialize(mi::neuraylib::ISerializer* serializer) const override;
  void deserialize(mi::neuraylib::IDeserializer* deserializer) override;

private:
  // Get a gpu id for the given host using robin-round scheme.
  bool get_gpu_id(mi::Sint32 host_id, mi::Uint32& gpu_id) const;

  mutable std::map<mi::Sint32, mi::Sint32>
    m_roundrobin_ids; // Used in around-robin scheme to return gpu ids.
  mutable std::vector<affinity_struct> m_final_spatial_subdivision; // Used only for the scene dump.
  std::vector<affinity_struct>
    m_spatial_subdivision; // List of bbox to gpu id/host id mapping from ParaView.
  std::map<mi::Uint32, vtknvindex_host_properties*>
    m_host_info; // The host id to host properties mapping.
};

#endif
