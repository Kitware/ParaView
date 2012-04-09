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
#include "vtkCommand.h"
#include "vtkAMRVolumeMapper.h"
#include "vtkOverlappingAMR.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredDataDeliveryFilter.h"
#include "vtkVolumeProperty.h"

#include <map>
#include <string>


vtkStandardNewMacro(vtkAMRVolumeRepresentation);
//----------------------------------------------------------------------------
vtkAMRVolumeRepresentation::vtkAMRVolumeRepresentation()
{
  this->RequestedRenderMode = 0; // Use Smart Mode
  this->RequestedResamplingMode = 0; // Frustrum Mode
  this->RenderView = NULL;
  this->VolumeMapper = vtkAMRVolumeMapper::New();
  this->VolumeMapper->SetUseDefaultThreading(true);
  this->Property = vtkVolumeProperty::New();

  this->Actor = vtkPVLODVolume::New();
  this->Actor->SetProperty(this->Property);

  this->CacheKeeper = vtkPVCacheKeeper::New();

  this->OutlineSource = vtkOutlineSource::New();
  this->OutlineDeliveryFilter = vtkUnstructuredDataDeliveryFilter::New();
  this->OutlineUpdateSuppressor = vtkPVUpdateSuppressor::New();
  this->OutlineMapper = vtkPolyDataMapper::New();

  this->ColorArrayName = 0;
  this->ColorAttributeType = POINT_DATA;
  this->Cache = vtkOverlappingAMR::New();

  this->CacheKeeper->SetInputData(this->Cache);
  this->OutlineDeliveryFilter->SetInputConnection(
    this->OutlineSource->GetOutputPort());
  this->OutlineUpdateSuppressor->SetInputConnection(
    this->OutlineDeliveryFilter->GetOutputPort());
  this->OutlineMapper->SetInputConnection(
    this->OutlineUpdateSuppressor->GetOutputPort());
  this->Actor->SetLODMapper(this->OutlineMapper);
  this->FreezeFocalPoint = false;
}

//----------------------------------------------------------------------------
vtkAMRVolumeRepresentation::~vtkAMRVolumeRepresentation()
{
  this->VolumeMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
  this->OutlineSource->Delete();
  this->OutlineDeliveryFilter->Delete();
  this->OutlineUpdateSuppressor->Delete();
  this->OutlineMapper->Delete();
  this->CacheKeeper->Delete();

  this->SetColorArrayName(0);

  this->Cache->Delete();
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
int
vtkAMRVolumeRepresentation::RequestUpdateExtent(vtkInformation* request,
                                                vtkInformationVector** inputVector,
                                                vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
  this->VolumeMapper->ProcessUpdateExtentRequest(this->RenderView->GetRenderer(),
                                                  request, inputVector,
                                                  outputVector);
  return 1;
}
//----------------------------------------------------------------------------
int
vtkAMRVolumeRepresentation::RequestInformation(vtkInformation* request,
                                               vtkInformationVector** inputVector,
                                               vtkInformationVector* outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);
  this->VolumeMapper->ProcessInformationRequest(this->RenderView->GetRenderer(),
                                                request, inputVector,
                                                outputVector);
  return 1;
}
//----------------------------------------------------------------------------
int vtkAMRVolumeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type,
  vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (request_type == vtkPVView::REQUEST_INFORMATION())
    {
    outInfo->Set(vtkPVRenderView::GEOMETRY_SIZE(),
      this->Cache->GetActualMemorySize());
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
    if (this->GetNumberOfInputConnections(0) == 1)
      {
      outInfo->Set(vtkPVRenderView::REDISTRIBUTABLE_DATA_PRODUCER(),
        this->GetInputConnection(0, 0)->GetProducer());
      }
    }
  else if (request_type == vtkPVView::REQUEST_PREPARE_FOR_RENDER())
    {
    // // In REQUEST_PREPARE_FOR_RENDER, we need to ensure all our data-deliver
    // // filters have their states updated as requested by the view.

    // // this is where we will look to see on what nodes are we going to render and
    // // render set that up.
    this->OutlineDeliveryFilter->ProcessViewRequest(inInfo);
    if (this->OutlineDeliveryFilter->GetMTime() >
      this->OutlineUpdateSuppressor->GetForcedUpdateTimeStamp())
      {
      outInfo->Set(vtkPVRenderView::NEEDS_DELIVERY(), 1);
      }
    }
  else if (request_type == vtkPVView::REQUEST_DELIVERY())
    {
    this->OutlineDeliveryFilter->Modified();
    this->OutlineUpdateSuppressor->ForceUpdate();
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
    this->UpdateMapperParameters();
    }

  return this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo);
}

//----------------------------------------------------------------------------
int vtkAMRVolumeRepresentation::RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
    cerr << "vtkAMRVolumeRepresentation::RequestData" << endl;
  // mark delivery filters modified.
  this->OutlineDeliveryFilter->Modified();

  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    vtkOverlappingAMR* input =
      vtkOverlappingAMR::GetData(inputVector[0], 0);
    if (!this->GetUsingCacheForUpdate())
      {
      this->Cache->ShallowCopy(input);
      }
    this->CacheKeeper->Update();

    this->Actor->SetEnableLOD(0);
    this->VolumeMapper->SetInputConnection(
      this->CacheKeeper->GetOutputPort());

    this->OutlineSource->SetBounds(vtkOverlappingAMR::SafeDownCast(
        this->CacheKeeper->GetOutputDataObject(0))->GetBounds());
    }
  else
    {
    // when no input is present, it implies that this processes is on a node
    // without the data input i.e. either client or render-server, in which case
    // we show only the outline.
    this->VolumeMapper->RemoveAllInputs();
    this->Actor->SetEnableLOD(1);
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
  // FIXME: Need generic view API to add props.
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    if (this->RenderView)
      {
      this->RenderView->UnRegister(0);
      }
    this->RenderView = rview;
    this->RenderView->Register(0);
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
    if (this->RenderView)
      {
      this->RenderView->UnRegister(0);
      this->RenderView = NULL;
      }
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkAMRVolumeRepresentation::UpdateMapperParameters()
{
  this->VolumeMapper->SelectScalarArray(this->ColorArrayName);
  this->VolumeMapper->SetRequestedRenderMode(this->RequestedRenderMode);
  this->VolumeMapper->SetNumberOfSamples(this->NumberOfSamples);
  this->VolumeMapper->SetRequestedResamplingMode(this->RequestedResamplingMode);
  this->VolumeMapper->SetFreezeFocalPoint(this->FreezeFocalPoint);
  switch (this->ColorAttributeType)
    {
  case CELL_DATA:
    this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case POINT_DATA:
  default:
    this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    break;
    }
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
