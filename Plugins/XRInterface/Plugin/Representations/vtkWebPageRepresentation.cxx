/*=========================================================================

  Program:   ParaView
  Module:    vtkWebPageRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWebPageRepresentation.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkPlaneSource.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkTable.h"
#include "vtkTextRepresentation.h"
#include "vtkXRInterfaceWebView.h"

#include "vtkQWidgetRepresentation.h"
#include "vtkQWidgetWidget.h"

#include <QHBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineView>

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

vtkStandardNewMacro(vtkWebPageRepresentation);
vtkCxxSetObjectMacro(vtkWebPageRepresentation, TextWidgetRepresentation, vtk3DWidgetRepresentation);

//----------------------------------------------------------------------------
vtkWebPageRepresentation::vtkWebPageRepresentation()
{
  this->TextWidgetRepresentation = vtk3DWidgetRepresentation::New();
  this->QWidgetWidget = vtkQWidgetWidget::New();
  this->QWidgetWidget->CreateDefaultRepresentation();
  auto* rep = static_cast<vtkQWidgetRepresentation*>(this->QWidgetWidget->GetRepresentation());
  this->PlaneSource = rep->GetPlaneSource();
  this->TextWidgetRepresentation->SetWidget(this->QWidgetWidget);
  this->TextWidgetRepresentation->SetRepresentation(
    this->QWidgetWidget->GetQWidgetRepresentation());

  this->WebView = new vtkXRInterfaceWebView();
  this->QWidgetWidget->SetWidget(this->WebView);

  vtkPointSource* source = vtkPointSource::New();
  source->SetNumberOfPoints(1);
  source->Update();
  this->DummyPolyData = vtkPolyData::New();
  this->DummyPolyData->ShallowCopy(source->GetOutputDataObject(0));
  source->Delete();
}

//----------------------------------------------------------------------------
vtkWebPageRepresentation::~vtkWebPageRepresentation()
{
  delete this->WebView;
  this->TextWidgetRepresentation->Delete();
  this->DummyPolyData->Delete();
}

vtkQWidgetWidget* vtkWebPageRepresentation::GetDuplicateWidget()
{
  auto* qWidgetWidget = vtkQWidgetWidget::New();
  qWidgetWidget->SetWidget(this->QWidgetWidget->GetWidget());
  auto* rep = static_cast<vtkQWidgetRepresentation*>(this->QWidgetWidget->GetRepresentation());
  qWidgetWidget->SetRepresentation(rep);

  return qWidgetWidget;
}

void vtkWebPageRepresentation::SetInputText(const char* val)
{
  this->WebView->SetInputText(val);
}

//----------------------------------------------------------------------------
void vtkWebPageRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);

  if (this->TextWidgetRepresentation && this->TextWidgetRepresentation->GetRepresentation())
  {
    this->TextWidgetRepresentation->GetRepresentation()->SetVisibility(val);
    this->TextWidgetRepresentation->SetEnabled(val);
  }
}

//----------------------------------------------------------------------------
void vtkWebPageRepresentation::SetInteractivity(bool val)
{
  if (this->TextWidgetRepresentation && this->TextWidgetRepresentation->GetWidget())
  {
    this->TextWidgetRepresentation->GetWidget()->SetProcessEvents(val);
  }
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::SetOrigin(double x, double y, double z)
{
  this->PlaneSource->SetOrigin(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::SetOrigin(double xyz[3])
{
  this->PlaneSource->SetOrigin(xyz);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkWebPageRepresentation::GetOrigin()
{
  return this->PlaneSource->GetOrigin();
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::GetOrigin(double xyz[3])
{
  this->PlaneSource->GetOrigin(xyz);
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::SetPoint1(double x, double y, double z)
{
  this->PlaneSource->SetPoint1(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::SetPoint1(double xyz[3])
{
  this->PlaneSource->SetPoint1(xyz);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkWebPageRepresentation::GetPoint1()
{
  return this->PlaneSource->GetPoint1();
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::GetPoint1(double xyz[3])
{
  this->PlaneSource->GetPoint1(xyz);
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::SetPoint2(double x, double y, double z)
{
  this->PlaneSource->SetPoint2(x, y, z);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::SetPoint2(double xyz[3])
{
  this->PlaneSource->SetPoint2(xyz);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkWebPageRepresentation::GetPoint2()
{
  return this->PlaneSource->GetPoint2();
}

//------------------------------------------------------------------------------
void vtkWebPageRepresentation::GetPoint2(double xyz[3])
{
  this->PlaneSource->GetPoint2(xyz);
}

//----------------------------------------------------------------------------
int vtkWebPageRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkWebPageRepresentation::AddToView(vtkView* view)
{
  if (this->TextWidgetRepresentation)
  {
    view->AddRepresentation(this->TextWidgetRepresentation);
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkWebPageRepresentation::RemoveFromView(vtkView* view)
{
  if (this->TextWidgetRepresentation)
  {
    view->RemoveRepresentation(this->TextWidgetRepresentation);
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
int vtkWebPageRepresentation::RequestData(
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
int vtkWebPageRepresentation::ProcessViewRequest(
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
    this->WebView->loadURL(text);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkWebPageRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
