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

#ifndef vtknvindex_volumemapper_h
#define vtknvindex_volumemapper_h

#include <map>
#include <set>
#include <string>

#include <nv/index/icamera.h>
#include <nv/index/icolormap.h>
#include <nv/index/iconfig_settings.h>
#include <nv/index/iscene.h>
#include <nv/index/isession.h>
#include <nv/index/iviewport.h>

#include "vtkCamera.h"
#include "vtkIndeXRepresentationsModule.h"
#include "vtkMultiProcessController.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkTransform.h"

#include "vtknvindex_colormap_utility.h"
#include "vtknvindex_performance_values.h"
#include "vtknvindex_rtc_kernel_params.h"
#include "vtknvindex_scene.h"

class vtknvindex_cluster_properties;
class vtknvindex_instance;

// The class vtknvindex_volumemapper is responsible for all NVIDIA IndeX data preparation, the scene
// creation,
// the update and rendering of regular volumes.

class VTKINDEXREPRESENTATIONS_EXPORT vtknvindex_volumemapper : public vtkSmartVolumeMapper
{
public:
  static vtknvindex_volumemapper* New();
  vtkTypeMacro(vtknvindex_volumemapper, vtkSmartVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtknvindex_volumemapper();
  ~vtknvindex_volumemapper();

  // Get dataset bounding box.
  double* GetBounds() override;
  using Superclass::GetBounds;

  // Set the whole volume bounds.
  void set_whole_bounds(const mi::math::Bbox<mi::Float64, 3> bounds);

  // Shutdown forward loggers, NVIDIA IndeX library and unload libraries.
  void shutdown();

  // Overriding from vtkSmartVolumeMapper.
  void Render(vtkRenderer* ren, vtkVolume* vol) override;

  // Prepare data for the importer.
  bool prepare_data(mi::Sint32 time_step);

  // Get the local host id from NVIDIA IndeX running on this machine.
  mi::Sint32 get_local_hostid();

  // Set the cluster properties from Representation.
  void set_cluster_properties(vtknvindex_cluster_properties* cluster_properties);

  // Returns true if NVIDIA IndeX is initialized by this mapper.
  bool is_mapper_initialized() { return m_is_mapper_initialized; }

  // Update render canvas.
  void update_canvas(vtkRenderer* ren);

  // The configuration settings needs to be updated on changes applied in the GUI.
  void config_settings_changed();

  // The volume opacity needs to be updated on changes applied in the GUI.
  void opacity_changed();

  // Slices need to be updated on changes applied in the GUI.
  void slices_changed();

  // The CUDA code need to be updated on changes applied in the GUI.
  void rtc_kernel_changed(vtknvindex_rtc_kernels kernel, const std::string& kernel_program,
    const void* params_buffer, mi::Uint32 buffer_size);

  // Initialize the mapper.
  bool initialize_mapper(vtkVolume* vol);

  // Set/get caching state.
  void is_caching(bool is_caching);
  bool is_caching() const;

  // Set volume visibility
  void set_visibility(bool visibility);

private:
  vtknvindex_volumemapper(const vtknvindex_volumemapper&) = delete;
  void operator=(const vtknvindex_volumemapper&) = delete;

  // Is data prepared for given time step?
  bool is_data_prepared(mi::Sint32 time_step);

  bool m_is_caching;              // True when ParaView is caching data on animation loops.
  bool m_is_mapper_initialized;   // True if mapper was initialized.
  bool m_config_settings_changed; // True if some parameter changed on the GUI.
  bool m_opacity_changed;         // True if volume opacity changed.
  bool m_slices_changed;          // True if any slice parameter changed.
  bool m_volume_changed;          // True when switching between properties.

  bool m_rtc_kernel_changed; // True when switching between CUDA code.
  bool m_rtc_param_changed;  // True when a kernel parameter changed.

  vtkMTimeType m_last_MTime;   // last MTime when volume was modified
  std::string m_prev_property; // Volume property that was rendered.

  std::map<mi::Sint32, bool>
    m_time_step_data_prepared; // Is data for given frame ready for importer?
  vtknvindex_scene m_scene;    // NVIDIA IndeX scene.
  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties gathered from ParaView.
  vtknvindex_performance_values m_performance_values;  // Performance values logger.
  vtkMultiProcessController* m_controller;             // MPI controller from ParaView.
  bool m_is_mpi_rendering;          // True when multiple MPI ranks are used for rendering.
  vtkDataArray* m_scalar_array;     // Scalar array containing actual data.
  std::vector<void*> m_subset_ptrs; // Array with pointers to scalars data for all time steps.

  mi::Float64 m_whole_bounds[6]; // Whole volume bounds.

  std::set<std::string> m_data_array_warning_printed; // A warning was already printed for these

  vtknvindex_rtc_params_buffer m_volume_rtc_kernel; // The CUDA code applied to the current volume.

  vtknvindex_instance* m_index_instance; // global index instance pointer
};

#endif
