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
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPExtentTranslator.h"
#include "vtkPolyDataMapper.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVolumeProperty.h"

#include <map>
#include <string>


vtkStandardNewMacro(vtkImageVolumeRepresentation);
//----------------------------------------------------------------------------
vtkImageVolumeRepresentation::vtkImageVolumeRepresentation()
{
  this->VolumeMapper = vtkSmartVolumeMapper::New();
  this->Property = vtkVolumeProperty::New();

  this->Actor = vtkPVLODVolume::New();
  this->Actor->SetProperty(this->Property);

  this->CacheKeeper = vtkPVCacheKeeper::New();

  this->OutlineSource = vtkOutlineSource::New();
  this->OutlineMapper = vtkPolyDataMapper::New();

  this->Cache = vtkImageData::New();

  this->CacheKeeper->SetInputData(this->Cache);
  this->Actor->SetLODMapper(this->OutlineMapper);

  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;
}

//----------------------------------------------------------------------------
vtkImageVolumeRepresentation::~vtkImageVolumeRepresentation()
{
  this->VolumeMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
  this->OutlineSource->Delete();
  this->OutlineMapper->Delete();
  this->CacheKeeper->Delete();

  this->Cache->Delete();
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
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
    {
    return 0;
    }
  if (request_type == vtkPVView::REQUEST_UPDATE())
    {
    vtkPVRenderView::SetPiece(inInfo, this,
      this->OutlineSource->GetOutputDataObject(0),
      this->DataSize);
    // BUG #14792.
    // We report this->DataSize explicitly since the data being "delivered" is
    // not the data that should be used to make rendering decisions based on
    // data size.
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetGeometryBounds(inInfo, this->DataBounds);

    // Pass partitioning information to the render view.
    vtkPVRenderView::SetOrderedCompositingInformation(
      inInfo, this, this->PExtentTranslator.GetPointer(),
      this->WholeExtent, this->Origin, this->Spacing);

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
#ifndef VTK_LEGACY_REMOVE
void vtkImageVolumeRepresentation::PassOrderedCompositingInformation(
  vtkPVDataRepresentation* vtkNotUsed(self), vtkInformation* vtkNotUsed(inInfo))
{
  vtkGenericWarningMacro(
    "vtkImageVolumeRepresentation::PassOrderedCompositingInformation was deprecated in "
    "ParaView 5.0 and will be removed in a future version. Change your representation "
    "to cache information about image and then pass to "
    "vtkPVRenderView::SetOrderedCompositingInformation() directly.");
}
#endif

//----------------------------------------------------------------------------
int vtkImageVolumeRepresentation::RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

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
    this->VolumeMapper->SetInputConnection(
      this->CacheKeeper->GetOutputPort());

    vtkImageData* output = vtkImageData::SafeDownCast(this->CacheKeeper->GetOutputDataObject(0));
    this->OutlineSource->SetBounds(output->GetBounds());
    this->OutlineSource->GetBounds(this->DataBounds);
    this->OutlineSource->Update();

    this->DataSize = output->GetActualMemorySize();

    // Collect information about volume that is needed for data redistribution
    // later.
    this->PExtentTranslator->GatherExtents(output);
    this->PExtentTranslator->Print(cout);
    output->GetOrigin(this->Origin);
    output->GetSpacing(this->Spacing);
    vtkStreamingDemandDrivenPipeline::GetWholeExtent(
      inputVector[0]->GetInformationObject(0), this->WholeExtent);
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
    return this->Superclass::AddToView(view);
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
    return this->Superclass::RemoveFromView(view);
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::UpdateMapperParameters()
{
  const char* colorArrayName = NULL;
  int fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  vtkInformation *info = this->GetInputArrayInformation(0);
  if (info &&
    info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
    {
    colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
    }

  this->VolumeMapper->SelectScalarArray(colorArrayName);
  switch (fieldAssociation)
    {
  case vtkDataObject::FIELD_ASSOCIATION_CELLS:
    this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case vtkDataObject::FIELD_ASSOCIATION_NONE:
    this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
    break;

  case vtkDataObject::FIELD_ASSOCIATION_POINTS:
  default:
    this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    break;
    }

  this->Actor->SetMapper(this->VolumeMapper);
  // this is necessary since volume mappers don't like empty arrays.
  this->Actor->SetVisibility(colorArrayName != NULL && colorArrayName[0] != 0);
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

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetIndependantComponents(bool val)
{
  this->Property->SetIndependentComponents(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetRequestedRenderMode(int mode)
{
  this->VolumeMapper->SetRequestedRenderMode(mode);
}
