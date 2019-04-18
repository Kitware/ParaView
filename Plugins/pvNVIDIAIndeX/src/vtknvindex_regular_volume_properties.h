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

#ifndef vtknvindex_regular_volume_properties_h
#define vtknvindex_regular_volume_properties_h

#include <string>

#include <mi/base/interface_implement.h>
#include <mi/math/bbox.h>

class vtknvindex_host_properties;
struct vtknvindex_irregular_volume_data;
class vtkUnstructuredGridBase;

class vtknvindex_regular_volume_properties_base
  : public mi::base::Interface_declare<0x18c04ecc, 0x0296, 0x42af, 0xa4, 0x1d, 0x35, 0xe1, 0x7e,
      0x46, 0x2d, 0x9b>
{
};

// The class vtknvindex_regular_volume_properties represents the complementary
// regular volume dataset information such as dimension, data type, time steps, etc.
class vtknvindex_regular_volume_properties
  : public mi::base::Interface_implement<vtknvindex_regular_volume_properties_base>
{

public:
  vtknvindex_regular_volume_properties();
  ~vtknvindex_regular_volume_properties();

  // Scalar type of the volumetric (vtkImageData) dataset.
  void set_scalar_type(std::string scalar_type);
  void get_scalar_type(std::string& scalar_type) const;

  // Voxel range of the volumetric (vtkImageData) dataset.
  void set_voxel_range(mi::math::Vector<mi::Float32, 2> voxel_range);
  void get_voxel_range(mi::math::Vector<mi::Float32, 2>& voxel_range) const;

  // Scalar range of the volumetric (vtkImageData) dataset.
  void set_scalar_range(mi::math::Vector<mi::Float32, 2> scalar_range);
  void get_scalar_range(mi::math::Vector<mi::Float32, 2>& scalar_range) const;

  // Size of the volumetric dataset.
  void set_volume_size(mi::math::Vector_struct<mi::Uint32, 3> volume_size);
  void get_volume_size(mi::math::Vector_struct<mi::Uint32, 3>& volume_size) const;

  // Extents of the volumetric dataset.
  void set_volume_extents(mi::math::Bbox<mi::Sint32, 3> volume_extents);
  void get_volume_extents(mi::math::Bbox<mi::Sint32, 3>& volume_extents) const;

  // Extents of the volumetric dataset.
  void set_ivol_volume_extents(mi::math::Bbox<mi::Float32, 3> volume_extents);
  void get_ivol_volume_extents(mi::math::Bbox<mi::Float32, 3>& volume_extents) const;

  // Translation, Scaling.
  void set_volume_translation(mi::math::Vector<mi::Float32, 3> translation);
  void get_volume_translation(mi::math::Vector<mi::Float32, 3>& translation) const;

  void set_volume_scaling(mi::math::Vector<mi::Float32, 3> scaling);
  void get_volume_scaling(mi::math::Vector<mi::Float32, 3>& scaling) const;

  // True if data is time series.
  bool is_timeseries_data() const;
  void set_is_timeseries_data(bool is_timeseries);

  // Set/get total number of time steps for time series data.
  void set_nb_time_steps(mi::Uint32 nb_time_steps);
  mi::Uint32 get_nb_time_steps() const;

  // Set/get current time step for time series data.
  void set_current_time_step(mi::Uint64 current_time_step);
  mi::Uint32 get_current_time_step() const;

  // Set/get starting time step.
  void set_time_step_start(mi::Uint32 time_step_start);
  mi::Uint32 get_time_step_start() const;

  // Write given volume data to shared memory.
  bool write_shared_memory(vtkDataArray* scalar_array, mi::Sint32* bounds,
    vtknvindex_host_properties* host_properties, mi::Uint32 current_timestep, bool use_shared_mem);

  // Write irregular volume data to shared memory.
  bool write_shared_memory(vtknvindex_irregular_volume_data* ivol_data,
    vtkUnstructuredGridBase* ugrid, vtknvindex_host_properties* host_properties,
    mi::Uint32 current_timestep);

  void print_info() const;

  // DiCE database element methods.
  virtual void serialize(mi::neuraylib::ISerializer* serializer) const;
  virtual void deserialize(mi::neuraylib::IDeserializer* deserializer);
  virtual mi::base::Uuid get_class_id() const;
  virtual const char* get_class_name() const;

private:
  vtknvindex_regular_volume_properties(const vtknvindex_regular_volume_properties&) = delete;
  void operator=(const vtknvindex_regular_volume_properties&) = delete;

  // Converts volume data from ParaView's zyx format to Index's xyz format
  template <typename T>
  void transform_zyx_to_xyz(T* pv_volume, T* shm_volume, mi::Sint32* bounds) const;

  bool m_is_timeseries_data;       // True if volume data is time series based.
  mi::Uint32 m_time_steps_written; // No. of time steps written into shared memory.
  mi::Uint32 m_nb_time_steps;      // Number of time steps.
  mi::Uint32 m_current_time_step;  // Current time step.
  mi::Uint32 m_time_step_start;    // Starting timestep.

  std::string m_scalar_type;                             // Volume's scalar type as string.
  mi::math::Bbox<mi::Sint32, 3> m_volume_extents;        // ParaView volume extents.
  mi::math::Bbox<mi::Float32, 3> m_ivol_volume_extents;  // ParaView irregular volume extents.
  mi::math::Vector<mi::Float32, 2> m_voxel_range;        // Voxel range of the Volume data.
  mi::math::Vector<mi::Float32, 2> m_scalar_range;       // Scalar range of the Volume data.
  mi::math::Vector_struct<mi::Uint32, 3> m_volume_size;  // Entire volume size.
  mi::math::Vector<mi::Float32, 3> m_volume_translation; // Volume translation.
  mi::math::Vector<mi::Float32, 3> m_volume_scaling;     // Volume scaling.
};

#endif
