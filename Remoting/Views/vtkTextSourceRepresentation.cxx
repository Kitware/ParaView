/*=========================================================================

  Program:   ParaView
  Module:    vtkTextSourceRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextSourceRepresentation.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAbstractArray.h"
#include "vtkAbstractWidget.h"
#include "vtkAlgorithmOutput.h"
#include "vtkBillboardTextActor3D.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFlagpoleLabel.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkTable.h"
#include "vtkTextRepresentation.h"
#include "vtkVariant.h"

namespace
{
std::string vtkExtractString(vtkDataObject* data)
{
  std::string text;
  vtkFieldData* fieldData = data->GetFieldData();
  vtkAbstractArray* array = fieldData->GetAbstractArray(0);
  if (array && array->GetNumberOfTuples() > 0)
  {
    text = array->GetVariantValue(0).ToString();
  }
  return text;
}
}

vtkStandardNewMacro(vtkTextSourceRepresentation);
vtkCxxSetObjectMacro(
  vtkTextSourceRepresentation, TextWidgetRepresentation, vtk3DWidgetRepresentation);
vtkCxxSetObjectMacro(vtkTextSourceRepresentation, FlagpoleLabel, vtkFlagpoleLabel);
vtkCxxSetObjectMacro(vtkTextSourceRepresentation, BillboardTextActor, vtkBillboardTextActor3D);

//----------------------------------------------------------------------------
vtkTextSourceRepresentation::vtkTextSourceRepresentation()
{
  this->TextPropMode = 0;
  this->TextWidgetRepresentation = nullptr;
  this->FlagpoleLabel = nullptr;
  this->BillboardTextActor = nullptr;

  vtkPointSource* source = vtkPointSource::New();
  source->SetNumberOfPoints(1);
  source->Update();
  this->DummyPolyData = vtkPolyData::New();
  this->DummyPolyData->ShallowCopy(source->GetOutputDataObject(0));
  source->Delete();
}

//----------------------------------------------------------------------------
vtkTextSourceRepresentation::~vtkTextSourceRepresentation()
{
  this->SetFlagpoleLabel(nullptr);
  this->SetTextWidgetRepresentation(nullptr);
  this->SetBillboardTextActor(nullptr);
  this->DummyPolyData->Delete();
}

//----------------------------------------------------------------------------
void vtkTextSourceRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  if (this->TextWidgetRepresentation && this->TextWidgetRepresentation->GetRepresentation())
  {
    this->TextWidgetRepresentation->GetRepresentation()->SetVisibility(val);
    this->TextWidgetRepresentation->SetEnabled(val);
  }

  if (this->FlagpoleLabel && this->TextPropMode == 1)
  {
    this->FlagpoleLabel->SetVisibility(val);
  }

  if (this->BillboardTextActor && this->TextPropMode == 2)
  {
    this->BillboardTextActor->SetVisibility(val);
  }
}

//----------------------------------------------------------------------------
void vtkTextSourceRepresentation::SetInteractivity(bool val)
{
  if (this->TextWidgetRepresentation && this->TextWidgetRepresentation->GetWidget())
  {
    this->TextWidgetRepresentation->GetWidget()->SetProcessEvents(val);
  }
}

//----------------------------------------------------------------------------
int vtkTextSourceRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkTextSourceRepresentation::AddToView(vtkView* view)
{
  if (this->TextWidgetRepresentation)
  {
    view->AddRepresentation(this->TextWidgetRepresentation);
  }
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
  {
    vtkRenderer* activeRenderer = pvview->GetRenderer();
    if (this->FlagpoleLabel)
    {
      activeRenderer->AddActor(this->FlagpoleLabel);
    }
    if (this->BillboardTextActor)
    {
      activeRenderer->AddActor(this->BillboardTextActor);
    }
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkTextSourceRepresentation::RemoveFromView(vtkView* view)
{
  if (this->TextWidgetRepresentation)
  {
    view->RemoveRepresentation(this->TextWidgetRepresentation);
  }
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
  {
    vtkRenderer* activeRenderer = pvview->GetRenderer();
    if (this->FlagpoleLabel)
    {
      activeRenderer->RemoveActor(this->FlagpoleLabel);
    }
    if (this->BillboardTextActor)
    {
      activeRenderer->RemoveActor(this->BillboardTextActor);
    }
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
int vtkTextSourceRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkTable* input = vtkTable::GetData(inputVector[0], 0);
    if (input->GetNumberOfRows() > 0 && input->GetNumberOfColumns() > 0)
    {
      this->DummyPolyData->GetFieldData()->ShallowCopy(input->GetRowData());
    }
  }
  else
  {
    this->DummyPolyData->Initialize();
  }
  this->DummyPolyData->Modified();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkTextSourceRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->DummyPolyData);
    vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(inInfo, this,
      /*deliver_to_client=*/true, /*gather_before_delivery=*/false);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);

    // since there's no direct connection between the mapper and the collector,
    // we don't put an update-suppressor in the pipeline.

    std::string text =
      vtkExtractString(producerPort->GetProducer()->GetOutputDataObject(producerPort->GetIndex()));
    vtkTextRepresentation* repr = this->TextWidgetRepresentation
      ? vtkTextRepresentation::SafeDownCast(this->TextWidgetRepresentation->GetRepresentation())
      : nullptr;
    if (repr)
    {
      repr->SetText(text.c_str());
      repr->SetVisibility(text.empty() ? 0 : this->TextPropMode == 0);
    }
    if (this->BillboardTextActor)
    {
      this->BillboardTextActor->SetInput(text.c_str());
      this->BillboardTextActor->SetVisibility(text.empty() ? 0 : this->TextPropMode == 2);
    }
    if (this->FlagpoleLabel)
    {
      this->FlagpoleLabel->SetInput(text.c_str());
      this->FlagpoleLabel->SetVisibility(text.empty() ? 0 : this->TextPropMode == 1);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkTextSourceRepresentation::SetTextPropMode(int val)
{
  if (this->TextPropMode == val)
  {
    return;
  }

  this->TextPropMode = val;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTextSourceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
