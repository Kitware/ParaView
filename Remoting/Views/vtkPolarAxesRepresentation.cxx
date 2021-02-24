/*=========================================================================

  Program:   ParaView
  Module:    vtkPolarAxesRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  this->PolarAxesActor = vtkPolarAxesActor::New();
  this->PolarAxesActor->PickableOff();

  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
  this->Orientation[0] = this->Orientation[1] = this->Orientation[2] = 0.0;
  this->Scale[0] = this->Scale[1] = this->Scale[2] = 1.0;
  this->CustomBounds[0] = this->CustomBounds[2] = this->CustomBounds[4] = 0.0;
  this->CustomBounds[1] = this->CustomBounds[3] = this->CustomBounds[5] = 1.0;
  this->EnableCustomBounds[0] = 0;
  this->EnableCustomBounds[1] = 0;
  this->EnableCustomBounds[2] = 0;
  this->CustomRange[0] = 0.0;
  this->CustomRange[1] = 1.0;
  this->EnableCustomRange = 0;
  this->RendererType = vtkPVRenderView::DEFAULT_RENDERER;
  this->ParentVisibility = true;
  vtkMath::UninitializeBounds(this->DataBounds);
}

//----------------------------------------------------------------------------
vtkPolarAxesRepresentation::~vtkPolarAxesRepresentation()
{
  this->PolarAxesActor->Delete();
}

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
void vtkPolarAxesRepresentation::SetLastRadialAxisColor(double r, double g, double b)
{
  this->PolarAxesActor->GetLastRadialAxisProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetSecondaryRadialAxesColor(double r, double g, double b)
{
  this->PolarAxesActor->GetSecondaryRadialAxesProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarArcsColor(double r, double g, double b)
{
  this->PolarAxesActor->GetPolarArcsProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetSecondaryPolarArcsColor(double r, double g, double b)
{
  this->PolarAxesActor->GetSecondaryPolarArcsProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetPolarAxisColor(double r, double g, double b)
{
  this->PolarAxesActor->GetPolarAxisProperty()->SetColor(r, g, b);
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

  // calcul du pole
  this->PolarAxesActor->SetPole((bds[0] + bds[1]) * 0.5, (bds[2] + bds[3]) * 0.5, 0);

  double maxradius = 0.0;
  double pole[3];
  this->PolarAxesActor->GetPole(pole);
  for (int i = 0; i < 2; i++)
  {
    double currentradius = 0.0;
    currentradius = sqrt(pow(bds[i] - pole[0], 2) + pow(bds[i + 2] - pole[1], 2));
    maxradius = (maxradius < currentradius) ? currentradius : maxradius;
  }
  this->PolarAxesActor->SetMaximumRadius(maxradius);

  if (this->EnableCustomRange)
  {
    this->PolarAxesActor->SetRange(this->CustomRange);
  }
  else
  {
    this->PolarAxesActor->SetRange(0, maxradius);
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
void vtkPolarAxesRepresentation::SetMinimumRadius(double radius)
{
  this->PolarAxesActor->SetMinimumRadius(radius);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetMinimumAngle(double angle)
{
  this->PolarAxesActor->SetMinimumAngle(angle);
}

//----------------------------------------------------------------------------
void vtkPolarAxesRepresentation::SetMaximumAngle(double angle)
{
  this->PolarAxesActor->SetMaximumAngle(angle);
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
double vtkPolarAxesRepresentation::GetDeltaRangeMajor()
{
  return this->PolarAxesActor->GetDeltaRangeMajor();
}

//----------------------------------------------------------------------------
double vtkPolarAxesRepresentation::GetDeltaRangeMinor()
{
  return this->PolarAxesActor->GetDeltaRangeMinor();
}
