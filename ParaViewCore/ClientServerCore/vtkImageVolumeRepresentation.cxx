/*=========================================================================

  Program:   ParaView
  Module:    vtkImageVolumeRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageVolumeRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkImageData.h"
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

#include <vtkstd/map>
#include <vtkstd/string>

class vtkImageVolumeRepresentation::vtkInternals
{
public:
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkVolumeMapper> >
    MapOfMappers;
  MapOfMappers Mappers;
  vtkstd::string ActiveVolumeMapper;
};

vtkStandardNewMacro(vtkImageVolumeRepresentation);
//----------------------------------------------------------------------------
vtkImageVolumeRepresentation::vtkImageVolumeRepresentation()
{
  this->Internals = new vtkInternals();
  this->DefaultMapper = vtkFixedPointVolumeRayCastMapper::New();
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
  this->Cache = vtkImageData::New();

  this->CacheKeeper->SetInput(this->Cache);
  this->OutlineDeliveryFilter->SetInputConnection(
    this->OutlineSource->GetOutputPort());
  this->OutlineUpdateSuppressor->SetInputConnection(
    this->OutlineDeliveryFilter->GetOutputPort());
  this->OutlineMapper->SetInputConnection(
    this->OutlineUpdateSuppressor->GetOutputPort());
  this->Actor->SetLODMapper(this->OutlineMapper);
}

//----------------------------------------------------------------------------
vtkImageVolumeRepresentation::~vtkImageVolumeRepresentation()
{
  delete this->Internals;
  this->DefaultMapper->Delete();
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
void vtkImageVolumeRepresentation::AddVolumeMapper(
  const char* name, vtkVolumeMapper* mapper)
{
  this->Internals->Mappers[name] = mapper;
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetActiveVolumeMapper(const char* mapper)
{
  this->Internals->ActiveVolumeMapper = mapper? mapper : "";
  this->MarkModified();
}

//----------------------------------------------------------------------------
vtkVolumeMapper* vtkImageVolumeRepresentation::GetActiveVolumeMapper()
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
int vtkImageVolumeRepresentation::FillInputPortInformation(
  int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageVolumeRepresentation::ProcessViewRequest(
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
int vtkImageVolumeRepresentation::RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // mark delivery filters modified.
  this->OutlineDeliveryFilter->Modified();

  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
    if (!this->GetUsingCacheForUpdate())
      {
      this->Cache->ShallowCopy(input);
      }
    this->CacheKeeper->Update();

    this->Actor->SetEnableLOD(0);
    this->GetActiveVolumeMapper()->SetInputConnection(
      this->CacheKeeper->GetOutputPort());

    this->OutlineSource->SetBounds(vtkImageData::SafeDownCast(
        this->CacheKeeper->GetOutputDataObject(0))->GetBounds());
    }
  else
    {
    // when no input is present, it implies that this processes is on a node
    // without the data input i.e. either client or render-server, in which case
    // we show only the outline.
    this->GetActiveVolumeMapper()->RemoveAllInputs();
    this->Actor->SetEnableLOD(1);
    }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkImageVolumeRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::MarkModified()
{
  if (!this->GetUseCache())
    {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
    }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
bool vtkImageVolumeRepresentation::AddToView(vtkView* view)
{
  // FIXME: Need generic view API to add props.
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkImageVolumeRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::UpdateMapperParameters()
{
  vtkVolumeMapper* activeMapper = this->GetActiveVolumeMapper();
  activeMapper->SelectScalarArray(this->ColorArrayName);
  switch (this->ColorAttributeType)
    {
  case CELL_DATA:
    activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case POINT_DATA:
  default:
    activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    break;
    }
  this->Actor->SetMapper(activeMapper);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


//***************************************************************************
// Forwarded to Actor.

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}
//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetPosition(double x , double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}
//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->Actor->SetVisibility(val? 1 : 0);
}

//***************************************************************************
// Forwarded to vtkVolumeProperty.
//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetInterpolationType(int val)
{
  this->Property->SetInterpolationType(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetColor(vtkColorTransferFunction* lut)
{
  this->Property->SetColor(lut);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetScalarOpacity(vtkPiecewiseFunction* pwf)
{
  this->Property->SetScalarOpacity(pwf);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetScalarOpacityUnitDistance(double val)
{
  this->Property->SetScalarOpacityUnitDistance(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetAmbient(double val)
{
  this->Property->SetAmbient(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetDiffuse(double val)
{
  this->Property->SetDiffuse(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetSpecular(double val)
{
  this->Property->SetSpecular(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetShade(bool val)
{
  this->Property->SetShade(val);
}
