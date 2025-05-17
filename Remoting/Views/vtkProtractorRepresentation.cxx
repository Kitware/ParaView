// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProtractorRepresentation.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAbstractArray.h"
#include "vtkAbstractWidget.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAngleRepresentation2D.h"
#include "vtkAxisActor2D.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLeaderActor2D.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkPointSource.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkVariant.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkProtractorRepresentation);

//----------------------------------------------------------------------------
vtkCxxSetSmartPointerMacro(
  vtkProtractorRepresentation, AngleRepresentation, vtkAngleRepresentation2D);

//----------------------------------------------------------------------------
vtkProtractorRepresentation::vtkProtractorRepresentation()
{
  this->AngleRepresentation.TakeReference(vtkAngleRepresentation2D::New());
  this->AngleRepresentation->SetDragable(false);
  this->AngleRepresentation->InstantiateHandleRepresentation();
  this->AngleRepresentation->ArcVisibilityOn();
  this->AngleRepresentation->GetArc()->UseFontSizeFromPropertyOn();
  this->AngleRepresentation->GetArc()->AutoLabelOff();
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  if (this->AngleRepresentation)
  {
    this->AngleRepresentation->SetVisibility(val);
  }
}

//----------------------------------------------------------------------------
int vtkProtractorRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkProtractorRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rView = vtkPVRenderView::SafeDownCast(view);
  if (this->AngleRepresentation && rView)
  {
    auto* renderer = rView->GetRenderer(vtkPVRenderView::NON_COMPOSITED_RENDERER);
    renderer->AddActor(this->AngleRepresentation);
    this->AngleRepresentation->SetRenderer(renderer);
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkProtractorRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rView = vtkPVRenderView::SafeDownCast(view);
  if (this->AngleRepresentation && rView)
  {
    rView->GetRenderer(vtkPVRenderView::NON_COMPOSITED_RENDERER)
      ->RemoveActor(this->AngleRepresentation);
    this->AngleRepresentation->SetRenderer(nullptr);
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::SetTextProperty(vtkTextProperty* prop)
{
  vtkMTimeType previousMTime = this->AngleRepresentation->GetMTime();
  prop->SetVerticalJustificationToCentered();
  this->AngleRepresentation->GetArc()->SetLabelTextProperty(prop);
  if (this->AngleRepresentation->GetArc()->GetMTime() != previousMTime)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::SetLineColor(double r, double g, double b)
{
  auto getMaxTime = [this]()
  {
    return std::max({ this->AngleRepresentation->GetArc()->GetMTime(),
      this->AngleRepresentation->GetRay1()->GetMTime(),
      this->AngleRepresentation->GetRay2()->GetMTime() });
  };

  vtkMTimeType previousMTime = getMaxTime();
  this->AngleRepresentation->GetArc()->GetProperty()->SetColor(r, g, b);
  this->AngleRepresentation->GetRay1()->GetProperty()->SetColor(r, g, b);
  this->AngleRepresentation->GetRay2()->GetProperty()->SetColor(r, g, b);
  if (getMaxTime() != previousMTime)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::SetLineThickness(double thickness)
{
  auto getMaxTime = [this]()
  {
    return std::max({ this->AngleRepresentation->GetArc()->GetMTime(),
      this->AngleRepresentation->GetRay1()->GetMTime(),
      this->AngleRepresentation->GetRay2()->GetMTime() });
  };

  vtkMTimeType previousMTime = getMaxTime();
  this->AngleRepresentation->GetArc()->GetProperty()->SetLineWidth(thickness);
  this->AngleRepresentation->GetRay1()->GetProperty()->SetLineWidth(thickness);
  this->AngleRepresentation->GetRay2()->GetProperty()->SetLineWidth(thickness);
  if (getMaxTime() != previousMTime)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::SetArrowStyle(int style)
{
  auto getMaxTime = [this]()
  {
    return std::max(this->AngleRepresentation->GetRay1()->GetMTime(),
      this->AngleRepresentation->GetRay2()->GetMTime());
  };

  vtkMTimeType previousMTime = getMaxTime();
  this->AngleRepresentation->GetRay1()->SetArrowStyle(style);
  this->AngleRepresentation->GetRay2()->SetArrowStyle(style);
  if (getMaxTime() != previousMTime)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::SetLabelFactorSize(double factor)
{
  vtkMTimeType previousMTime = this->AngleRepresentation->GetMTime();
  this->AngleRepresentation->GetArc()->SetLabelFactor(factor);
  if (this->AngleRepresentation->GetArc()->GetMTime() != previousMTime)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::SetLabelFormat(char* labelFormat)
{
  vtkMTimeType previousMTime = this->AngleRepresentation->GetMTime();
  this->AngleRepresentation->SetLabelFormat(labelFormat);
  if (this->AngleRepresentation->GetMTime() != previousMTime)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::SetAngleScale(double scale)
{
  vtkMTimeType previousMTime = this->AngleRepresentation->GetMTime();
  this->AngleRepresentation->SetScale(scale);
  if (this->AngleRepresentation->GetMTime() != previousMTime)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkProtractorRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    auto input = vtkPolyData::GetData(inputVector[0], 0);
    if (input)
    {
      this->Clone->ShallowCopy(input);
    }
  }

  this->Clone->Modified();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkProtractorRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->Clone);
    // `gather_before_delivery` is true, since vtkPolyLineSource (which is the
    // source for the ruler) doesn't produce any data on ranks except the root.
    vtkPVRenderView::SetDeliverToAllProcesses(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);

    // since there's no direct connection between the mapper and the collector,
    // we don't put an update-suppressor in the pipeline.

    vtkPolyData* line = vtkPolyData::SafeDownCast(
      producerPort->GetProducer()->GetOutputDataObject(producerPort->GetIndex()));
    if (line->GetNumberOfPoints() != 3)
    {
      vtkWarningMacro(<< "Expected line to have 3 points, but it had " << line->GetNumberOfPoints()
                      << " points.");
      return 0;
    }
    this->AngleRepresentation->SetPoint1WorldPosition(line->GetPoints()->GetPoint(0));
    this->AngleRepresentation->SetCenterWorldPosition(line->GetPoints()->GetPoint(1));
    this->AngleRepresentation->SetPoint2WorldPosition(line->GetPoints()->GetPoint(2));
    this->AngleRepresentation->SetForce3DArcPlacement(true);
    this->AngleRepresentation->BuildRepresentation();
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkProtractorRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AngleRepresentation: " << std::endl;
  this->AngleRepresentation->PrintSelf(os, indent.GetNextIndent());
}
