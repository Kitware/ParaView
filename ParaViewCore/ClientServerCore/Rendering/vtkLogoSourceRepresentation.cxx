/*=========================================================================

  Program:   ParaView
  Module:    vtkLogoSourceRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLogoSourceRepresentation.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAbstractWidget.h"
#include "vtkAlgorithmOutput.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogoRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVRenderView.h"
#include "vtkProperty2D.h"

vtkStandardNewMacro(vtkLogoSourceRepresentation);
vtkCxxSetObjectMacro(
  vtkLogoSourceRepresentation, LogoWidgetRepresentation, vtk3DWidgetRepresentation);

//----------------------------------------------------------------------------
vtkLogoSourceRepresentation::vtkLogoSourceRepresentation()
{
  this->CacheKeeper->SetInputData(this->ImageCache);
}

//----------------------------------------------------------------------------
vtkLogoSourceRepresentation::~vtkLogoSourceRepresentation()
{
  this->SetLogoWidgetRepresentation(nullptr);
}

//----------------------------------------------------------------------------
void vtkLogoSourceRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  if (this->LogoWidgetRepresentation && this->LogoWidgetRepresentation->GetRepresentation())
  {
    this->LogoWidgetRepresentation->GetRepresentation()->SetVisibility(val);
    this->LogoWidgetRepresentation->SetEnabled(val);
  }
}

//----------------------------------------------------------------------------
void vtkLogoSourceRepresentation::SetInteractivity(bool val)
{
  if (this->LogoWidgetRepresentation && this->LogoWidgetRepresentation->GetWidget())
  {
    this->LogoWidgetRepresentation->GetWidget()->SetProcessEvents(val);
  }
}

//----------------------------------------------------------------------------
int vtkLogoSourceRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkLogoSourceRepresentation::AddToView(vtkView* view)
{
  if (this->LogoWidgetRepresentation)
  {
    view->AddRepresentation(this->LogoWidgetRepresentation);
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkLogoSourceRepresentation::RemoveFromView(vtkView* view)
{
  if (this->LogoWidgetRepresentation)
  {
    view->RemoveRepresentation(this->LogoWidgetRepresentation);
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkLogoSourceRepresentation::MarkModified()
{
  if (!this->GetUseCache())
  {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
  }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
bool vtkLogoSourceRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
int vtkLogoSourceRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
    this->ImageCache->ShallowCopy(input);
  }
  this->ImageCache->Modified();
  this->CacheKeeper->Update();

  // It is tempting to try to do the data delivery in RequestData() itself.
  // However, whenever a representation updates, ParaView GUI may have some
  // GatherInformation() requests that happen. That messes up with any
  // data-delivery code placed here. So we leave the data delivery to the
  // REQUEST_PREPARE_FOR_RENDER() pass.
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkLogoSourceRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->CacheKeeper->GetOutputDataObject(0));
    vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(inInfo, this,
      /*deliver_to_client=*/true, /*gather_before_delivery=*/false);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);

    // since there's no direct connection between the mapper and the collector,
    // we don't put an update-suppressor in the pipeline.
    vtkImageData* image = vtkImageData::SafeDownCast(
      producerPort->GetProducer()->GetOutputDataObject(producerPort->GetIndex()));

    // Setup the logo
    vtkLogoRepresentation* repr = vtkLogoRepresentation::SafeDownCast(this->LogoWidgetRepresentation
        ? this->LogoWidgetRepresentation->GetRepresentation()
        : nullptr);
    if (image && image->GetNumberOfPoints() > 0)
    {
      int dims[3];
      image->GetDimensions(dims);
      repr->SetImage(image);
      repr->GetImageProperty()->SetOpacity(this->Opacity);
      repr->SetVisibility(true);
      float height = repr->GetPosition2()[1];
      repr->SetPosition2(height * dims[0] / dims[1], height);
    }
    else
    {
      repr->SetVisibility(false);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkLogoSourceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent nextIndent = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Opacity: " << this->Opacity << endl;
  if (this->LogoWidgetRepresentation)
  {
    os << indent << "LogoWidgetRepresentation: " << endl;
    this->LogoWidgetRepresentation->PrintSelf(os, nextIndent);
  }
  else
  {
    os << indent << "LogoWidgetRepresentation: (None)" << endl;
  }
  os << indent << "ImageCache: " << endl;
  this->ImageCache->PrintSelf(os, nextIndent);
  os << indent << "CacheKeeper: " << endl;
  this->CacheKeeper->PrintSelf(os, nextIndent);
}
