// SPDX-FileCopyrightText: Copyright (c) Copyright 2021 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtknvindex_volume_compute_h
#define vtknvindex_volume_compute_h

#include <string>
#include <vector>

#include <nv/index/idistributed_compute_destination_buffer.h>
#include <nv/index/idistributed_compute_technique.h>

#include <mi/math/vector.h>

class vtknvindex_cluster_properties;
/// Example volume compute technique
class vtknvindex_volume_compute
  : public nv::index::Distributed_compute_technique<0x83e770a4, 0x4270, 0x40fc, 0x8d, 0xc5, 0x61,
      0xda, 0xa, 0xdb, 0xc7, 0x78>
{
public:
  /// Empty default constructor for serialization
  vtknvindex_volume_compute();
  /// Constructor
  vtknvindex_volume_compute(const mi::math::Vector_struct<mi::Uint32, 3>& volume_size,
    mi::Sint32 border_size, const mi::Sint32& ghost_levels, std::string scalar_type,
    mi::Sint32 scalar_components, vtknvindex_cluster_properties* cluster_properties);

  virtual void launch_compute(mi::neuraylib::IDice_transaction* dice_transaction,
    nv::index::IDistributed_compute_destination_buffer* dst_buffer) const;

  virtual void set_enabled(bool enable);
  virtual bool get_enabled() const;

  /// -------------------------------------------------------------------------------------------

  /// The copy needs to create and return a newly allocated object of the same
  /// class and has to copy all the fields which should be the same in the copy.
  ///
  /// \return     The new copy of the database element.
  ///
  virtual mi::neuraylib::IElement* copy() const;

  /// Get a human readable representation of the class name.
  ///
  virtual const char* get_class_name() const;

  /// Serialize the class to the given serializer.
  ///
  /// \param serializer       Write to this serializer.
  ///
  virtual void serialize(mi::neuraylib::ISerializer* serializer) const;

  /// Deserialize the class from the given deserializer.
  ///
  /// \param deserializer     Read from this deserializer.
  ///
  virtual void deserialize(mi::neuraylib::IDeserializer* deserializer);

private:
  bool m_enabled;

  mi::Sint32 m_border_size;                            // subcube border size.
  mi::Sint32 m_ghost_levels;                           // VTK ghost levels.
  std::string m_scalar_type;                           // Volume scalar type as string.
  mi::Sint32 m_scalar_components;                      // Number of components.
  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties.
  mi::math::Vector<mi::Uint32, 3> m_volume_size;       // Volume size.
};

#endif
