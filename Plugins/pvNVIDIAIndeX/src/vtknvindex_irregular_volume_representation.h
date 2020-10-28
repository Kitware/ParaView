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

#ifndef vtknvindex_irregular_volume_representation_h
#define vtknvindex_irregular_volume_representation_h

#include <mi/math/bbox.h>

#include "vtkIndeXRepresentationsModule.h"
#include "vtkPVConfig.h"
#include "vtkVolumeRepresentation.h"

#include "vtknvindex_rtc_kernel_params.h"

class vtkColorTransferFunction;
class vtkMultiProcessController;
class vtkOrderedCompositeDistributor;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkPVGeometryFilter;
class vtkPVLODVolume;
class vtkPVUpdateSuppressor;
class vtkResampleToImage;
class vtkUnstructuredDataDeliveryFilter;
class vtkUnstructuredGridVolumeMapper;
class vtkVolumeProperty;
class vtkVolumeRepresentationPreprocessor;

class vtknvindex_config_settings;
class vtknvindex_cluster_properties;
class vtknvindex_irregular_volume_mapper;

// The class vtknvindex_representation represents the base class for distributing and rendering
// of irregular volume data (unstructured volume grids) in NVIDIA IndeX.

class VTKINDEXREPRESENTATIONS_EXPORT vtknvindex_irregular_volume_representation
  : public vtkVolumeRepresentation
{
public:
  static vtknvindex_irregular_volume_representation* New();
  vtkTypeMacro(vtknvindex_irregular_volume_representation, vtkVolumeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Register a volume mapper with the representation.
  void AddVolumeMapper(const char* name, vtkUnstructuredGridVolumeMapper*);

  // Set the active volume mapper to use.
  virtual void SetActiveVolumeMapper(const char*);
  vtkUnstructuredGridVolumeMapper* GetActiveVolumeMapper();

  // The method vtkAlgorithm::ProcessRequest() is equivalent for all rendering passes.
  // The method is typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  // Overridden to propagate to the active representation.
  void SetVisibility(bool val) override;

  //***************************************************************************
  // Forwarded to vtkVolumeRepresentationPreprocessor
  void SetExtractedBlockIndex(unsigned int index);

  //***************************************************************************
  // Forwarded to vtkResampleToImage
  void SetSamplingDimensions(int dims[3])
  {
    this->SetSamplingDimensions(dims[0], dims[1], dims[2]);
  }

  void SetSamplingDimensions(int xdim, int ydim, int zdim);

  //***************************************************************************
  // Forwarded to vtkVolumeProperty and vtkProperty (when applicable).
  void SetScalarOpacityUnitDistance(double val);

  //@{
  /**
   * Specify whether or not to redistribute the data. The default is false
   * since that is the only way in general to guarantee correct rendering.
   * Can set to true if all rendered data sets are based on the same
   * data partitioning in order to save on the data redistribution.
   */
  vtkSetMacro(UseDataPartitions, bool);
  vtkGetMacro(UseDataPartitions, bool);
  //@}

  //
  // Configuration options set from ParaView GUI.
  // These are stored in vtknvindex_config_settings class.
  //

  // Set subcube size.
  void set_subcube_size(unsigned x, unsigned y, unsigned z);

  // Set subcube border size.
  void set_subcube_border(int border);

  // Set dump internal state of NVIDIA IndeX.
  void set_dump_internal_state(bool is_dump);

  // Set flag to log performance values.
  void set_log_performance(bool is_log);

  // Set CUDA code parameter.
  void set_volume_filter(int filter);

  // Set raycast step size factor
  void set_step_size_factor(double step_size_factor);

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

  void set_roi_minJ(double val);
  void set_roi_maxJ(double val);
  vtkGetVector2Macro(m_roi_range_J, double);

  void set_roi_minK(double val);
  void set_roi_maxK(double val);
  vtkGetVector2Macro(m_roi_range_K, double);

protected:
  vtknvindex_irregular_volume_representation();
  ~vtknvindex_irregular_volume_representation();

  // Fill input port information.
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Adds the representation to the view. This is called from
  // vtkView::AddRepresentation(). The sub classes should override this method.
  // Returns true if the addition succeeds.
  bool AddToView(vtkView* view) override;

  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  bool RemoveFromView(vtkView* view) override;

  // Passes on parameters to the active volume mapper.
  virtual void UpdateMapperParameters();

  vtkNew<vtkVolumeRepresentationPreprocessor> Preprocessor;
  vtkNew<vtknvindex_irregular_volume_mapper> DefaultMapper;

  double DataBounds[6];

  bool UseDataPartitions = false;

private:
  vtknvindex_irregular_volume_representation(
    const vtknvindex_irregular_volume_representation&) = delete;
  void operator=(const vtknvindex_irregular_volume_representation&) = delete;

  // Update current kernel
  void update_current_kernel();

  class vtkInternals;
  vtkInternals* Internals;

  // Region of interest in the GUI.
  double m_roi_range_I[2];
  double m_roi_range_J[2];
  double m_roi_range_K[2];

  vtkNew<vtkResampleToImage> ResampleToImageFilter;
  vtkMultiProcessController* m_controller;           // MPI controller from ParaView.
  vtknvindex_config_settings* m_app_config_settings; // Application side config settings.
  vtknvindex_cluster_properties*
    m_cluster_properties; // Cluster wide properties, refer class documentation.
  mi::math::Bbox_struct<mi::Float32, 3> m_roi_gui;    // Region of interest set in the GUI.
  mi::math::Bbox<mi::Float32, 3> m_volume_dimensions; // Cached volume dimensions

  // backup of original Image Reduction Factors
  mi::Sint32 m_still_image_reduction_factor;
  mi::Sint32 m_interactive_image_reduction_factor;

  mi::Float32 m_prev_time_step;

  // rtc kernel params
  vtknvindex_ivol_isosurface_params m_isosurface_params;
  vtknvindex_ivol_depth_enhancement_params m_depth_enhancement_params;
  vtknvindex_ivol_edge_enhancement_params m_edge_enhancement_params;
  vtknvindex_ivol_custom_params m_custom_params;
  std::string m_custom_kernel_filename;
  std::string m_custom_kernel_program;
};

#endif // vtknvindex_irregular_volume_representation_h
