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

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#include <string>

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLODProp3D.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPExtentTranslator.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPolyDataMapper.h"
#include "vtkProcessModule.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVolumeProperty.h"

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_config_settings.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_instance.h"
#include "vtknvindex_representation.h"
#include "vtknvindex_utilities.h"
#include "vtknvindex_volumemapper.h"

vtknvindex_representation_initializer::vtknvindex_representation_initializer()
{
  static unsigned int counter = 0;
  if (counter == 0)
  {
    vtkProcessModule::SetDefaultMinimumGhostLevelsToRequestForStructuredPipelines(2);
    ++counter;
  }
}

vtknvindex_representation_initializer::~vtknvindex_representation_initializer()
{
}

namespace
{
//----------------------------------------------------------------------------
void vtkGetNonGhostExtent(int* resultExtent, vtkImageData* dataSet)
{
  // Only meant for topologically structured grids.
  dataSet->GetExtent(resultExtent);

  if (vtkUnsignedCharArray* ghostArray = vtkUnsignedCharArray::SafeDownCast(
        dataSet->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName())))
  {
    // We have a ghost array. We need to iterate over the array to prune ghost
    // extents.

    int pntExtent[6];
    std::copy(resultExtent, resultExtent + 6, pntExtent);

    int validCellExtent[6];
    vtkStructuredData::GetCellExtentFromPointExtent(pntExtent, validCellExtent);

    // The start extent is the location of the first cell with ghost value 0.
    for (vtkIdType cc = 0, numTuples = ghostArray->GetNumberOfTuples(); cc < numTuples; ++cc)
    {
      if (ghostArray->GetValue(cc) == 0)
      {
        int ijk[3];
        vtkStructuredData::ComputeCellStructuredCoordsForExtent(cc, pntExtent, ijk);
        validCellExtent[0] = ijk[0];
        validCellExtent[2] = ijk[1];
        validCellExtent[4] = ijk[2];
        break;
      }
    }

    // The end extent is the  location of the last cell with ghost value 0.
    for (vtkIdType cc = (ghostArray->GetNumberOfTuples() - 1); cc >= 0; --cc)
    {
      if (ghostArray->GetValue(cc) == 0)
      {
        int ijk[3];
        vtkStructuredData::ComputeCellStructuredCoordsForExtent(cc, pntExtent, ijk);
        validCellExtent[1] = ijk[0];
        validCellExtent[3] = ijk[1];
        validCellExtent[5] = ijk[2];
        break;
      }
    }

    // Convert cell-extents to pt extents.
    resultExtent[0] = validCellExtent[0];
    resultExtent[2] = validCellExtent[2];
    resultExtent[4] = validCellExtent[4];

    resultExtent[1] = std::min(validCellExtent[1] + 1, resultExtent[1]);
    resultExtent[3] = std::min(validCellExtent[3] + 1, resultExtent[3]);
    resultExtent[5] = std::min(validCellExtent[5] + 1, resultExtent[5]);
  }
}
} // namespace

vtkStandardNewMacro(vtknvindex_representation);

//----------------------------------------------------------------------------
vtknvindex_cached_bounds::vtknvindex_cached_bounds()
{
  for (mi::Uint32 i = 0; i < 6; i++)
    data_bounds[i] = 0.0;

  for (mi::Uint32 i = 0; i < 6; i++)
    whole_extent[i] = 0;

  for (mi::Uint32 i = 0; i < 3; i++)
    origin[i] = spacing[i] = 0.0;
}

//----------------------------------------------------------------------------
vtknvindex_cached_bounds::vtknvindex_cached_bounds(const vtknvindex_cached_bounds& cached_bound)
{
  for (mi::Uint32 i = 0; i < 6; i++)
    data_bounds[i] = cached_bound.data_bounds[i];

  for (mi::Uint32 i = 0; i < 6; i++)
    whole_extent[i] = cached_bound.whole_extent[i];

  for (mi::Uint32 i = 0; i < 3; i++)
    origin[i] = cached_bound.origin[i];

  for (mi::Uint32 i = 0; i < 3; i++)
    spacing[i] = cached_bound.spacing[i];
}

vtknvindex_cached_bounds::vtknvindex_cached_bounds(const double _data_bounds[6],
  const int _whole_extent[6], const double _origin[3], const double _spacing[3])
{
  for (mi::Uint32 i = 0; i < 6; i++)
    data_bounds[i] = _data_bounds[i];

  for (mi::Uint32 i = 0; i < 6; i++)
    whole_extent[i] = _whole_extent[i];

  for (mi::Uint32 i = 0; i < 3; i++)
    origin[i] = _origin[i];

  for (mi::Uint32 i = 0; i < 3; i++)
    spacing[i] = _spacing[i];
}

// This should not be needed anymore.
//// The class vtknvindex_cache_keeper is an derived class of the original vtkPVCacheKeeper
//// which it's used for datasets with time series to avoid data to be loaded again
//// when some time steps were already cached by NVIDIA IndeX.
//
////----------------------------------------------------------------------------
// class vtknvindex_cache_keeper : public vtkPVCacheKeeper
//{
// public:
//  static vtknvindex_cache_keeper* New();
//  vtkTypeMacro(vtknvindex_cache_keeper, vtkPVCacheKeeper);
//
// protected:
//  vtknvindex_cache_keeper()
//  {
//    // This avoids the code that keeps track of memory used by the cache since
//    // this is not applicable in our case.
//    this->SetCacheSizeKeeper(NULL);
//  }
//  ~vtknvindex_cache_keeper() {}
//  // Overridden to avoid caching the data object. We don't cache in
//  // ParaView because NVIDIA IndeX will cache the data internally.
//  bool SaveData(vtkDataObject* dobj) override
//  {
//    vtkDataObject* dNew = dobj->NewInstance();
//    this->Superclass::SaveData(dNew);
//    dNew->Delete();
//    return true;
//  }
//  void RemoveAllCaches() override
//  {
//    // We never clear cache in our demo.
//  }
//};
//
// vtkStandardNewMacro(vtknvindex_cache_keeper);

//----------------------------------------------------------------------------
class vtknvindex_lod_volume : public vtkPVLODVolume
{
public:
  static vtknvindex_lod_volume* New();
  vtkTypeMacro(vtknvindex_lod_volume, vtkPVLODVolume);

  vtknvindex_lod_volume() { m_caching_pass = false; }

  void set_caching_pass(bool caching_pass) { m_caching_pass = caching_pass; }

  int RenderVolumetricGeometry(vtkViewport* vp) override
  {
    if (!m_caching_pass && !this->CanRender())
    {
      return 1;
    }
    int retval = this->LODProp->RenderVolumetricGeometry(vp);

    this->EstimatedRenderTime = this->LODProp->GetEstimatedRenderTime();

    return retval;
  }

protected:
  bool m_caching_pass;
};

vtkStandardNewMacro(vtknvindex_lod_volume);

//----------------------------------------------------------------------------
vtknvindex_representation::vtknvindex_representation()
{
  m_controller = vtkMultiProcessController::GetGlobalController();

  // Init IndeX and ARC
  vtknvindex_instance::get()->init_index();

  // Replace default volume mapper with vtknvindex_volumemapper.
  this->VolumeMapper->Delete();
  this->VolumeMapper = vtknvindex_volumemapper::New();

  // Replace default Actor.
  this->Actor->Delete();
  this->Actor = vtknvindex_lod_volume::New();
  this->Actor->SetProperty(this->Property);
  this->Actor->SetLODMapper(this->OutlineMapper);

  m_cluster_properties = new vtknvindex_cluster_properties();
  m_app_config_settings = m_cluster_properties->get_config_settings();

  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)
    ->set_cluster_properties(m_cluster_properties);

  // TODO: These values should be communicated by ParaView.
  //       Currently there is no way to do this.
  m_roi_range_I[0] = -100.0;
  m_roi_range_I[1] = 100.0;
  m_roi_range_J[0] = -100.0;
  m_roi_range_J[1] = 100.0;
  m_roi_range_K[0] = -100.0;
  m_roi_range_K[1] = 100.0;

  m_still_image_reduction_factor = 1;
  m_interactive_image_reduction_factor = 2;

  m_has_time_steps = 0;
}

//----------------------------------------------------------------------------
vtknvindex_representation::~vtknvindex_representation()
{
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->shutdown();

  delete m_cluster_properties;
}

//----------------------------------------------------------------------------
void vtknvindex_representation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtknvindex_representation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // Check if a dataset has time steps.
    // Restore cached bounds if available.

    vtkPVRenderView::SetForceDataDistributionMode(inInfo, vtkMPIMoveData::COLLECT);

    if (m_has_time_steps)
    {
      vtknvindex_cached_bounds* cached_bounds = get_cached_bounds(m_cur_time);
      if (cached_bounds)
      {
        for (mi::Uint32 i = 0; i < 6; i++)
          this->DataBounds[i] = cached_bounds->data_bounds[i];

        for (mi::Uint32 i = 0; i < 6; i++)
          this->WholeExtent[i] = cached_bounds->whole_extent[i];

        for (mi::Uint32 i = 0; i < 3; i++)
          this->Origin[i] = cached_bounds->origin[i];

        for (mi::Uint32 i = 0; i < 3; i++)
          this->Spacing[i] = cached_bounds->spacing[i];
      }
    }

    vtkPVRenderView::SetPiece(
      inInfo, this, this->OutlineSource->GetOutputDataObject(0), this->DataSize);

    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds);

    // Pass partitioning information to the render view.
    vtkPVRenderView::SetOrderedCompositingInformation(inInfo, this,
      this->PExtentTranslator.GetPointer(), this->WholeExtent, this->Origin, this->Spacing);

    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
  {
    vtkPVRenderView::SetRequiresDistributedRenderingLOD(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    this->UpdateMapperParameters();

    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    if (producerPort)
    {
      this->OutlineMapper->SetInputConnection(producerPort);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtknvindex_representation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    m_still_image_reduction_factor = rview->GetStillRenderImageReductionFactor();
    m_interactive_image_reduction_factor = rview->GetInteractiveRenderImageReductionFactor();

    rview->SetStillRenderImageReductionFactor(1);
    rview->SetInteractiveRenderImageReductionFactor(1);
    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtknvindex_representation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->SetStillRenderImageReductionFactor(m_still_image_reduction_factor);
    rview->SetInteractiveRenderImageReductionFactor(m_interactive_image_reduction_factor);
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
int vtknvindex_representation::RequestDataBase(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
    this->Cache->ShallowCopy(input);

    this->Actor->SetEnableLOD(0);
    this->VolumeMapper->SetInputData(this->Cache);

    vtkImageData* output = vtkImageData::SafeDownCast(this->Cache);

    // Check if a dataset has time steps and skip initialization if bounds data are already cached.
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    int has_time_steps = inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (has_time_steps)
    {
      mi::Sint32 cur_time_step = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      vtknvindex_cached_bounds* cached_bounds = get_cached_bounds(cur_time_step);
      if (cached_bounds)
      {
        return vtkImageVolumeRepresentation::Superclass::RequestData(
          request, inputVector, outputVector);
      }
    }

    this->OutlineSource->SetBounds(output->GetBounds());
    this->OutlineSource->GetBounds(this->DataBounds);
    this->OutlineSource->Update();

    this->DataSize = output->GetActualMemorySize();

    // Collect information about volume that is needed for data redistribution
    // later.

    // Since the KdTree generator is not expecting ghost cells, we need to pass
    // it extents without ghost cells.
    if (output->HasAnyGhostCells())
    {
      vtkNew<vtkImageData> dummy;
      int ext[6];
      vtkGetNonGhostExtent(ext, output);
      dummy->SetExtent(ext);
      dummy->SetOrigin(output->GetOrigin());
      dummy->SetSpacing(output->GetSpacing());
      this->PExtentTranslator->GatherExtents(dummy.Get());
    }
    else
    {
      this->PExtentTranslator->GatherExtents(output);
    }

    output->GetOrigin(this->Origin);
    output->GetSpacing(this->Spacing);
    vtkStreamingDemandDrivenPipeline::GetWholeExtent(
      inputVector[0]->GetInformationObject(0), this->WholeExtent);
  }
  else
  {
    // If no input is available, it then implies that this processes is on a node
    // without the data input, i.e., either client or render-server. In such a case
    // we show the outline only.
    this->VolumeMapper->RemoveAllInputs();
    this->Actor->SetEnableLOD(1);
  }

  return vtkImageVolumeRepresentation::Superclass::RequestData(request, inputVector, outputVector);
}

//-------------------------------------------------------------------------------------------------
int vtknvindex_representation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Call base class first
  if (!this->RequestDataBase(request, inputVector, outputVector))
  {
    return 0;
  }

  // Return if there are no objects to process
  if (inputVector[0]->GetNumberOfInformationObjects() < 1)
  {
    return 1;
  }

  // Check if dataset has time steps.

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  mi::Sint32 nb_time_steps = 0;
  mi::Sint32 cur_time_step = 0;

  // Only use IndeX time series support if Cache Geometry for Animations is enabled.
  int has_time_steps = vtkPVGeneralSettings::GetInstance()->GetCacheGeometryForAnimation() &&
    inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (has_time_steps)
  {
    nb_time_steps = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    mi::Float64* time_steps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    mi::Float64 cur_time = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    cur_time_step = find_time_step(cur_time, time_steps, nb_time_steps);

    mi::Float64 time_range[2] = { 0.0, 0.0 };
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), time_range);
    }

    vtknvindex_regular_volume_properties* volume_properties =
      m_cluster_properties->get_regular_volume_properties();
    volume_properties->set_is_timeseries_data(true);
    volume_properties->set_nb_time_steps(nb_time_steps);
    volume_properties->set_current_time_step(cur_time_step);
  }

  m_has_time_steps = has_time_steps;
  m_cur_time = cur_time_step;

  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  (void)input;
  assert(input != NULL);

  vtknvindex_cached_bounds* cached_bounds =
    has_time_steps ? get_cached_bounds(cur_time_step) : NULL;

  bool using_cache = (cached_bounds != NULL); // this->CacheKeeper->GetCachingEnabled();

  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->is_caching(using_cache);
  static_cast<vtknvindex_lod_volume*>(this->Actor)->set_caching_pass(using_cache);

  vtkDataSet* ds = vtkDataSet::SafeDownCast(this->Cache);
  if (ds)
  {
    mi::Sint32 extent[6] = { 0, 0, 0, 0, 0, 0 };
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
    }

    mi::Sint32 num_processes = m_controller->GetNumberOfProcesses();
    mi::Sint32* all_rank_extents = new mi::Sint32[6 * num_processes];

    // Calculate global extends by gathering extents of all the pieces.
    m_controller->AllGather(extent, all_rank_extents, 6);

    // volume extents
    mi::math::Bbox<mi::Sint32, 3> volume_extents;
    volume_extents.clear();

    // volume dimensions
    for (mi::Sint32 i = 0, idx = 0; i < num_processes; i++, idx += 6)
    {
      mi::Sint32* cur_extent = all_rank_extents + idx;
      mi::math::Bbox<mi::Sint32, 3> subset_extent;
      subset_extent.min.x = cur_extent[0];
      subset_extent.min.y = cur_extent[2];
      subset_extent.min.z = cur_extent[4];
      subset_extent.max.x = cur_extent[1];
      subset_extent.max.y = cur_extent[3];
      subset_extent.max.z = cur_extent[5];

      volume_extents.insert(subset_extent);
    }

    // volume size
    m_volume_size.x = static_cast<mi::Uint32>(volume_extents.max.x - volume_extents.min.x + 1);
    m_volume_size.y = static_cast<mi::Uint32>(volume_extents.max.y - volume_extents.min.y + 1);
    m_volume_size.z = static_cast<mi::Uint32>(volume_extents.max.z - volume_extents.min.z + 1);

    if (!using_cache)
    {
      ds->GetBounds(this->DataBounds);

      // Calculate global extends by gathering extents of all the pieces.
      // mi::Sint32 num_processes = m_controller->GetNumberOfProcesses();
      mi::Float64* all_rank_bounds = new mi::Float64[6 * num_processes];

      m_controller->AllGather(this->DataBounds, all_rank_bounds, 6);

      mi::math::Bbox<mi::Float64, 3> volume_bounds;
      volume_bounds.clear();

      // volume dimensions
      for (mi::Sint32 i = 0, idx = 0; i < num_processes; i++, idx += 6)
      {
        mi::Float64* cur_extent = all_rank_bounds + idx;
        mi::math::Bbox<mi::Float64, 3> subset_bounds;
        subset_bounds.min.x = cur_extent[0];
        subset_bounds.min.y = cur_extent[2];
        subset_bounds.min.z = cur_extent[4];
        subset_bounds.max.x = cur_extent[1];
        subset_bounds.max.y = cur_extent[3];
        subset_bounds.max.z = cur_extent[5];

        volume_bounds.insert(subset_bounds);
      }

      static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->set_whole_bounds(volume_bounds);

      mi::math::Vector<mi::Float64, 3> scaling(
        (volume_extents.max.x - volume_extents.min.x) / (volume_bounds.max.x - volume_bounds.min.x),
        (volume_extents.max.y - volume_extents.min.y) / (volume_bounds.max.y - volume_bounds.min.y),
        (volume_extents.max.z - volume_extents.min.z) /
          (volume_bounds.max.z - volume_bounds.min.z));

      mi::math::Vector<mi::Float32, 3> translation_flt(
        static_cast<mi::Float32>(volume_bounds.min.x * scaling.x),
        static_cast<mi::Float32>(volume_bounds.min.y * scaling.y),
        static_cast<mi::Float32>(volume_bounds.min.z * scaling.z));

      mi::math::Vector<mi::Float32, 3> scaling_flt(static_cast<mi::Float32>(1. / scaling.x),
        static_cast<mi::Float32>(1. / scaling.y), static_cast<mi::Float32>(1. / scaling.z));

      m_cluster_properties->get_regular_volume_properties()->set_volume_extents(volume_extents);
      m_cluster_properties->get_regular_volume_properties()->set_volume_translation(
        translation_flt);
      m_cluster_properties->get_regular_volume_properties()->set_volume_scaling(scaling_flt);

      // volume size
      m_cluster_properties->get_regular_volume_properties()->set_volume_size(m_volume_size);

      delete[] all_rank_bounds;

      // Cache bounds only for time varying datasets.
      if (has_time_steps)
        set_cached_bounds(cur_time_step);

      update_index_roi();
    }
  }
  else
  {
    ERROR_LOG << "Failed to retrieve the entire extent in vtknvindex_representation::RequestData,";
  }

  m_controller->Barrier();

  return 1;
}

//-------------------------------------------------------------------------------------------------
int vtknvindex_representation::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  if (inputVector[0]->GetNumberOfInformationObjects() < 1)
  {
    return 1;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int ghost_levels = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // ParaView's ghost cells must be equal to NVIDIA IndeX's border size.
  ghost_levels += m_app_config_settings->get_subcube_border();

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghost_levels);

  return 1;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_representation::SetVisibility(bool val)
{
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->set_visibility(val);
  update_index_roi();

  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
void vtknvindex_representation::SetScalarOpacityUnitDistance(double val)
{
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->opacity_changed();
  this->Property->SetScalarOpacityUnitDistance(val);
}

//-------------------------------------------------------------------------------------------------
vtknvindex_cached_bounds* vtknvindex_representation::get_cached_bounds(mi::Sint32 time)
{
  std::map<mi::Sint32, vtknvindex_cached_bounds>::iterator it = m_time_to_cached_bounds.find(time);

  if (it == m_time_to_cached_bounds.end())
    return NULL;

  return &(it->second);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_representation::set_cached_bounds(mi::Sint32 time)
{
  vtknvindex_cached_bounds cached_bounds(
    this->DataBounds, this->WholeExtent, this->Origin, this->Spacing);

  m_time_to_cached_bounds.insert(
    std::pair<mi::Sint32, vtknvindex_cached_bounds>(time, cached_bounds));
}

//-------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_representation::find_time_step(
  mi::Float64 time, const mi::Float64* time_steps, mi::Sint32 nb_tsteps) const
{
  std::vector<mi::Float64> times(time_steps, time_steps + nb_tsteps);
  auto lower = std::lower_bound(times.begin(), times.end(), time);

  if (lower == times.end())
    lower = times.end() - 1;

  return static_cast<mi::Uint32>(*lower);
}

//
// Configuration options set by ParaView GUI.
//-------------------------------------------------------------------------------------------------
void vtknvindex_representation::set_subcube_size(unsigned x, unsigned y, unsigned z)
{
  mi::math::Vector_struct<mi::Uint32, 3> subcube_size = { x, y, z };
  m_app_config_settings->set_subcube_size(subcube_size);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_subcube_border(int border)
{
  m_app_config_settings->set_subcube_border(border);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_filter_mode(int filter_mode)
{
  m_app_config_settings->set_filter_mode(filter_mode);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_preintegration(bool enable_preint)
{
  m_app_config_settings->set_preintegration(enable_preint);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();
}

void vtknvindex_representation::set_volume_step_size(double step_size)
{
  m_app_config_settings->set_step_size(static_cast<mi::Float32>(step_size));
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_opacity_reference_mode(int opacity_reference_mode)
{
  m_app_config_settings->set_opacity_mode(opacity_reference_mode);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->opacity_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_opacity_reference(double opacity_reference)
{
  m_app_config_settings->set_opacity_reference(opacity_reference);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->opacity_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::update_index_roi()
{
  // Set region of interest.
  mi::math::Bbox<mi::Float32, 3> roi;

  roi.min.x = (m_volume_size.x - 1) * m_roi_gui.min.x;
  roi.max.x = (m_volume_size.x - 1) * m_roi_gui.max.x;
  roi.min.y = (m_volume_size.y - 1) * m_roi_gui.min.y;
  roi.max.y = (m_volume_size.y - 1) * m_roi_gui.max.y;
  roi.min.z = (m_volume_size.z - 1) * m_roi_gui.min.z;
  roi.max.z = (m_volume_size.z - 1) * m_roi_gui.max.z;

  m_app_config_settings->set_region_of_interest(roi);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_roi_minI(double val)
{
  m_roi_gui.min.x = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_roi_maxI(double val)
{
  m_roi_gui.max.x = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_roi_minJ(double val)
{
  m_roi_gui.min.y = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_roi_maxJ(double val)
{
  m_roi_gui.max.y = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_roi_minK(double val)
{
  m_roi_gui.min.z = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_roi_maxK(double val)
{
  m_roi_gui.max.z = (val + 100.0) / 200.0;
  update_index_roi();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_log_performance(bool is_log)
{
  m_app_config_settings->set_log_performance(is_log);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_dump_internal_state(bool is_dump)
{
  m_app_config_settings->set_dump_internal_state(is_dump);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::show_volume(bool enable)
{
  m_app_config_settings->enable_volume(enable);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_selected_slice(int /*selected_slice*/)
{
}

//----------------------------------------------------------------------------
void vtknvindex_representation::enable_slice1(bool enable)
{
  m_app_config_settings->enable_slice(0, enable);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_slice_mode1(int slice_mode)
{
  m_app_config_settings->set_slice_mode(0, slice_mode);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_slice_pos1(double pos)
{
  m_app_config_settings->set_slice_displace(0, pos / 100.0);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::enable_slice2(bool enable)
{
  m_app_config_settings->enable_slice(1, enable);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_slice_mode2(int slice_mode)
{
  m_app_config_settings->set_slice_mode(1, slice_mode);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_slice_pos2(double pos)
{
  m_app_config_settings->set_slice_displace(1, pos / 100.0);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::enable_slice3(bool enable)
{
  m_app_config_settings->enable_slice(2, enable);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_slice_mode3(int slice_mode)
{
  m_app_config_settings->set_slice_mode(2, slice_mode);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_slice_pos3(double pos)
{
  m_app_config_settings->set_slice_displace(2, pos / 100.0);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->slices_changed();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::update_current_kernel()
{
  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
      static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)
        ->rtc_kernel_changed(RTC_KERNELS_ISOSURFACE, reinterpret_cast<void*>(&m_isosurface_params),
          sizeof(m_isosurface_params));
      break;

    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)
        ->rtc_kernel_changed(RTC_KERNELS_DEPTH_ENHANCEMENT,
          reinterpret_cast<void*>(&m_depth_enhancement_params), sizeof(m_depth_enhancement_params));
      break;

    case RTC_KERNELS_EDGE_ENHANCEMENT:
      static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)
        ->rtc_kernel_changed(RTC_KERNELS_EDGE_ENHANCEMENT,
          reinterpret_cast<void*>(&m_edge_enhancement_params), sizeof(m_edge_enhancement_params));
      break;

    case RTC_KERNELS_GRADIENT:
      static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)
        ->rtc_kernel_changed(RTC_KERNELS_GRADIENT, reinterpret_cast<void*>(&m_gradient_params),
          sizeof(m_gradient_params));
      break;

    case RTC_KERNELS_NONE:
    default:
      static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)
        ->rtc_kernel_changed(RTC_KERNELS_NONE, 0, 0);
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_volume_filter(int filter)
{
  vtknvindex_rtc_kernels kernel = static_cast<vtknvindex_rtc_kernels>(filter);

  m_app_config_settings->set_rtc_kernel(kernel);
  static_cast<vtknvindex_volumemapper*>(this->VolumeMapper)->config_settings_changed();

  update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_light_type(int light_type)
{
  m_isosurface_params.light_mode = light_type;
  m_depth_enhancement_params.light_mode = light_type;

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;

    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_light_angle(double light_angle)
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

    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_light_elevation(double light_elevation)
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

    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_surf_ambient(double ambient)
{
  m_isosurface_params.amb_fac = static_cast<float>(ambient);
  m_depth_enhancement_params.amb_fac = static_cast<float>(ambient);

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;

    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_surf_specular(double specular)
{
  m_isosurface_params.spec_fac = static_cast<float>(specular);
  m_depth_enhancement_params.spec_fac = static_cast<float>(specular);

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;

    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_surf_specular_power(double specular_power)
{
  m_isosurface_params.shininess = static_cast<float>(specular_power);
  m_depth_enhancement_params.shininess = static_cast<float>(specular_power);

  switch (m_app_config_settings->get_rtc_kernel())
  {
    case RTC_KERNELS_ISOSURFACE:
    case RTC_KERNELS_DEPTH_ENHANCEMENT:
      update_current_kernel();
      break;

    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_iso_min(double iso_min)
{
  m_isosurface_params.iso_min = static_cast<float>(iso_min / 100.0);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_ISOSURFACE)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_iso_max(double iso_max)
{
  m_isosurface_params.iso_max = static_cast<float>(iso_max / 100.0);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_ISOSURFACE)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_iso_fill_mode(int mode)
{
  m_isosurface_params.fill_up = mode;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_ISOSURFACE)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_iso_use_shading(bool use_shading)
{
  m_isosurface_params.use_shading = use_shading ? 1 : 0;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_ISOSURFACE)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_depth_samples(int depth_samples)
{
  m_depth_enhancement_params.max_dsteps = depth_samples;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_DEPTH_ENHANCEMENT)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_depth_gamma(double gamma)
{
  m_depth_enhancement_params.screen_gamma = gamma;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_DEPTH_ENHANCEMENT)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_edge_range(double edge_range)
{
  m_edge_enhancement_params.sample_range = static_cast<float>(edge_range);

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_EDGE_ENHANCEMENT)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_edge_samples(int edge_samples)
{
  m_edge_enhancement_params.stp_num = edge_samples;

  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_EDGE_ENHANCEMENT)
    update_current_kernel();
}

//----------------------------------------------------------------------------
void vtknvindex_representation::set_gradient_level(double gradient_level)
{
  m_gradient_params.gradient = static_cast<mi::Float32>(gradient_level / 100.0);
  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_GRADIENT)
    update_current_kernel();
}
//----------------------------------------------------------------------------
void vtknvindex_representation::set_gradient_scale(double gradient_scale)
{
  m_gradient_params.grad_max = static_cast<mi::Float32>(gradient_scale);
  if (m_app_config_settings->get_rtc_kernel() == RTC_KERNELS_GRADIENT)
    update_current_kernel();
}
