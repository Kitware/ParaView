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

#include <map>
#include <string>

#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderViewDataDeliveryManager.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkResampleToImage.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRepresentationPreprocessor.h"

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_config_settings.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_instance.h"
#include "vtknvindex_irregular_volume_mapper.h"
#include "vtknvindex_irregular_volume_representation.h"
#include "vtknvindex_utilities.h"

//----------------------------------------------------------------------------
class vtknvindex_irregular_volume_representation::vtkInternals
{
public:
  typedef std::map<std::string, vtkSmartPointer<vtkUnstructuredGridVolumeMapper> > MapOfMappers;
  MapOfMappers Mappers;
  std::string ActiveVolumeMapper;
};

vtkStandardNewMacro(vtknvindex_irregular_volume_representation);

//----------------------------------------------------------------------------
vtknvindex_irregular_volume_representation::vtknvindex_irregular_volume_representation()
  : Superclass()
{
  m_controller = vtkMultiProcessController::GetGlobalController();

  this->ResampleToImageFilter->SetSamplingDimensions(128, 128, 128);

  this->Internals = new vtkInternals();

  this->Preprocessor->SetTetrahedraOnly(1);

  this->Actor->SetMapper(this->DefaultMapper);

  // Create NVIDIA IndeX cluster properties and application settings.
  m_cluster_properties = new vtknvindex_cluster_properties(true);
  m_app_config_settings = m_cluster_properties->get_config_settings();

  this->DefaultMapper->set_cluster_properties(m_cluster_properties);

  // TODO: These values should be communicated by ParaView.
  // Currently there is no way to do this.
  m_roi_range_I[0] = -100.0;
  m_roi_range_I[1] = 100.0;
  m_roi_range_J[0] = -100.0;
  m_roi_range_J[1] = 100.0;
  m_roi_range_K[0] = -100.0;
  m_roi_range_K[1] = 100.0;

  m_prev_time_step = -1.0f;

  m_still_image_reduction_factor = 1;
  m_interactive_image_reduction_factor = 2;
}

//----------------------------------------------------------------------------
vtknvindex_irregular_volume_representation::~vtknvindex_irregular_volume_representation()
{
  this->DefaultMapper->shutdown();

  delete this->Internals;
  this->Internals = 0;

  delete m_cluster_properties;
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::AddVolumeMapper(
  const char* name, vtkUnstructuredGridVolumeMapper* mapper)
{
  this->Internals->Mappers[name] = mapper;
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetActiveVolumeMapper(const char* mapper)
{
  this->Internals->ActiveVolumeMapper = mapper ? mapper : "";
  this->MarkModified();
}

//----------------------------------------------------------------------------
vtkUnstructuredGridVolumeMapper* vtknvindex_irregular_volume_representation::GetActiveVolumeMapper()
{
  if (this->Internals->ActiveVolumeMapper != "")
  {
    vtkInternals::MapOfMappers::iterator iter =
      this->Internals->Mappers.find(this->Internals->ActiveVolumeMapper);
    if (iter != this->Internals->Mappers.end() && iter->second.GetPointer())
    {
      return iter->second.GetPointer();
    }
  }

  return this->DefaultMapper;
}

//----------------------------------------------------------------------------
int vtknvindex_irregular_volume_representation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGridBase");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtknvindex_irregular_volume_representation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    this->Preprocessor->SetInputConnection(this->GetInternalOutputPort());
    this->Preprocessor->Update();

    // Check for time series data info.
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    mi::Float32 cur_time_step = 0;
    int has_time_steps = inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    if (has_time_steps)
    {
      cur_time_step = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }

    // If the time step changed, then update volume data
    if (m_prev_time_step < 0.f)
    {
      m_prev_time_step = cur_time_step;
    }
    else
    {
      if (cur_time_step != m_prev_time_step)
      {
        // The volume needs to be reloaded.
        DefaultMapper->volume_changed();
        m_prev_time_step = cur_time_step;
      }
    }

    vtkDataSet* ds = vtkDataSet::SafeDownCast(this->Preprocessor->GetOutputDataObject(0));
    if (ds)
    {
      ds->GetBounds(this->DataBounds);
    }

    // Calculate global extends by gathering extents of all the pieces.
    mi::Sint32 num_processes = m_controller->GetNumberOfProcesses();
    mi::Float64* all_rank_extents = new mi::Float64[6 * num_processes];
    {
      m_controller->AllGather(this->DataBounds, all_rank_extents, 6);
    }

    mi::math::Bbox<mi::Float64, 3> whole_bounds; // Cached volume dimensions
    whole_bounds.clear();

    // volume dimensions
    for (mi::Sint32 i = 0, idx = 0; i < num_processes; i++, idx += 6)
    {
      mi::Float64* cur_extent = all_rank_extents + idx;
      mi::math::Bbox<mi::Float64, 3> cur_volume_dimensions;
      cur_volume_dimensions.min.x = cur_extent[0];
      cur_volume_dimensions.min.y = cur_extent[2];
      cur_volume_dimensions.min.z = cur_extent[4];
      cur_volume_dimensions.max.x = cur_extent[1];
      cur_volume_dimensions.max.y = cur_extent[3];
      cur_volume_dimensions.max.z = cur_extent[5];

      whole_bounds.insert(cur_volume_dimensions);
    }

    this->DefaultMapper->set_whole_bounds(whole_bounds);

    m_volume_dimensions.min = mi::Float32_3(whole_bounds.min);
    m_volume_dimensions.max = mi::Float32_3(whole_bounds.max);

    delete[] all_rank_extents;

    m_cluster_properties->get_regular_volume_properties()->set_ivol_volume_extents(
      m_volume_dimensions);

    // volume size
    update_index_roi();
  }
  else
  {
    this->Preprocessor->RemoveAllInputs();
    vtkNew<vtkUnstructuredGrid> placeholder;
    this->Preprocessor->SetInputData(0, placeholder.GetPointer());
    this->Preprocessor->Update();
  }

  m_controller->Barrier();

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtknvindex_irregular_volume_representation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetPiece(inInfo, this, this->Preprocessor->GetOutputDataObject(0));
    vtkPVRenderView::SetOrderedCompositingConfiguration(inInfo, this,
      vtkPVRenderView::DATA_IS_REDISTRIBUTABLE | vtkPVRenderView::USE_DATA_FOR_LOAD_BALANCING);
    vtkPVRenderView::SetRedistributionModeToDuplicateBoundaryCells(inInfo, this);

    vtkNew<vtkMatrix4x4> matrix;
    this->Actor->GetMatrix(matrix.GetPointer());
    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds, matrix.GetPointer());

    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {

    auto controller = vtkMultiProcessController::GetGlobalController();
    vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(inInfo->Get(vtkPVRenderView::VIEW()));
    auto ddm = vtkPVRenderViewDataDeliveryManager::SafeDownCast(view->GetDeliveryManager());

#ifdef VTKNVINDEX_USE_KDTREE
    if (m_controller->GetLocalProcessId() == 0)
    {
      DefaultMapper->set_raw_cuts(ddm->GetRawCuts(), ddm->GetRawCutsRankAssignments());
#if 0
      const std::vector<vtkBoundingBox>& raw_cuts = ddm->GetRawCuts();
      const std::vector<int>& raw_cuts_ranks = ddm->GetRawCutsRankAssignments();

      INFO_LOG << "Retrieved raw_cuts for process" << m_controller->GetLocalProcessId() << ": "
               << raw_cuts.size();
      for (size_t i = 0; i < raw_cuts.size(); ++i)
      {
        if (raw_cuts[i].IsValid())
        {
          mi::Float64 bbox[6];
          raw_cuts[i].GetBounds(bbox);
          INFO_LOG << "  raw_cuts[ " << i << "], rank " << raw_cuts_ranks[i] << ": " << bbox[0] << ", "
                   << bbox[2] << ", " << bbox[4] << "; " << bbox[1] << ", " << bbox[3] << ", "
                   << bbox[5];
        }
        else
        {
          INFO_LOG << "  raw_cuts[ " << i << "], rank " << raw_cuts_ranks[i] << ": invalid";
        }
      }
#endif
    }
#endif // VTKNVINDEX_USE_KDTREE

    // Retrieve ParaView's kd-tree in order to obtain domain subdivision bounding boxes.
    if (ddm->GetCuts().size() > 0 && controller != nullptr &&
      controller->GetLocalProcessId() < static_cast<int>(ddm->GetCuts().size()))
    {
      DefaultMapper->set_subregion_bounds(ddm->GetCuts()[controller->GetLocalProcessId()]);
    }
    else
    {
      DefaultMapper->set_subregion_bounds(vtkBoundingBox());
    }

    if (inInfo->Has(vtkPVRenderView::USE_LOD()))
    {
      this->Actor->SetEnableLOD(1);
    }
    else
    {
      this->Actor->SetEnableLOD(0);
    }

    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);

    vtkUnstructuredGridVolumeMapper* activeMapper = this->GetActiveVolumeMapper();
    activeMapper->SetInputConnection(producerPort);

    this->UpdateMapperParameters();
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtknvindex_irregular_volume_representation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    m_still_image_reduction_factor = rview->GetStillRenderImageReductionFactor();
    m_interactive_image_reduction_factor = rview->GetInteractiveRenderImageReductionFactor();
    rview->SetStillRenderImageReductionFactor(1);
    rview->SetInteractiveRenderImageReductionFactor(1);

    rview->GetRenderer()->AddActor(this->Actor);
    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtknvindex_irregular_volume_representation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->SetStillRenderImageReductionFactor(m_still_image_reduction_factor);
    rview->SetInteractiveRenderImageReductionFactor(m_interactive_image_reduction_factor);

    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::UpdateMapperParameters()
{
  vtkUnstructuredGridVolumeMapper* activeMapper = this->GetActiveVolumeMapper();
  const char* colorArrayName = NULL;
  int fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
  }

  activeMapper->SelectScalarArray(colorArrayName);

  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    default:
      activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
      break;
  }

  this->Actor->SetMapper(activeMapper);
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//***************************************************************************
// Forwarded to vtkVolumeRepresentationPreprocessor

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetExtractedBlockIndex(unsigned int index)
{
  this->Preprocessor->SetExtractedBlockIndex(index);
}

//***************************************************************************
// Forwarded to vtkResampleToImage
//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetSamplingDimensions(int xdim, int ydim, int zdim)
{
  this->ResampleToImageFilter->SetSamplingDimensions(xdim, ydim, zdim);
}

//***************************************************************************
// Forwarded to Actor.
//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetVisibility(bool val)
{
  DefaultMapper->set_visibility(val);
  update_index_roi();

  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetScalarOpacityUnitDistance(double val)
{
  static_cast<vtknvindex_irregular_volume_mapper*>(this->DefaultMapper)->opacity_changed();
  this->Superclass::SetScalarOpacityUnitDistance(val);
}

//
// Configuration options set from ParaView GUI.
//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_subcube_size(
  unsigned x, unsigned y, unsigned z)
{
  mi::math::Vector_struct<mi::Uint32, 3> subcube_size = { x, y, z };
  m_app_config_settings->set_subcube_size(subcube_size);
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_subcube_border(int border)
{
  m_app_config_settings->set_subcube_border(border);
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::update_index_roi()
{
  // Set region of interest.
  mi::math::Bbox<mi::Float32, 3> roi;

  roi.min.x = m_volume_dimensions.min.x +
    (m_volume_dimensions.max.x - m_volume_dimensions.min.x) * m_roi_gui.min.x;
  roi.max.x = m_volume_dimensions.min.x +
    (m_volume_dimensions.max.x - m_volume_dimensions.min.x) * m_roi_gui.max.x;
  roi.min.y = m_volume_dimensions.min.y +
    (m_volume_dimensions.max.y - m_volume_dimensions.min.y) * m_roi_gui.min.y;
  roi.max.y = m_volume_dimensions.min.y +
    (m_volume_dimensions.max.y - m_volume_dimensions.min.y) * m_roi_gui.max.y;
  roi.min.z = m_volume_dimensions.min.z +
    (m_volume_dimensions.max.z - m_volume_dimensions.min.z) * m_roi_gui.min.z;
  roi.max.z = m_volume_dimensions.min.z +
    (m_volume_dimensions.max.z - m_volume_dimensions.min.z) * m_roi_gui.max.z;

  m_app_config_settings->set_region_of_interest(roi);
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_minI(double val)
{
  m_roi_gui.min.x = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_maxI(double val)
{
  m_roi_gui.max.x = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_minJ(double val)
{
  m_roi_gui.min.y = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_maxJ(double val)
{
  m_roi_gui.max.y = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_minK(double val)
{
  m_roi_gui.min.z = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_maxK(double val)
{
  m_roi_gui.max.z = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_log_performance(bool is_log)
{
  m_app_config_settings->set_log_performance(is_log);
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_dump_internal_state(bool is_dump)
{
  m_app_config_settings->set_dump_internal_state(is_dump);
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::update_current_kernel()
{
  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
      static_cast<vtknvindex_irregular_volume_mapper*>(this->DefaultMapper)
        ->rtc_kernel_changed(RTC_KERNELS_ISOSURFACE, KERNEL_IRREGULAR_ISOSURFACE_STRING,
          reinterpret_cast<void*>(&m_isosurface_params), sizeof(m_isosurface_params));
      break;

    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      static_cast<vtknvindex_irregular_volume_mapper*>(this->DefaultMapper)
        ->rtc_kernel_changed(RTC_KERNELS_DEPTH_ENHANCEMENT,
          KERNEL_IRREGULAR_DEPTH_ENHANCEMENT_STRING,
          reinterpret_cast<void*>(&m_depth_enhancement_params), sizeof(m_depth_enhancement_params));
      break;

    case RTC_KERNELS_EDGE_ENHANCEMENT:
      static_cast<vtknvindex_irregular_volume_mapper*>(this->DefaultMapper)
        ->rtc_kernel_changed(RTC_KERNELS_EDGE_ENHANCEMENT, KERNEL_IRREGULAR_EDGE_ENHANCEMENT_STRING,
          reinterpret_cast<void*>(&m_edge_enhancement_params), sizeof(m_edge_enhancement_params));
      break;

    case RTC_KERNELS_CUSTOM:
      static_cast<vtknvindex_irregular_volume_mapper*>(this->DefaultMapper)
        ->rtc_kernel_changed(RTC_KERNELS_CUSTOM, m_custom_kernel_program,
          reinterpret_cast<void*>(&m_custom_params), sizeof(m_custom_params));
      break;

    case RTC_KERNELS_NONE:
    default:
      static_cast<vtknvindex_irregular_volume_mapper*>(this->DefaultMapper)
        ->rtc_kernel_changed(RTC_KERNELS_NONE, "", 0, 0);
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_volume_filter(int filter)
{
  vtknvindex_rtc_kernels kernel = static_cast<vtknvindex_rtc_kernels>(filter);

  m_app_config_settings->set_rtc_kernel(kernel);
  static_cast<vtknvindex_irregular_volume_mapper*>(this->DefaultMapper)->config_settings_changed();

  update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_step_size_factor(double step_size_factor)
{
  m_app_config_settings->set_step_size(static_cast<mi::Float32>(step_size_factor));
  static_cast<vtknvindex_irregular_volume_mapper*>(this->DefaultMapper)->opacity_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_light_type(int light_type)
{
  m_isosurface_params.light_mode = light_type;
  m_depth_enhancement_params.light_mode = light_type;

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;
    case RTC_KERNELS_EDGE_ENHANCEMENT:
    case RTC_KERNELS_CUSTOM:
    case RTC_KERNELS_NONE:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_light_angle(double light_angle)
{
  mi::Float32 angle = static_cast<float>(2.0 * vtkMath::Pi() * (light_angle / 360.0));
  m_isosurface_params.angle = angle;
  m_depth_enhancement_params.angle = angle;

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;
    case RTC_KERNELS_EDGE_ENHANCEMENT:
    case RTC_KERNELS_CUSTOM:
    case RTC_KERNELS_NONE:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_light_elevation(double light_elevation)
{
  mi::Float32 elevation =
    static_cast<float>(2.0 * vtkMath::Pi() * ((light_elevation + 90.0) / 360.0));

  m_isosurface_params.elevation = elevation;
  m_depth_enhancement_params.elevation = elevation;

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;
    case RTC_KERNELS_EDGE_ENHANCEMENT:
    case RTC_KERNELS_CUSTOM:
    case RTC_KERNELS_NONE:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_surf_ambient(double ambient)
{
  m_isosurface_params.amb_fac = static_cast<float>(ambient);
  m_depth_enhancement_params.amb_fac = static_cast<float>(ambient);

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;
    case RTC_KERNELS_EDGE_ENHANCEMENT:
    case RTC_KERNELS_CUSTOM:
    case RTC_KERNELS_NONE:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_surf_specular(double specular)
{
  m_isosurface_params.spec_fac = static_cast<float>(specular);
  m_depth_enhancement_params.spec_fac = static_cast<float>(specular);

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;
    case RTC_KERNELS_EDGE_ENHANCEMENT:
    case RTC_KERNELS_CUSTOM:
    case RTC_KERNELS_NONE:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_surf_specular_power(double specular_power)
{
  m_isosurface_params.shininess = static_cast<float>(specular_power);
  m_depth_enhancement_params.shininess = static_cast<float>(specular_power);

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;
    case RTC_KERNELS_EDGE_ENHANCEMENT:
    case RTC_KERNELS_CUSTOM:
    case RTC_KERNELS_NONE:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_iso_min(double iso_min)
{
  m_isosurface_params.iso_min = static_cast<float>(iso_min / 100.0);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_ISOSURFACE)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_iso_max(double iso_max)
{
  m_isosurface_params.iso_max = static_cast<float>(iso_max / 100.0);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_ISOSURFACE)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_iso_fill_mode(int mode)
{
  m_isosurface_params.fill_up = mode;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_ISOSURFACE)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_iso_use_shading(bool use_shading)
{
  m_isosurface_params.use_shading = use_shading ? 1 : 0;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_ISOSURFACE)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_depth_samples(int depth_samples)
{
  m_depth_enhancement_params.max_dsteps = depth_samples;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_DEPTH_ENHANCEMENT)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_depth_gamma(double gamma)
{
  m_depth_enhancement_params.screen_gamma = gamma;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_DEPTH_ENHANCEMENT)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_edge_range(double edge_range)
{
  m_edge_enhancement_params.sample_range = static_cast<float>(edge_range);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_EDGE_ENHANCEMENT)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_edge_samples(int edge_samples)
{
  m_edge_enhancement_params.stp_num = edge_samples;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_EDGE_ENHANCEMENT)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_kernel_filename(const char* kernel_filename)
{
  m_custom_kernel_filename = std::string(kernel_filename);
  set_kernel_update();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_kernel_update()
{
  std::ifstream is(m_custom_kernel_filename);
  m_custom_kernel_program =
    std::string((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_custom_pfloat_1(double custom_cf1)
{
  m_custom_params.floats[0] = static_cast<float>(custom_cf1);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_custom_pfloat_2(double custom_cf2)
{
  m_custom_params.floats[1] = static_cast<float>(custom_cf2);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_custom_pfloat_3(double custom_cf3)
{
  m_custom_params.floats[2] = static_cast<float>(custom_cf3);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_custom_pfloat_4(double custom_cf4)
{
  m_custom_params.floats[3] = static_cast<float>(custom_cf4);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_custom_pint_1(int custom_ci1)
{
  m_custom_params.ints[0] = static_cast<float>(custom_ci1);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_custom_pint_2(int custom_ci2)
{
  m_custom_params.ints[1] = static_cast<float>(custom_ci2);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_custom_pint_3(int custom_ci3)
{
  m_custom_params.ints[2] = static_cast<float>(custom_ci3);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_custom_pint_4(int custom_ci4)
{
  m_custom_params.ints[3] = static_cast<float>(custom_ci4);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_CUSTOM)
    update_current_kernel();
}
