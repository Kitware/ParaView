// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtknvindex_irregular_volume_importer_h
#define vtknvindex_irregular_volume_importer_h

#include <mi/dice.h>
#include <nv/index/idistributed_data_import_callback.h>

class vtknvindex_cluster_properties;

// The class vtknvindex_volume_importer represents a distributed data importer for NVIDIA IndeX.
// It load ParaView's subsets of a irregular volume dataset based shared memory.
class vtknvindex_irregular_volume_importer
  : public nv::index::Distributed_continuous_data_import_callback<0xa034b89a, 0xdd90, 0x464d, 0x85,
      0x9, 0x5f, 0xef, 0x93, 0xb8, 0x2d, 0x96>
{
public:
  vtknvindex_irregular_volume_importer();
  virtual ~vtknvindex_irregular_volume_importer();

  vtknvindex_irregular_volume_importer(const std::string& scalar_type);

  // Estimates the size (in byte) of the volume data contained in the bounding box.
  mi::Size estimate(const mi::math::Bbox_struct<mi::Float32, 3>& bounding_box,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;
  using Self::estimate; // handle overloaded method

  // Sets the cluster properties triggered by ParaView.
  void set_cluster_properties(vtknvindex_cluster_properties* host_properties);

  // Create the internal storage of ParaView's subset inside bounding box.
  nv::index::IDistributed_data_subset* create(
    const mi::math::Bbox_struct<mi::Float32, 3>& bounding_box,
    nv::index::IData_subset_factory* factory,
    mi::neuraylib::IDice_transaction* dice_transaction) const override;
  using Self::create; // handle overloaded method

  // DiCE methods
  void serialize(mi::neuraylib::ISerializer* serializer) const override;
  void deserialize(mi::neuraylib::IDeserializer* deserializer) override;
  mi::base::Uuid subset_id() const override;

  std::string m_scalar_type;                           // Volume's scalar type as string.
  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties.
};

#endif // vtknvindex_irregular_volume_importer_h
