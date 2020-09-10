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

#ifndef vtknvindex_representation_h
#define vtknvindex_representation_h

#include <map>

#include <mi/math/bbox.h>

#include "vtkImageVolumeRepresentation.h"
#include "vtkIndeXRepresentationsModule.h"
#include "vtkSmartPointer.h"

#include "vtknvindex_rtc_kernel_params.h"

class vtkMultiProcessController;

class vtknvindex_config_settings;
class vtknvindex_cluster_properties;

// The class vtknvindex_cached_bounds collects and holds the dataset boundary data used for
// time variant datasets if all data is already cached and no more boundary
// information can be queried.

struct vtknvindex_cached_bounds
{
  double data_bounds[6];
  vtknvindex_cached_bounds();
  vtknvindex_cached_bounds(const vtknvindex_cached_bounds& cached_bound);
  vtknvindex_cached_bounds(const double _data_bounds[6]);
};

// The class vtknvindex_representation represents the base class for distributing and rendering
// of regular volume data (structured volume grids) in NVIDIA IndeX.

class VTKINDEXREPRESENTATIONS_EXPORT vtknvindex_representation : public vtkImageVolumeRepresentation
{
public:
  static vtknvindex_representation* New();
  vtkTypeMacro(vtknvindex_representation, vtkImageVolumeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Overrides from vtkImageVolumeRepresentation.
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;

  // Overrides for correct ghost cell.
  int RequestDataBase(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Overrides from vtkPVDataRepresentation.
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Overwrite from vtkAlgorithm.
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name) override;
  void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, int fieldAttributeType) override;
  void SetInputArrayToProcess(int idx, vtkInformation* info) override;
  void SetInputArrayToProcess(int idx, int port, int connection, const char* fieldAssociation,
    const char* attributeTypeorName) override;

  // Get/Set the visibility for this representation. When the visibility of
  void SetVisibility(bool val) override;

  // Forwarded to vtkProperty (when applicable).
  void SetScalarOpacityUnitDistance(double val);

  //
  // Configuration options set by ParaView GUI.
  // These values are stored in an instance of the vtknvindex_config_settings class.
  //

  // NVIDIA IndeX's regular volume parameters.
  // Set subcube size.
  void set_subcube_size(unsigned x, unsigned y, unsigned z);

  // Set subcube border size.
  void set_subcube_border(int border);

  // Set filtering mode.
  void set_filter_mode(int filter_mode);

  // Set pre-integration mode.
  void set_preintegration(bool enable_preint);

  // Set volume step size.
  void set_volume_step_size(double step_size);

  // Set opacity reference mode.
  void set_opacity_reference_mode(int opacity_reference_mode);

  // Set opacity reference.
  void set_opacity_reference(double opacity_reference);

  // Set dump internal state of NVIDIA IndeX.
  void set_dump_internal_state(bool is_dump);

  // Set flag to log performance values.
  void set_log_performance(bool is_log);

  // Set slice parameters.
  void show_volume(bool enable);
  void set_selected_slice(int selected_slice);

  void enable_slice1(bool enable);
  void set_slice_mode1(int slice_mode);
  void set_slice_pos1(double pos);
  void enable_slice2(bool enable);
  void set_slice_mode2(int slice_mode);
  void set_slice_pos2(double pos);
  void enable_slice3(bool enable);
  void set_slice_mode3(int slice_mode);
  void set_slice_pos3(double pos);

  // Set CUDA code parameter.
  void set_volume_filter(int filter);

  // Set common lighting parameter.
  void set_light_type(int light_type);
  void set_light_angle(double light_angle);
  void set_light_elevation(double light_elevation);

  // Set common specular parameters.
  void set_surf_ambient(double ambient);
  void set_surf_specular(double specular);
  void set_surf_specular_power(double specular_power);

  // Set iso-surface parameters.
  void set_iso_min(double iso_min);
  void set_iso_max(double iso_max);
  void set_iso_fill_mode(int mode);
  void set_iso_use_shading(bool use_shading);

  // Set depth enhancement parameters.
  void set_depth_samples(int depth_samples);
  void set_depth_gamma(double gamma);

  // Set edge enhancement parameters.
  void set_edge_range(double edge_range);
  void set_edge_samples(int edge_samples);

  // Set gradient parameters.
  void set_gradient_level(double gradient_level);
  void set_gradient_scale(double gradient_scale);

  // Set custom parameters.
  void set_kernel_filename(const char* kernel_filename);
  void set_kernel_update();
  void set_custom_pfloat_1(double custom_cf1);
  void set_custom_pfloat_2(double custom_cf2);
  void set_custom_pfloat_3(double custom_cf3);
  void set_custom_pfloat_4(double custom_cf4);
  void set_custom_pint_1(int ci1);
  void set_custom_pint_2(int ci2);
  void set_custom_pint_3(int ci3);
  void set_custom_pint_4(int ci4);

  // Set region of interest.
  void update_index_roi();

  void set_roi_minI(double val);
  void set_roi_maxI(double val);
  vtkGetVector2Macro(m_roi_range_I, double);
  vtkSetVector2Macro(m_roi_range_I, double);

  void set_roi_minJ(double val);
  void set_roi_maxJ(double val);
  vtkGetVector2Macro(m_roi_range_J, double);
  vtkSetVector2Macro(m_roi_range_J, double);

  void set_roi_minK(double val);
  void set_roi_maxK(double val);
  vtkGetVector2Macro(m_roi_range_K, double);
  vtkSetVector2Macro(m_roi_range_K, double);

protected:
  vtknvindex_representation();
  ~vtknvindex_representation();

private:
  vtknvindex_representation(const vtknvindex_representation&) = delete;
  void operator=(const vtknvindex_representation&) = delete;

  vtknvindex_cached_bounds* get_cached_bounds(mi::Sint32 time);
  void set_cached_bounds(mi::Sint32 time);

  // Utility function for searching the lower bound time stamp inside a sorted time steps list.
  mi::Sint32 find_time_step(
    mi::Float64 time, const mi::Float64* time_steps, mi::Sint32 nb_tsteps) const;

  // Update current kernel
  void update_current_kernel();

  // For the GUI
  double m_roi_range_I[2];
  double m_roi_range_J[2];
  double m_roi_range_K[2];

  vtkMultiProcessController* m_controller;           // MPI controller from ParaView.
  vtknvindex_config_settings* m_app_config_settings; // Application side config settings.
  vtknvindex_cluster_properties*
    m_cluster_properties; // Cluster wide properties, refer class documentation.
  mi::math::Bbox_struct<mi::Float32, 3> m_roi_gui; // Region of interest set in the GUI.
  mi::math::Vector<mi::Uint32, 3> m_volume_size;   // Cached volume size.

  // backup of original Image Reduction Factors
  mi::Sint32 m_still_image_reduction_factor;
  mi::Sint32 m_interactive_image_reduction_factor;

  // boundary caching for time series
  vtkSmartPointer<vtkPolyData> OutlineGeometry;
  mi::Sint32 m_has_time_steps;
  mi::Sint32 m_cur_time;
  std::map<mi::Sint32, vtknvindex_cached_bounds> m_time_to_cached_bounds;

  // rtc kernel params
  vtknvindex_isosurface_params m_isosurface_params;
  vtknvindex_depth_enhancement_params m_depth_enhancement_params;
  vtknvindex_edge_enhancement_params m_edge_enhancement_params;
  vtknvindex_gradient_params m_gradient_params;
  vtknvindex_custom_params m_custom_params;
  std::string m_custom_kernel_filename;
  std::string m_custom_kernel_program;
};

// Schwartz counter to manage initialization.
class VTKINDEXREPRESENTATIONS_EXPORT vtknvindex_representation_initializer
{
public:
  vtknvindex_representation_initializer();
  ~vtknvindex_representation_initializer();

private:
  vtknvindex_representation_initializer(const vtknvindex_representation_initializer&) = delete;
  void operator=(const vtknvindex_representation_initializer&) = delete;
};
static vtknvindex_representation_initializer vtknvindex_representation_initializer_instance;
#endif // vtknvindex_representation_h
