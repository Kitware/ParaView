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

#ifndef vtknvindex_volume_compute_h
#define vtknvindex_volume_compute_h

#include <string>
#include <vector>

#include <nv/index/idistributed_compute_destination_buffer.h>
#include <nv/index/idistributed_compute_technique.h>

#include <mi/math/vector.h>

class vtknvindex_cluster_properties;
/// Example volume compute technique
class vtknvindex_volume_compute : public nv::index::Distributed_compute_technique<0x83e770a4,
                                    0x4270, 0x40fc, 0x8d, 0xc5, 0x61, 0xda, 0xa, 0xdb, 0xc7, 0x78>
{
public:
  /// Empty default constructor for serialization
  vtknvindex_volume_compute();
  /// Constructor
  vtknvindex_volume_compute(const mi::math::Vector_struct<mi::Uint32, 3>& volume_size,
    mi::Sint32 border_size, std::string scalar_type,
    vtknvindex_cluster_properties* cluster_properties);

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

  virtual void get_references(mi::neuraylib::ITag_set* result) const;

private:
  bool m_enabled;

  mi::Sint32 m_border_size;                            // subcube border size.
  std::string m_scalar_type;                           // Volume scalar type as string.
  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties.
  mi::math::Vector<mi::Uint32, 3> m_volume_size;       // Volume size.
};

#endif
