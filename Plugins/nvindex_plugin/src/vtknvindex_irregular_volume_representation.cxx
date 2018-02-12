/* Copyright 2018 NVIDIA Corporation. All rights reserved.
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
#include "vtkPVCacheKeeper.h"
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVUpdateSuppressor.h"
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
{
  m_controller = vtkMultiProcessController::GetGlobalController();

  this->ResampleToImageFilter = vtkResampleToImage::New();
  this->ResampleToImageFilter->SetSamplingDimensions(128, 128, 128);

  this->Internals = new vtkInternals();

  this->Preprocessor = vtkVolumeRepresentationPreprocessor::New();
  this->Preprocessor->SetTetrahedraOnly(1);

  this->CacheKeeper = vtkPVCacheKeeper::New();

  // Change the default mapper to NVIDIA IndeX irregular volume mapper.
  this->DefaultMapper = vtknvindex_irregular_volume_mapper::New();
  this->Property = vtkVolumeProperty::New();
  this->Actor = vtkPVLODVolume::New();

  this->CacheKeeper->SetInputConnection(this->Preprocessor->GetOutputPort());

  this->Actor->SetProperty(this->Property);
  this->Actor->SetMapper(this->DefaultMapper);
  vtkMath::UninitializeBounds(this->DataBounds);

  // Create NVIDIA IndeX cluster properties and application settings.
  m_cluster_properties = new vtknvindex_cluster_properties();
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
}

//----------------------------------------------------------------------------
vtknvindex_irregular_volume_representation::~vtknvindex_irregular_volume_representation()
{
  this->Preprocessor->Delete();
  this->CacheKeeper->Delete();

  this->DefaultMapper->shutdown();
  this->DefaultMapper->Delete();

  this->Property->Delete();
  this->Actor->Delete();

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
void vtknvindex_irregular_volume_representation::MarkModified()
{
  if (!this->GetUseCache())
  {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
  }
  this->Superclass::MarkModified();
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

  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    this->Preprocessor->SetInputConnection(this->GetInternalOutputPort());

    this->Preprocessor->Update();
    this->CacheKeeper->Update();

    // Check for time series data info.
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    mi::Sint32 nb_time_steps = 0;
    mi::Float32 cur_time_step = 0;
    int has_time_steps = inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    if (has_time_steps)
    {
      nb_time_steps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      mi::Float64* time_steps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

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

    vtkDataSet* ds = vtkDataSet::SafeDownCast(this->CacheKeeper->GetOutputDataObject(0));
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

    mi::math::Bbox<mi::Sint32, 3> volume_dimensions;
    volume_dimensions.clear();

    // volume dimensions
    for (mi::Sint32 i = 0, idx = 0; i < num_processes; i++, idx += 6)
    {
      mi::Float64* cur_extent = all_rank_extents + idx;
      mi::math::Bbox<mi::Sint32, 3> cur_volume_dimensions;
      cur_volume_dimensions.min.x = static_cast<mi::Sint32>(std::floor(cur_extent[0]));
      cur_volume_dimensions.min.y = static_cast<mi::Sint32>(std::floor(cur_extent[2]));
      cur_volume_dimensions.min.z = static_cast<mi::Sint32>(std::floor(cur_extent[4]));
      cur_volume_dimensions.max.x = static_cast<mi::Sint32>(std::ceil(cur_extent[1]));
      cur_volume_dimensions.max.y = static_cast<mi::Sint32>(std::ceil(cur_extent[3]));
      cur_volume_dimensions.max.z = static_cast<mi::Sint32>(std::ceil(cur_extent[5]));

      volume_dimensions.insert(cur_volume_dimensions);
    }

    delete[] all_rank_extents;

    m_cluster_properties->get_regular_volume_properties()->set_volume_extents(volume_dimensions);

    // volume size
    mi::math::Vector_struct<mi::Uint32, 3> volume_size;
    volume_size.x = volume_dimensions.max.x - volume_dimensions.min.x + 1;
    volume_size.y = volume_dimensions.max.y - volume_dimensions.min.y + 1;
    volume_size.z = volume_dimensions.max.z - volume_dimensions.min.z + 1;

    m_cluster_properties->get_regular_volume_properties()->set_volume_size(volume_size);

    // region of interest
    mi::math::Bbox_struct<mi::Float32, 3> roi;

    roi.min.x = volume_dimensions.min.x +
      (volume_dimensions.max.x - volume_dimensions.min.x) * m_roi_gui.min.x;
    roi.max.x = volume_dimensions.min.x +
      (volume_dimensions.max.x - volume_dimensions.min.x) * m_roi_gui.max.x;
    roi.min.y = volume_dimensions.min.y +
      (volume_dimensions.max.y - volume_dimensions.min.y) * m_roi_gui.min.y;
    roi.max.y = volume_dimensions.min.y +
      (volume_dimensions.max.y - volume_dimensions.min.y) * m_roi_gui.max.y;
    roi.min.z = volume_dimensions.min.z +
      (volume_dimensions.max.z - volume_dimensions.min.z) * m_roi_gui.min.z;
    roi.max.z = volume_dimensions.min.z +
      (volume_dimensions.max.z - volume_dimensions.min.z) * m_roi_gui.max.z;

    m_app_config_settings->set_region_of_interest(roi);
  }
  else
  {
    this->Preprocessor->RemoveAllInputs();
    vtkNew<vtkUnstructuredGrid> placeholder;
    this->Preprocessor->SetInputData(0, placeholder.GetPointer());
    this->CacheKeeper->Update();
  }

  m_controller->Barrier();

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtknvindex_irregular_volume_representation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
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

    vtkPVRenderView::SetPiece(inInfo, this, this->CacheKeeper->GetOutputDataObject(0));
    vtkPVRenderView::MarkAsRedistributable(inInfo, this);

    vtkNew<vtkMatrix4x4> matrix;
    this->Actor->GetMatrix(matrix.GetPointer());
    vtkPVRenderView::SetGeometryBounds(inInfo, this->DataBounds, matrix.GetPointer());
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {

    vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(inInfo->Get(vtkPVRenderView::VIEW()));
    vtkPVDataDeliveryManager* ddm = view->GetDeliveryManager();
    vtkPKdTree* kd_tree = ddm->GetKdTree();

    // Retrieve ParaView's KDTRee in order to obtain domain subdivision bounding boxes.
    if (kd_tree)
    {
      DefaultMapper->set_domain_kdtree(kd_tree);
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
void vtknvindex_irregular_volume_representation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}
//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetPosition(double x, double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}
//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val ? 1 : 0);
  this->Superclass::SetVisibility(val);
}

//***************************************************************************
// Forwarded to vtkVolumeProperty.
//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetInterpolationType(int val)
{
  this->Property->SetInterpolationType(val);
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetColor(vtkColorTransferFunction* lut)
{
  this->Property->SetColor(lut);
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetScalarOpacity(vtkPiecewiseFunction* pwf)
{
  this->Property->SetScalarOpacity(pwf);
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::SetScalarOpacityUnitDistance(double val)
{
  this->Property->SetScalarOpacityUnitDistance(val);
}

//
// Configuration options set from Paraview GUI.
//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_subcube_size(int x, int y, int z)
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
void vtknvindex_irregular_volume_representation::set_filter_mode(int filter_mode)
{
  m_app_config_settings->set_filter_mode(filter_mode);
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_preintegration(bool enable_preint)
{
  m_app_config_settings->set_preintegration(enable_preint);
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_minI(double val)
{
  m_roi_gui.min.x = (val + 100.0) / 200.0;
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_maxI(double val)
{
  m_roi_gui.max.x = (val + 100.0) / 200.0;
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_minJ(double val)
{
  m_roi_gui.min.y = (val + 100.0) / 200.0;
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_maxJ(double val)
{
  m_roi_gui.max.y = (val + 100.0) / 200.0;
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_minK(double val)
{
  m_roi_gui.min.z = (val + 100.0) / 200.0;
  DefaultMapper->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_representation::set_roi_maxK(double val)
{
  m_roi_gui.max.z = (val + 100.0) / 200.0;
  DefaultMapper->config_settings_changed();
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
