// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_3_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkPolarAxesRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkAxisActor.h"
#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPVChangeOfBasisHelper.h"
#include "vtkPVRenderView.h"
#include "vtkPolarAxesActor.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkPolarAxesRepresentation);
//----------------------------------------------------------------------------
vtkPolarAxesRepresentation::vtkPolarAxesRepresentation()
{
  this->PolarAxesActor->PickableOff();
  vtkMath::UninitializeBounds(this->DataBounds);
}

//----------------------------------------------------------------------------
vtkPolarAxesRepresentation::~vtkPolarAxesRepresentation() = default;

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetVisibility(bool val)
{
  if (this->GetVisibility() != val)
  {
    this->Superclass::SetVisibility(val);
    this->PolarAxesActor->SetVisibility((this->ParentVisibility && val) ? 1 : 0);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetParentVisibility(bool val)
{
  if (this->ParentVisibility != val)
  {
    this->ParentVisibility = val;
    this->PolarAxesActor->SetVisibility((val && this->GetVisibility()) ? 1 : 0);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetEnableOverallColor(bool enable)
{
  if (this->EnableOverallColor != enable)
  {
    this->EnableOverallColor = enable;
    this->Modified();

    if (this->EnableOverallColor)
    {
      this->PolarAxesActor->GetLastRadialAxisProperty()->SetColor(
        this->OverallColor[0], this->OverallColor[1], this->OverallColor[2]);
      this->PolarAxesActor->GetSecondaryRadialAxesProperty()->SetColor(
        this->OverallColor[0], this->OverallColor[1], this->OverallColor[2]);
      this->PolarAxesActor->GetPolarArcsProperty()->SetColor(
        this->OverallColor[0], this->OverallColor[1], this->OverallColor[2]);
      this->PolarAxesActor->GetSecondaryPolarArcsProperty()->SetColor(
        this->OverallColor[0], this->OverallColor[1], this->OverallColor[2]);
      this->PolarAxesActor->GetPolarAxisProperty()->SetColor(
        this->OverallColor[0], this->OverallColor[1], this->OverallColor[2]);
    }
    else
    {
      this->PolarAxesActor->GetLastRadialAxisProperty()->SetColor(
        this->LastRadialAxisColor[0], this->LastRadialAxisColor[1], this->LastRadialAxisColor[2]);
      this->PolarAxesActor->GetSecondaryRadialAxesProperty()->SetColor(
        this->SecondaryRadialAxesColor[0], this->SecondaryRadialAxesColor[1],
        this->SecondaryRadialAxesColor[2]);
      this->PolarAxesActor->GetPolarArcsProperty()->SetColor(
        this->PolarArcsColor[0], this->PolarArcsColor[1], this->PolarArcsColor[2]);
      this->PolarAxesActor->GetSecondaryPolarArcsProperty()->SetColor(
        this->SecondaryPolarArcsColor[0], this->SecondaryPolarArcsColor[1],
        this->SecondaryPolarArcsColor[2]);
      this->PolarAxesActor->GetPolarAxisProperty()->SetColor(
        this->PolarAxisColor[0], this->PolarAxisColor[1], this->PolarAxisColor[2]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetOverallColor(double r, double g, double b)
{
  if (this->EnableOverallColor &&
    (this->OverallColor[0] != r || this->OverallColor[1] != g || this->OverallColor[2] != b))
  {
    this->OverallColor[0] = r;
    this->OverallColor[1] = g;
    this->OverallColor[2] = b;
    this->Modified();

    this->PolarAxesActor->GetLastRadialAxisProperty()->SetColor(r, g, b);
    this->PolarAxesActor->GetSecondaryRadialAxesProperty()->SetColor(r, g, b);
    this->PolarAxesActor->GetPolarArcsProperty()->SetColor(r, g, b);
    this->PolarAxesActor->GetSecondaryPolarArcsProperty()->SetColor(r, g, b);
    this->PolarAxesActor->GetPolarAxisProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetLastRadialAxisColor(double r, double g, double b)
{
  if (!this->EnableOverallColor &&
    (this->LastRadialAxisColor[0] != r || this->LastRadialAxisColor[1] != g ||
      this->LastRadialAxisColor[2] != b))
  {
    this->LastRadialAxisColor[0] = r;
    this->LastRadialAxisColor[1] = g;
    this->LastRadialAxisColor[2] = b;
    this->Modified();

    this->PolarAxesActor->GetLastRadialAxisProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetSecondaryRadialAxesColor(double r, double g, double b)
{
  if (!this->EnableOverallColor &&
    (this->SecondaryRadialAxesColor[0] != r || this->SecondaryRadialAxesColor[1] != g ||
      this->SecondaryRadialAxesColor[2] != b))
  {
    this->SecondaryRadialAxesColor[0] = r;
    this->SecondaryRadialAxesColor[1] = g;
    this->SecondaryRadialAxesColor[2] = b;
    this->Modified();

    this->PolarAxesActor->GetSecondaryRadialAxesProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarArcsColor(double r, double g, double b)
{
  if (!this->EnableOverallColor &&
    (this->PolarArcsColor[0] != r || this->PolarArcsColor[1] != g || this->PolarArcsColor[2] != b))
  {
    this->PolarArcsColor[0] = r;
    this->PolarArcsColor[1] = g;
    this->PolarArcsColor[2] = b;
    this->Modified();

    this->PolarAxesActor->GetPolarArcsProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetSecondaryPolarArcsColor(double r, double g, double b)
{
  if (!this->EnableOverallColor &&
    (this->SecondaryPolarArcsColor[0] != r || this->SecondaryPolarArcsColor[1] != g ||
      this->SecondaryPolarArcsColor[2] != b))
  {
    this->SecondaryPolarArcsColor[0] = r;
    this->SecondaryPolarArcsColor[1] = g;
    this->SecondaryPolarArcsColor[2] = b;
    this->Modified();

    this->PolarAxesActor->GetSecondaryPolarArcsProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisColor(double r, double g, double b)
{
  if (!this->EnableOverallColor &&
    (this->PolarAxisColor[0] != r || this->PolarAxisColor[1] != g || this->PolarAxisColor[2] != b))
  {
    this->PolarAxisColor[0] = r;
    this->PolarAxisColor[1] = g;
    this->PolarAxisColor[2] = b;
    this->Modified();

    this->PolarAxesActor->GetPolarAxisProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisTitleTextProperty(vtkTextProperty* prop)
{
  this->PolarAxesActor->SetPolarAxisTitleTextProperty(prop);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisLabelTextProperty(vtkTextProperty* prop)
{
  this->PolarAxesActor->SetPolarAxisLabelTextProperty(prop);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetLastRadialAxisTextProperty(vtkTextProperty* prop)
{
  this->PolarAxesActor->SetLastRadialAxisTextProperty(prop);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetSecondaryRadialAxesTextProperty(vtkTextProperty* prop)
{
  this->PolarAxesActor->SetSecondaryRadialAxesTextProperty(prop);
}

//----------------------------------------------------------------------------
bool vtkPolarAxesRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
  {
    if (vtkRenderer* renderer = pvview->GetRenderer(this->RendererType))
    {
      renderer->AddActor(this->PolarAxesActor);
      this->PolarAxesActor->SetCamera(renderer->GetActiveCamera());
      this->RenderView = pvview;
      return this->Superclass::AddToView(view);
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPolarAxesRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
  {
    if (vtkRenderer* renderer = pvview->GetRenderer(this->RendererType))
    {
      renderer->RemoveActor(this->PolarAxesActor);
      this->PolarAxesActor->SetCamera(nullptr);
      this->RenderView = nullptr;
      return this->Superclass::RemoveFromView(view);
    }
  }
  this->RenderView = nullptr;
  return false;
}

//----------------------------------------------------------------------------
int vtkPolarAxesRepresentation::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPolarAxesRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->OutlineGeometry.Get());
    vtkPVRenderView::SetDeliverToAllProcesses(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    if (producerPort)
    {
      vtkAlgorithm* producer = producerPort->GetProducer();
      vtkDataObject* data =
        vtkDataObject::SafeDownCast(producer->GetOutputDataObject(producerPort->GetIndex()));
      if ((data && data->GetMTime() > this->DataBoundsTime.GetMTime()) ||
        this->GetMTime() > this->DataBoundsTime.GetMTime())
      {
        this->InitializeDataBoundsFromData(data);
        this->UpdateBounds();
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPolarAxesRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    this->InitializeDataBoundsFromData(input);
  }

  if (vtkMath::AreBoundsInitialized(this->DataBounds))
  {
    vtkNew<vtkOutlineSource> outlineSource;
    outlineSource->SetBoxTypeToAxisAligned();
    outlineSource->SetBounds(this->DataBounds);
    outlineSource->Update();
    this->OutlineGeometry->ShallowCopy(outlineSource->GetOutput());
  }
  else
  {
    this->OutlineGeometry->Initialize();
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::InitializeDataBoundsFromData(vtkDataObject* data)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(data);
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(data);
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(data);
  if (ds)
  {
    ds->GetBounds(this->DataBounds);
  }
  else if (cd)
  {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    vtkBoundingBox bbox;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        double bds[6];
        ds->GetBounds(bds);
        if (vtkMath::AreBoundsInitialized(bds))
        {
          bbox.AddBounds(bds);
        }
      }
    }
    iter->Delete();
    bbox.GetBounds(this->DataBounds);
  }
  else if (htg)
  {
    htg->GetBounds(this->DataBounds);
  }
  this->DataBoundsTime.Modified();
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::UpdateBounds()
{
  double* scale = this->Scale;
  double* position = this->Position;
  double* rotation = this->Orientation;
  double bds[6];
  if (scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0 || position[0] != 0.0 ||
    position[1] != 0.0 || position[2] != 0.0 || rotation[0] != 0.0 || rotation[1] != 0.0 ||
    rotation[2] != 0.0)
  {
    const double* bounds = this->DataBounds;
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(position);
    transform->RotateZ(rotation[2]);
    transform->RotateX(rotation[0]);
    transform->RotateY(rotation[1]);
    transform->Scale(scale);
    vtkBoundingBox bbox;
    int i, j, k;
    double origX[3], x[3];

    for (i = 0; i < 2; i++)
    {
      origX[0] = bounds[i];
      for (j = 0; j < 2; j++)
      {
        origX[1] = bounds[2 + j];
        for (k = 0; k < 2; k++)
        {
          origX[2] = bounds[4 + k];
          transform->TransformPoint(origX, x);
          bbox.AddPoint(x);
        }
      }
    }
    bbox.GetBounds(bds);
  }
  else
  {
    memcpy(bds, this->DataBounds, sizeof(double) * 6);
  }

  // overload bounds with the active custom bounds
  for (int i = 0; i < 3; ++i)
  {
    int pos = i * 2;
    if (this->EnableCustomBounds[i])
    {
      bds[pos] = this->CustomBounds[pos];
      bds[pos + 1] = this->CustomBounds[pos + 1];
    }
  }

  this->PolarAxesActor->SetBounds(bds);

  double pole[3] = { 0.0, 0.0, 0.0 };
  double center[2] = { (bds[0] + bds[1]) * 0.5, (bds[2] + bds[3]) * 0.5 };
  double maxRadius = 0.0;
  double minRadius = EnableCustomRadius ? this->MinRadius : 0.0;
  double minAngle = EnableCustomAngle ? this->MinAngle : 0.0;
  double maxAngle = EnableCustomAngle ? this->MaxAngle : 360.0;

  if (this->EnableAutoPole)
  {
    this->PolarAxesActor->SetPole(center);

    maxRadius = sqrt(pow(bds[1] - center[0], 2) + pow(bds[3] - center[1], 2));
  }
  else
  {
    for (std::size_t ind{ 0 }; ind < 3; ++ind)
    {
      pole[ind] = position[ind];
    }

    this->PolarAxesActor->SetPole(pole);

    // Compute the max length between pole and bounds for maximum radius
    // Check bottom-left, top-left, bottom-right, top-right
    if (pole[0] < center[0])
    {
      if (pole[1] < center[1])
      {
        maxRadius = sqrt(pow(bds[1] - pole[0], 2) + pow(bds[3] - pole[1], 2));
      }
      else
      {
        maxRadius = sqrt(pow(bds[1] - pole[0], 2) + pow(bds[2] - pole[1], 2));
      }
    }
    else
    {
      if (pole[1] < center[1])
      {
        maxRadius = sqrt(pow(bds[0] - pole[0], 2) + pow(bds[3] - pole[1], 2));
      }
      else
      {
        maxRadius = sqrt(pow(bds[0] - pole[0], 2) + pow(bds[2] - pole[1], 2));
      }
    }
    // Compute the min length between pole and bounds if pole is outside box for minimum radius and
    // min/max angle
    // Check bottom-left, top-left, left, bottom-right, top-right, right, bottom, top
    // If inside, keep default values
    if (pole[0] < bds[0])
    {
      if (!this->EnableCustomRadius)
      {
        if (pole[1] < bds[2])
        {
          minRadius = sqrt(pow(bds[0] - pole[0], 2) + pow(bds[2] - pole[1], 2));
        }
        else if (pole[1] > bds[3])
        {
          minRadius = sqrt(pow(bds[0] - pole[0], 2) + pow(pole[1] - bds[3], 2));
        }
        else
        {
          minRadius = bds[0] - pole[0];
        }
      }

      if (!this->EnableCustomAngle)
      {
        maxAngle = ((pole[1] < bds[3]) ? atan((bds[3] - pole[1]) / (bds[0] - pole[0]))
                                       : atan((bds[3] - pole[1]) / (bds[1] - pole[0]))) *
          180.0 / vtkMath::Pi();
        minAngle = ((pole[1] < bds[2]) ? atan((bds[2] - pole[1]) / (bds[1] - pole[0]))
                                       : atan((bds[2] - pole[1]) / (bds[0] - pole[0]))) *
          180.0 / vtkMath::Pi();
      }
    }
    else if (pole[0] > bds[1])
    {
      if (!this->EnableCustomRadius)
      {
        if (pole[1] < bds[2])
        {
          minRadius = sqrt(pow(pole[0] - bds[1], 2) + pow(bds[2] - pole[1], 2));
        }
        else if (pole[1] > bds[3])
        {
          minRadius = sqrt(pow(pole[0] - bds[1], 2) + pow(pole[1] - bds[3], 2));
        }
        else
        {
          minRadius = pole[0] - bds[1];
        }
      }

      if (!this->EnableCustomAngle)
      {
        maxAngle = 180 +
          ((pole[1] < bds[2]) ? atan((bds[2] - pole[1]) / (bds[0] - pole[0]))
                              : atan((bds[2] - pole[1]) / (bds[1] - pole[0]))) *
            180 / vtkMath::Pi();
        minAngle = 180 +
          ((pole[1] < bds[3]) ? atan((bds[3] - pole[1]) / (bds[1] - pole[0]))
                              : atan((bds[3] - pole[1]) / (bds[0] - pole[0]))) *
            180 / vtkMath::Pi();
      }
    }
    else if (pole[1] < bds[2])
    {
      if (!this->EnableCustomRadius)
      {
        minRadius = bds[2] - pole[1];
      }

      if (!this->EnableCustomAngle)
      {
        maxAngle = 180 + atan((bds[2] - pole[1]) / (bds[0] - pole[0])) * 180 / vtkMath::Pi();
        minAngle = atan((bds[2] - pole[1]) / (bds[1] - pole[0])) * 180 / vtkMath::Pi();
      }
    }
    else if (pole[1] > bds[3])
    {
      if (!this->EnableCustomRadius)
      {
        minRadius = pole[1] - bds[3];
      }

      if (!this->EnableCustomAngle)
      {
        maxAngle = atan((bds[3] - pole[1]) / (bds[1] - pole[0])) * 180 / vtkMath::Pi();
        minAngle = 180 + atan((bds[3] - pole[1]) / (bds[0] - pole[0])) * 180 / vtkMath::Pi();
      }
    }
  }

  this->PolarAxesActor->SetMinimumRadius(minRadius);
  this->PolarAxesActor->SetMaximumRadius(maxRadius);
  this->PolarAxesActor->SetMinimumAngle(minAngle);
  this->PolarAxesActor->SetMaximumAngle(maxAngle);

  if (this->EnableCustomRange)
  {
    this->PolarAxesActor->SetRange(this->CustomRange);
  }
  else
  {
    this->PolarAxesActor->SetRange(minRadius, maxRadius);
  }
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->PolarAxesActor->PrintSelf(os, indent.GetNextIndent());
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetLog(bool active)
{
  this->PolarAxesActor->SetLog(active);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetNumberOfRadialAxes(vtkIdType val)
{
  this->PolarAxesActor->SetRequestedNumberOfRadialAxes(val);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetNumberOfPolarAxes(vtkIdType val)
{
  this->PolarAxesActor->SetRequestedNumberOfPolarAxes(val);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetNumberOfPolarAxisTicks(int val)
{
  this->PolarAxesActor->SetNumberOfPolarAxisTicks(val);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetAutoSubdividePolarAxis(bool active)
{
  this->PolarAxesActor->SetAutoSubdividePolarAxis(active);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDeltaAngleRadialAxes(double angle)
{
  this->PolarAxesActor->SetRequestedDeltaAngleRadialAxes(angle);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDeltaRangePolarAxes(double range)
{
  this->PolarAxesActor->SetRequestedDeltaRangePolarAxes(range);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetSmallestVisiblePolarAngle(double angle)
{
  this->PolarAxesActor->SetSmallestVisiblePolarAngle(angle);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetTickLocation(int location)
{
  this->PolarAxesActor->SetTickLocation(location);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetRadialUnits(bool use)
{
  this->PolarAxesActor->SetRadialUnits(use);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetScreenSize(double size)
{
  this->PolarAxesActor->SetScreenSize(size);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisTitle(const char* title)
{
  this->PolarAxesActor->SetPolarAxisTitle(title);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarLabelFormat(const char* format)
{
  this->PolarAxesActor->SetPolarLabelFormat(format);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetExponentLocation(int location)
{
  this->PolarAxesActor->SetExponentLocation(location);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetRadialAngleFormat(const char* format)
{
  this->PolarAxesActor->SetRadialAngleFormat(format);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetEnableDistanceLOD(int enable)
{
  this->PolarAxesActor->SetEnableDistanceLOD(enable);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDistanceLODThreshold(double val)
{
  this->PolarAxesActor->SetDistanceLODThreshold(val);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetEnableViewAngleLOD(int enable)
{
  this->PolarAxesActor->SetEnableViewAngleLOD(enable);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetViewAngleLODThreshold(double val)
{
  this->PolarAxesActor->SetViewAngleLODThreshold(val);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisVisibility(int visible)
{
  this->PolarAxesActor->SetPolarAxisVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDrawRadialGridlines(int draw)
{
  this->PolarAxesActor->SetDrawRadialGridlines(draw);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDrawPolarArcsGridlines(int draw)
{
  this->PolarAxesActor->SetDrawPolarArcsGridlines(draw);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarTitleVisibility(int visible)
{
  this->PolarAxesActor->SetPolarTitleVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetRadialAxisTitleLocation(int location)
{
  this->PolarAxesActor->SetRadialAxisTitleLocation(location);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisTitleLocation(int location)
{
  this->PolarAxesActor->SetPolarAxisTitleLocation(location);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetRadialTitleOffset(double offsetX, double offsetY)
{
  this->PolarAxesActor->SetRadialTitleOffset(offsetX, offsetY);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarTitleOffset(double offsetX, double offsetY)
{
  this->PolarAxesActor->SetPolarTitleOffset(offsetX, offsetY);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarLabelOffset(double offsetY)
{
  this->PolarAxesActor->SetPolarLabelOffset(offsetY);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarExponentOffset(double offsetY)
{
  this->PolarAxesActor->SetPolarExponentOffset(offsetY);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarLabelVisibility(int visible)
{
  this->PolarAxesActor->SetPolarLabelVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetArcTicksOriginToPolarAxis(int use)
{
  this->PolarAxesActor->SetArcTicksOriginToPolarAxis(use);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetRadialAxesOriginToPolarAxis(int use)
{
  this->PolarAxesActor->SetRadialAxesOriginToPolarAxis(use);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarTickVisibility(int visible)
{
  this->PolarAxesActor->SetPolarTickVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetAxisTickVisibility(int visible)
{
  this->PolarAxesActor->SetAxisTickVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetAxisMinorTickVisibility(int visible)
{
  this->PolarAxesActor->SetAxisMinorTickVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetAxisTickMatchesPolarAxes(int enable)
{
  this->PolarAxesActor->SetAxisTickMatchesPolarAxes(enable);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetArcTickVisibility(int visible)
{
  this->PolarAxesActor->SetArcTickVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetArcMinorTickVisibility(int visible)
{
  this->PolarAxesActor->SetArcMinorTickVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetArcTickMatchesRadialAxes(int enable)
{
  this->PolarAxesActor->SetArcTickMatchesRadialAxes(enable);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetTickRatioRadiusSize(double ratio)
{
  this->PolarAxesActor->SetTickRatioRadiusSize(ratio);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetArcMajorTickSize(double size)
{
  this->PolarAxesActor->SetArcMajorTickSize(size);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisMajorTickSize(double size)
{
  this->PolarAxesActor->SetPolarAxisMajorTickSize(size);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetLastRadialAxisMajorTickSize(double size)
{
  this->PolarAxesActor->SetLastRadialAxisMajorTickSize(size);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisTickRatioSize(double size)
{
  this->PolarAxesActor->SetPolarAxisTickRatioSize(size);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetLastAxisTickRatioSize(double size)
{
  this->PolarAxesActor->SetLastAxisTickRatioSize(size);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetArcTickRatioSize(double size)
{
  this->PolarAxesActor->SetArcTickRatioSize(size);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisMajorTickThickness(double thickness)
{
  this->PolarAxesActor->SetPolarAxisMajorTickThickness(thickness);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetLastRadialAxisMajorTickThickness(double thickness)
{
  this->PolarAxesActor->SetLastRadialAxisMajorTickThickness(thickness);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetArcMajorTickThickness(double thickness)
{
  this->PolarAxesActor->SetArcMajorTickThickness(thickness);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisTickRatioThickness(double thickness)
{
  this->PolarAxesActor->SetPolarAxisTickRatioThickness(thickness);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetLastAxisTickRatioThickness(double thickness)
{
  this->PolarAxesActor->SetLastAxisTickRatioThickness(thickness);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetArcTickRatioThickness(double thickness)
{
  this->PolarAxesActor->SetArcTickRatioThickness(thickness);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDeltaAngleMajor(double range)
{
  this->PolarAxesActor->SetDeltaAngleMajor(range);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDeltaAngleMinor(double range)
{
  this->PolarAxesActor->SetDeltaAngleMinor(range);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetRadialAxesVisibility(int visible)
{
  this->PolarAxesActor->SetRadialAxesVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetRadialTitleVisibility(int visible)
{
  this->PolarAxesActor->SetRadialTitleVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarArcsVisibility(int visible)
{
  this->PolarAxesActor->SetPolarArcsVisibility(visible);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetUse2DMode(int use)
{
  this->PolarAxesActor->SetUse2DMode(use);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetRatio(double ratio)
{
  this->PolarAxesActor->SetRatio(ratio);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarArcResolutionPerDegree(double resolution)
{
  this->PolarAxesActor->SetPolarArcResolutionPerDegree(resolution);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDeltaRangeMajor(double delta)
{
  this->PolarAxesActor->SetDeltaRangeMajor(delta);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetDeltaRangeMinor(double delta)
{
  this->PolarAxesActor->SetDeltaRangeMinor(delta);
}
