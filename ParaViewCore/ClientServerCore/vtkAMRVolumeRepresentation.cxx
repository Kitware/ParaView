/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRVolumeRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRVolumeRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkAMRIncrementalResampleHelper.h"
#include "vtkAMRResampleFilter.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkTimerLog.h"
#include "vtkUniformGrid.h"
#include "vtkVolumeProperty.h"

#include <map>
#include <string>


vtkStandardNewMacro(vtkAMRVolumeRepresentation);
//----------------------------------------------------------------------------
vtkAMRVolumeRepresentation::vtkAMRVolumeRepresentation()
{
  this->RequestedRenderMode = 0; //vtkAMRVolumeMapper::DefaultRenderMode; // Use Smart Mode
  this->RequestedResamplingMode = 0; // Frustrum Mode
  this->VolumeMapper = vtkSmartVolumeMapper::New();
  //this->VolumeMapper->SetUseDefaultThreading(true);
  this->Property = vtkVolumeProperty::New();

  this->Actor = vtkPVLODVolume::New();
  this->Actor->SetProperty(this->Property);

  this->Resampler = vtkAMRIncrementalResampleHelper::New();

  this->CacheKeeper = vtkPVCacheKeeper::New();

  this->ColorArrayName = 0;
  this->ColorAttributeType = POINT_DATA;
  this->Cache = vtkOverlappingAMR::New();

  this->CacheKeeper->SetInputData(this->Cache);
  this->FreezeFocalPoint = false;
  vtkMath::UninitializeBounds(this->DataBounds);

  this->StreamingBlockId = 0;
  this->StreamingCapableSource = false;

  this->InitializeResampler = true;
}

//----------------------------------------------------------------------------
vtkAMRVolumeRepresentation::~vtkAMRVolumeRepresentation()
{
  this->VolumeMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
  this->CacheKeeper->Delete();

  this->SetColorArrayName(0);

  this->Cache->Delete();
  this->Resampler->Delete();
}

//----------------------------------------------------------------------------
int vtkAMRVolumeRepresentation::FillInputPortInformation(
  int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRVolumeRepresentation::RequestUpdateExtent(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestUpdateExtent(
      request, inputVector, outputVector))
    {
    return 0;
    }

  // If this->StreamingCapableSource is true, i.e. input is streaming capable
  // and streaming is enabled, the update request is always "qualified". i.e. we
  // only request a particular block from the input. The block is passed on by
  // the view during streaming updates. Default behavior is to just request the
  // 0th block.
  if (this->StreamingCapableSource)
    {
    for (int cc=0; cc < this->GetNumberOfInputPorts(); cc++)
      {
      for (int kk=0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
        {
        vtkInformation* info = inputVector[cc]->GetInformationObject(kk);
        info->Set(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1);
        int block = static_cast<int>(this->StreamingBlockId);
        info->Set(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(),
          &block, 1);
        cout << "vtkAMRVolumeRepresentation::RequestUpdateExtent " 
          << block << endl;
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRVolumeRepresentation::RequestInformation(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
    {
    return 0;
    }

  // Determine if the input is streaming capable.
  this->StreamingCapableSource = false;
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()) &&
      vtkPVView::GetEnableStreaming())
      {
      this->StreamingCapableSource = true;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRVolumeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type,
  vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
    {
    return 0;
    }

  if (request_type == vtkPVView::REQUEST_UPDATE())
    {
    // FIXME:STREAMING :- how do we tell the view to use "cuts" from this
    // representation for ordered compositing?
    // At the same time, the image data is not the data being delivered
    // anywhere, so we don't really report it to the view's storage.
    // vtkPVRenderView::SetPiece(inInfo, this, this->Cache);

    vtkPVRenderView::SetPiece(inInfo, this,
      this->CacheKeeper->GetOutputDataObject(0));
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
    vtkPVRenderView::SetGeometryBounds(inInfo, this->DataBounds);
    vtkPVRenderView::SetStreamable(inInfo, this, this->StreamingCapableSource);
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
    this->UpdateMapperParameters();
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    if (producerPort)
      {
      vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(
        producerPort->GetProducer()->GetOutputDataObject(0));
      if (amr)
        {
        //cout << "AMR Being Rendering -----" << endl;
        //for (unsigned int cc=0; cc < amr->GetNumberOfLevels(); cc++)
        //  {
        //  for (unsigned int kk=0; kk < amr->GetNumberOfDataSets(cc); kk++)
        //    {
        //    cout << cc <<", " << kk << " = " << amr->GetDataSet(cc, kk) << endl;
        //    }
        //  }

        double bounds[6];
        amr->GetBounds(bounds);
#ifdef USE_RESAMPLER
        vtkNew<vtkAMRResampleFilter> resampler;
        resampler->SetNumberOfSamples(40, 40, 40);
        resampler->SetDemandDrivenMode(0);
        resampler->SetMin(bounds[0], bounds[2], bounds[4]);
        resampler->SetMax(bounds[1], bounds[3], bounds[5]);
        resampler->SetInputData(amr);
        resampler->Update();
        vtkMultiBlockDataSet* mbs = vtkMultiBlockDataSet::SafeDownCast(
          resampler->GetOutputDataObject(0));
        if (mbs && mbs->GetNumberOfBlocks() == 1)
          {
          this->VolumeMapper->SetInputData(
            vtkImageData::SafeDownCast(mbs->GetBlock(0)));
          }
#else
        if (this->InitializeResampler)
          {
          cout << "init resampler" << endl;
          this->Resampler->Initialize(amr);
          this->Resampler->SetAMRData(amr);
          this->InitializeResampler = false;
          }

        //double position[3], focalpoint[3];
        //this->RenderView->GetActiveCamera()->GetPosition(position);
        //this->RenderView->GetActiveCamera()->GetFocalPoint(focalpoint);

        //double radius = sqrt(vtkMath::Distance2BetweenPoints(
        //  position, focalpoint));

        //vtkBoundingBox bbox(focalpoint[0], focalpoint[0],
        //  focalpoint[1], focalpoint[1],
        //  focalpoint[2], focalpoint[2]);
        //bbox.AddPoint(focalpoint[0] - radius, focalpoint[1] - radius, focalpoint[2] - radius);
        //bbox.AddPoint(focalpoint[0] + radius, focalpoint[1] + radius, focalpoint[2] + radius);
        //bbox.GetBounds(bounds);

        vtkTimerLog::MarkStartEvent("Resample Volume");
        int samples[3]= {40, 40, 40};

        this->Resampler->UpdateROI(bounds, samples);
        this->Resampler->Update();
        vtkTimerLog::MarkEndEvent("Resample Volume");

        this->VolumeMapper->SetInputData(this->Resampler->GetGrid());
#endif
        }
      else
        {
        this->VolumeMapper->SetInputConnection(producerPort);
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRVolumeRepresentation::RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  vtkMath::UninitializeBounds(this->DataBounds);
  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    vtkOverlappingAMR* input =
      vtkOverlappingAMR::GetData(inputVector[0], 0);
    if (!this->GetUsingCacheForUpdate())
      {
      this->Cache->ShallowCopy(input);
      }
    this->CacheKeeper->Update();
    vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(
      this->CacheKeeper->GetOutputDataObject(0));
    if (amr)
      {
      amr->GetBounds(this->DataBounds);
      }
    }

  if (this->StreamingBlockId == 0 || !this->StreamingCapableSource)
    {
    this->InitializeResampler = true;
    }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkAMRVolumeRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::MarkModified()
{
  if (!this->GetUseCache())
    {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
    }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
bool vtkAMRVolumeRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    this->RenderView = view;
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkAMRVolumeRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->RemoveActor(this->Actor);
    this->RenderView = NULL;
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::UpdateMapperParameters()
{
  this->VolumeMapper->SelectScalarArray(this->ColorArrayName);
  //this->VolumeMapper->SetRequestedRenderMode(this->RequestedRenderMode);
  // FIXME: setting to RayCastRenderMode while we are debugging things
  this->VolumeMapper->SetRequestedRenderMode(vtkSmartVolumeMapper::RayCastRenderMode);

  // we always say point-data, since the resampler samples to points.
  this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
  //this->VolumeMapper->SetNumberOfSamples(this->NumberOfSamples);
  //this->VolumeMapper->SetRequestedResamplingMode(this->RequestedResamplingMode);
  //this->VolumeMapper->SetFreezeFocalPoint(this->FreezeFocalPoint);
  this->Actor->SetMapper(this->VolumeMapper);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


//***************************************************************************
// Forwarded to Actor.

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}
//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetPosition(double x , double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}
//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->Actor->SetVisibility(val? 1 : 0);
}

//***************************************************************************
// Forwarded to vtkVolumeProperty.
//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetInterpolationType(int val)
{
  this->Property->SetInterpolationType(val);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetColor(vtkColorTransferFunction* lut)
{
  this->Property->SetColor(lut);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetScalarOpacity(vtkPiecewiseFunction* pwf)
{
  this->Property->SetScalarOpacity(pwf);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetScalarOpacityUnitDistance(double val)
{
  this->Property->SetScalarOpacityUnitDistance(val);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetAmbient(double val)
{
  this->Property->SetAmbient(val);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetDiffuse(double val)
{
  this->Property->SetDiffuse(val);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetSpecular(double val)
{
  this->Property->SetSpecular(val);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::SetShade(bool val)
{
  this->Property->SetShade(val);
}
//----------------------------------------------------------------------------
