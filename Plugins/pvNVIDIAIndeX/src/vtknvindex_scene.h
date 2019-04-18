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

#ifndef vtknvindex_scene_h
#define vtknvindex_scene_h

#include "vtkVolume.h"
#include <vector>

#include "vtknvindex_colormap_utility.h"
#include "vtknvindex_rtc_kernel_params.h"

class vtknvindex_cluster_properties;
class vtknvindex_instance;

// The class vtknvindex_scene represents NVIDIA IndeX's scene representation of ParaView's state
// scene.
// Through this class dataset, camera, light, colormap and configuration settings
// are created and updated in NVIDIA IndeX.

class vtknvindex_scene
{
public:
  // Dataset volume type.
  enum Volume_type
  {
    VOLUME_TYPE_REGULAR = 0,
    VOLUME_TYPE_IRREGULAR
  };

  vtknvindex_scene();
  ~vtknvindex_scene();

  // Set volume visibility
  void set_visibility(bool visibility);

  // Setup scene for rendering in NVIDIA IndeX.
  void create_scene(vtkRenderer* ren, vtkVolume* vol,
    const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
    Volume_type volume_type);

  // Determine if the scene has already been created.
  bool scene_created() const;

  // Update only the volume.
  void update_volume(const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
    Volume_type volume_type);

  // Update scene settings, parameters etc.
  void update_scene(vtkRenderer* ren, vtkVolume* vol,
    const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
    bool config_settings_changed, bool opacity_changed, bool slices_changed);

  // Update rtc kernels and/or parameter.
  void update_rtc_kernel(const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
    const vtknvindex_rtc_params_buffer& rtc_param_buffer, Volume_type volume_type,
    bool kernel_changed);

  // Propagate camera changes from ParaView to NVIDIA IndeX.
  void update_camera(
    vtkRenderer* ren, const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction);

  // Propagate colormap changes from ParaView to NVIDIA IndeX.
  void update_colormap(vtkVolume* vol,
    const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
    vtknvindex_regular_volume_properties* regular_volume_properties);

  // Edit/update config settings.
  void update_config_settings(
    const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction);

  // Update volume opacity.
  void update_volume_opacity(
    vtkVolume* vol, const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction);

  // Update slices.
  void update_slices(const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction);

  void update_compute(const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction);

  // Cluster properties from the representation.
  void set_cluster_properties(vtknvindex_cluster_properties* cluster_properties);

private:
  // Dump internal state of NVIDIA IndeX
  void export_session();

  mi::Float32 calculate_volume_reference_step_size(
    vtkVolume* vol, mi::Uint32 mode, mi::Float32 opacity) const;

  bool m_scene_created; // True if scene is already initialized.
  bool m_only_init;     // Some config settings need to be updated only once.
  bool m_is_parallel;   // True if parallel projection is used;

  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties gathered from ParaView.

  mi::neuraylib::Tag m_vol_properties_tag; // Volume properties tag.
  mi::neuraylib::Tag m_timeseries_tag;     // Time series tag.
  mi::neuraylib::Tag m_root_group_tag;     // Parent root group tag for volume and slices.
  mi::neuraylib::Tag m_static_group_tag;   // Volume parent group tag.
  mi::neuraylib::Tag m_volume_tag;         // Volume database tag.

  mi::neuraylib::Tag m_prog_se_mapping_tag;       // Kernel program scene element mapping
  mi::neuraylib::Tag m_rtc_program_params_tag;    // CUDA program parameter tag.
  mi::neuraylib::Tag m_rtc_program_tag;           // CUDA program tag.
  mi::neuraylib::Tag m_volume_compute_attrib_tag; // Regular volume compute attribute tag.

  mi::neuraylib::Tag m_sparse_volume_render_properties_tag; // Sparse volume render properties tag.
  std::vector<mi::neuraylib::Tag> m_plane_tags;             // Computed planes tags.

  vtknvindex_rtc_params_buffer m_volume_rtc_kernel; // The current volume CUDA code.

  vtknvindex_instance* m_index_instance; // global index instance pointer
};
#endif // vtknvindex_scene_h
