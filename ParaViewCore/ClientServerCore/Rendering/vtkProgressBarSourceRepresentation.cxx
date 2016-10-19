/*=========================================================================

  Program:   ParaView
  Module:    vtkProgressBarSourceRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgressBarSourceRepresentation.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAbstractArray.h"
#include "vtkAbstractWidget.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVRenderView.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkProgressBarRepresentation.h"
#include "vtkTable.h"
#include "vtkVariant.h"

namespace
{
double vtkExtractDouble(vtkDataObject* data)
{
  double progressRate = 0;
  vtkFieldData* fieldData = data->GetFieldData();
  vtkAbstractArray* array = fieldData->GetAbstractArray(0);
  if (array && array->GetNumberOfTuples() > 0)
  {
    progressRate = array->GetVariantValue(0).ToDouble();
  }
  return progressRate;
}
}

vtkStandardNewMacro(vtkProgressBarSourceRepresentation);
vtkCxxSetObjectMacro(
  vtkProgressBarSourceRepresentation, ProgressBarWidgetRepresentation, vtk3DWidgetRepresentation);
//----------------------------------------------------------------------------
vtkProgressBarSourceRepresentation::vtkProgressBarSourceRepresentation()
{
  this->ProgressBarWidgetRepresentation = 0;

  this->CacheKeeper = vtkPVCacheKeeper::New();

  vtkPointSource* source = vtkPointSource::New();
  source->SetNumberOfPoints(1);
  source->Update();
  this->DummyPolyData = vtkPolyData::SafeDownCast(source->GetOutputDataObject(0));
  source->Delete();

  this->CacheKeeper->SetInputData(this->DummyPolyData);
}

//----------------------------------------------------------------------------
vtkProgressBarSourceRepresentation::~vtkProgressBarSourceRepresentation()
{
  this->SetProgressBarWidgetRepresentation(0);
  this->CacheKeeper->Delete();
}

//----------------------------------------------------------------------------
void vtkProgressBarSourceRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  if (this->ProgressBarWidgetRepresentation &&
    this->ProgressBarWidgetRepresentation->GetRepresentation())
  {
    this->ProgressBarWidgetRepresentation->GetRepresentation()->SetVisibility(val);
    this->ProgressBarWidgetRepresentation->SetEnabled(val);
  }
}

//----------------------------------------------------------------------------
void vtkProgressBarSourceRepresentation::SetInteractivity(bool val)
{
  if (this->ProgressBarWidgetRepresentation && this->ProgressBarWidgetRepresentation->GetWidget())
  {
    this->ProgressBarWidgetRepresentation->GetWidget()->SetProcessEvents(val);
  }
}

//----------------------------------------------------------------------------
int vtkProgressBarSourceRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkProgressBarSourceRepresentation::AddToView(vtkView* view)
{
  if (this->ProgressBarWidgetRepresentation)
  {
    view->AddRepresentation(this->ProgressBarWidgetRepresentation);
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkProgressBarSourceRepresentation::RemoveFromView(vtkView* view)
{
  if (this->ProgressBarWidgetRepresentation)
  {
    view->RemoveRepresentation(this->ProgressBarWidgetRepresentation);
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkProgressBarSourceRepresentation::MarkModified()
{
  if (!this->GetUseCache())
  {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
  }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
bool vtkProgressBarSourceRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
int vtkProgressBarSourceRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkTable* input = vtkTable::GetData(inputVector[0], 0);
    if (input->GetNumberOfRows() > 0 && input->GetNumberOfColumns() > 0)
    {
      this->DummyPolyData->GetFieldData()->ShallowCopy(input->GetRowData());
    }
  }
  this->DummyPolyData->Modified();
  this->CacheKeeper->Update();

  // It is tempting to try to do the data delivery in RequestData() itself.
  // However, whenever a representation updates, ParaView GUI may have some
  // GatherInformation() requests that happen. That messes up with any
  // data-delivery code placed here. So we leave the data delivery to the
  // REQUEST_PREPARE_FOR_RENDER() pass.
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkProgressBarSourceRepresentation::ProcessViewRequest(
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

    double progressRate =
      vtkExtractDouble(producerPort->GetProducer()->GetOutputDataObject(producerPort->GetIndex()));
    vtkProgressBarRepresentation* repr =
      vtkProgressBarRepresentation::SafeDownCast(this->ProgressBarWidgetRepresentation
          ? this->ProgressBarWidgetRepresentation->GetRepresentation()
          : NULL);
    if (repr)
    {
      repr->SetProgressRate(progressRate);
      repr->SetVisibility(true);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkProgressBarSourceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
