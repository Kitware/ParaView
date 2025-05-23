// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVAxesActor.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkFollower.h"
#include "vtkLineSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkVectorText.h"

#include <cmath>

vtkStandardNewMacro(vtkPVAxesActor);

vtkCxxSetObjectMacro(vtkPVAxesActor, UserDefinedTip, vtkPolyData);
vtkCxxSetObjectMacro(vtkPVAxesActor, UserDefinedShaft, vtkPolyData);

//-----------------------------------------------------------------------------
vtkPVAxesActor::vtkPVAxesActor()
{
  this->XAxisLabelText = nullptr;
  this->YAxisLabelText = nullptr;
  this->ZAxisLabelText = nullptr;

  this->SetXAxisLabelText("X");
  this->SetYAxisLabelText("Y");
  this->SetZAxisLabelText("Z");

  // colors chosen to match the output of vtkAxes.cxx's LUT.
  this->XAxisShaft = vtkActor::New();
  this->XAxisShaft->GetProperty()->SetColor(1, 0, 0);
  this->YAxisShaft = vtkActor::New();
  this->YAxisShaft->GetProperty()->SetColor(1, 1, 0);
  this->ZAxisShaft = vtkActor::New();
  this->ZAxisShaft->GetProperty()->SetColor(0, 1, 0);

  this->XAxisTip = vtkActor::New();
  this->XAxisTip->GetProperty()->SetColor(1, 0, 0);
  this->YAxisTip = vtkActor::New();
  this->YAxisTip->GetProperty()->SetColor(1, 1, 0);
  this->ZAxisTip = vtkActor::New();
  this->ZAxisTip->GetProperty()->SetColor(0, 1, 0);

  this->CylinderSource = vtkCylinderSource::New();
  this->CylinderSource->SetHeight(1.0);

  this->LineSource = vtkLineSource::New();
  this->LineSource->SetPoint1(0.0, 0.0, 0.0);
  this->LineSource->SetPoint2(0.0, 1.0, 0.0);

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetDirection(0, 1, 0);
  this->ConeSource->SetHeight(1.0);

  this->SphereSource = vtkSphereSource::New();

  vtkPolyDataMapper* shaftMapper = vtkPolyDataMapper::New();

  this->XAxisShaft->SetMapper(shaftMapper);
  this->YAxisShaft->SetMapper(shaftMapper);
  this->ZAxisShaft->SetMapper(shaftMapper);

  shaftMapper->Delete();

  vtkPolyDataMapper* tipMapper = vtkPolyDataMapper::New();

  this->XAxisTip->SetMapper(tipMapper);
  this->YAxisTip->SetMapper(tipMapper);
  this->ZAxisTip->SetMapper(tipMapper);

  tipMapper->Delete();

  this->TotalLength[0] = 1.0;
  this->TotalLength[1] = 1.0;
  this->TotalLength[2] = 1.0;

  this->NormalizedShaftLength[0] = 0.8;
  this->NormalizedShaftLength[1] = 0.8;
  this->NormalizedShaftLength[2] = 0.8;

  this->NormalizedTipLength[0] = 0.2;
  this->NormalizedTipLength[1] = 0.2;
  this->NormalizedTipLength[2] = 0.2;

  this->ConeResolution = 16;
  this->SphereResolution = 16;
  this->CylinderResolution = 16;

  this->ConeRadius = 0.4;
  this->SphereRadius = 0.5;
  this->CylinderRadius = 0.05;

  this->XAxisLabelPosition = 1;
  this->YAxisLabelPosition = 1;
  this->ZAxisLabelPosition = 1;

  this->ShaftType = vtkPVAxesActor::LINE_SHAFT;
  this->TipType = vtkPVAxesActor::CONE_TIP;

  this->UserDefinedTip = nullptr;
  this->UserDefinedShaft = nullptr;

  this->XAxisVectorText = vtkVectorText::New();
  this->YAxisVectorText = vtkVectorText::New();
  this->ZAxisVectorText = vtkVectorText::New();

  this->XAxisLabel = vtkFollower::New();
  this->YAxisLabel = vtkFollower::New();
  this->ZAxisLabel = vtkFollower::New();

  vtkPolyDataMapper* xmapper = vtkPolyDataMapper::New();
  vtkPolyDataMapper* ymapper = vtkPolyDataMapper::New();
  vtkPolyDataMapper* zmapper = vtkPolyDataMapper::New();

  xmapper->SetInputConnection(this->XAxisVectorText->GetOutputPort());
  ymapper->SetInputConnection(this->YAxisVectorText->GetOutputPort());
  zmapper->SetInputConnection(this->ZAxisVectorText->GetOutputPort());

  this->XAxisLabel->SetMapper(xmapper);
  this->YAxisLabel->SetMapper(ymapper);
  this->ZAxisLabel->SetMapper(zmapper);

  xmapper->Delete();
  ymapper->Delete();
  zmapper->Delete();

  this->UpdateProps();
}

//-----------------------------------------------------------------------------
vtkPVAxesActor::~vtkPVAxesActor()
{
  this->CylinderSource->Delete();
  this->LineSource->Delete();
  this->ConeSource->Delete();
  this->SphereSource->Delete();

  this->XAxisShaft->Delete();
  this->YAxisShaft->Delete();
  this->ZAxisShaft->Delete();

  this->XAxisTip->Delete();
  this->YAxisTip->Delete();
  this->ZAxisTip->Delete();

  this->SetUserDefinedTip(nullptr);
  this->SetUserDefinedShaft(nullptr);

  this->SetXAxisLabelText(nullptr);
  this->SetYAxisLabelText(nullptr);
  this->SetZAxisLabelText(nullptr);

  this->XAxisVectorText->Delete();
  this->YAxisVectorText->Delete();
  this->ZAxisVectorText->Delete();

  this->XAxisLabel->Delete();
  this->YAxisLabel->Delete();
  this->ZAxisLabel->Delete();
}

//-----------------------------------------------------------------------------
// Shallow copy of an actor.
void vtkPVAxesActor::ShallowCopy(vtkProp* prop)
{
  vtkPVAxesActor* a = vtkPVAxesActor::SafeDownCast(prop);
  if (a != nullptr)
  {
  }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::GetActors(vtkPropCollection* ac)
{
  ac->AddItem(this->XAxisShaft);
  ac->AddItem(this->YAxisShaft);
  ac->AddItem(this->ZAxisShaft);
  ac->AddItem(this->XAxisTip);
  ac->AddItem(this->YAxisTip);
  ac->AddItem(this->ZAxisTip);
  ac->AddItem(this->XAxisLabel);
  ac->AddItem(this->YAxisLabel);
  ac->AddItem(this->ZAxisLabel);
}

//-----------------------------------------------------------------------------
int vtkPVAxesActor::RenderOpaqueGeometry(vtkViewport* vp)
{
  int renderedSomething = 0;

  vtkRenderer* ren = vtkRenderer::SafeDownCast(vp);

  this->UpdateProps();

  this->XAxisLabel->SetCamera(ren->GetActiveCamera());
  this->YAxisLabel->SetCamera(ren->GetActiveCamera());
  this->ZAxisLabel->SetCamera(ren->GetActiveCamera());

  if (this->XAxisVisibility)
  {
    this->XAxisShaft->RenderOpaqueGeometry(vp);
    this->XAxisTip->RenderOpaqueGeometry(vp);
    this->XAxisLabel->RenderOpaqueGeometry(vp);
  }

  if (this->YAxisVisibility)
  {
    this->YAxisShaft->RenderOpaqueGeometry(vp);
    this->YAxisTip->RenderOpaqueGeometry(vp);
    this->YAxisLabel->RenderOpaqueGeometry(vp);
  }

  if (this->ZAxisVisibility)
  {
    this->ZAxisShaft->RenderOpaqueGeometry(vp);
    this->ZAxisTip->RenderOpaqueGeometry(vp);
    this->ZAxisLabel->RenderOpaqueGeometry(vp);
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkPVAxesActor::RenderTranslucentPolygonalGeometry(vtkViewport* vp)
{
  int renderedSomething = 0;

  this->UpdateProps();

  if (this->XAxisVisibility)
  {
    renderedSomething += this->XAxisShaft->RenderTranslucentPolygonalGeometry(vp);
    renderedSomething += this->XAxisTip->RenderTranslucentPolygonalGeometry(vp);
    renderedSomething += this->XAxisLabel->RenderTranslucentPolygonalGeometry(vp);
  }

  if (this->YAxisVisibility)
  {
    renderedSomething += this->YAxisShaft->RenderTranslucentPolygonalGeometry(vp);
    renderedSomething += this->YAxisTip->RenderTranslucentPolygonalGeometry(vp);
    renderedSomething += this->YAxisLabel->RenderTranslucentPolygonalGeometry(vp);
  }

  if (this->ZAxisVisibility)
  {
    renderedSomething += this->ZAxisShaft->RenderTranslucentPolygonalGeometry(vp);
    renderedSomething += this->ZAxisTip->RenderTranslucentPolygonalGeometry(vp);
    renderedSomething += this->ZAxisLabel->RenderTranslucentPolygonalGeometry(vp);
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
int vtkPVAxesActor::HasTranslucentPolygonalGeometry()
{
  int result = 0;

  this->UpdateProps();

  if (this->XAxisVisibility)
  {
    result |= this->XAxisShaft->HasTranslucentPolygonalGeometry();
    result |= this->XAxisTip->HasTranslucentPolygonalGeometry();
    result |= this->XAxisLabel->HasTranslucentPolygonalGeometry();
  }

  if (this->YAxisVisibility)
  {
    result |= this->YAxisShaft->HasTranslucentPolygonalGeometry();
    result |= this->YAxisTip->HasTranslucentPolygonalGeometry();
    result |= this->YAxisLabel->HasTranslucentPolygonalGeometry();
  }

  if (this->ZAxisVisibility)
  {
    result |= this->ZAxisShaft->HasTranslucentPolygonalGeometry();
    result |= this->ZAxisTip->HasTranslucentPolygonalGeometry();
    result |= this->ZAxisLabel->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::ReleaseGraphicsResources(vtkWindow* win)
{
  this->XAxisShaft->ReleaseGraphicsResources(win);
  this->YAxisShaft->ReleaseGraphicsResources(win);
  this->ZAxisShaft->ReleaseGraphicsResources(win);

  this->XAxisTip->ReleaseGraphicsResources(win);
  this->YAxisTip->ReleaseGraphicsResources(win);
  this->ZAxisTip->ReleaseGraphicsResources(win);

  this->XAxisLabel->ReleaseGraphicsResources(win);
  this->YAxisLabel->ReleaseGraphicsResources(win);
  this->ZAxisLabel->ReleaseGraphicsResources(win);
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::GetBounds(double bounds[6])
{
  double* bds = this->GetBounds();
  bounds[0] = bds[0];
  bounds[1] = bds[1];
  bounds[2] = bds[2];
  bounds[3] = bds[3];
  bounds[4] = bds[4];
  bounds[5] = bds[5];
}

//-----------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* vtkPVAxesActor::GetBounds()
{
  auto updateMaxs = [this](double* bounds)
  {
    for (int i = 0; i < 3; i++)
    {
      const int idx = 2 * i + 1;
      this->Bounds[idx] = std::max(bounds[idx], this->Bounds[idx]);
    }
  };

  double bounds[6]{ 0 };

  if (this->XAxisVisibility)
  {
    this->XAxisShaft->GetBounds(bounds);
    updateMaxs(bounds);
    this->XAxisTip->GetBounds(bounds);
    updateMaxs(bounds);
  }

  if (this->ZAxisVisibility)
  {
    this->ZAxisShaft->GetBounds(bounds);
    updateMaxs(bounds);
    this->ZAxisTip->GetBounds(bounds);
    updateMaxs(bounds);
  }

  if (this->YAxisVisibility)
  {
    this->YAxisShaft->GetBounds(bounds);
    updateMaxs(bounds);
    this->YAxisTip->GetBounds(bounds);
    updateMaxs(bounds);

    (vtkPolyDataMapper::SafeDownCast(this->YAxisShaft->GetMapper()))->GetInput()->GetBounds(bounds);
    updateMaxs(bounds);
  }

  // We want this actor to rotate / re-center about the origin, so give it
  // the bounds it would have if the axes were symmetrical.
  for (int i = 0; i < 3; i++)
  {
    this->Bounds[2 * i] = -this->Bounds[2 * i + 1];
  }

  return this->Bounds;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkPVAxesActor::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();

  return mTime;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkPVAxesActor::GetRedrawMTime()
{
  vtkMTimeType mTime = this->GetMTime();

  return mTime;
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetXAxisTipProperty()
{
  return this->XAxisTip->GetProperty();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetYAxisTipProperty()
{
  return this->YAxisTip->GetProperty();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetZAxisTipProperty()
{
  return this->ZAxisTip->GetProperty();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetXAxisShaftProperty()
{
  return this->XAxisShaft->GetProperty();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetYAxisShaftProperty()
{
  return this->YAxisShaft->GetProperty();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetZAxisShaftProperty()
{
  return this->ZAxisShaft->GetProperty();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetXAxisLabelProperty()
{
  return this->XAxisLabel->GetProperty();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetYAxisLabelProperty()
{
  return this->YAxisLabel->GetProperty();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkPVAxesActor::GetZAxisLabelProperty()
{
  return this->ZAxisLabel->GetProperty();
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::SetTotalLength(float x, float y, float z)
{
  if (this->TotalLength[0] != x || this->TotalLength[1] != y || this->TotalLength[2] != z)
  {
    this->TotalLength[0] = x;
    this->TotalLength[1] = y;
    this->TotalLength[2] = z;

    this->Modified();

    this->UpdateProps();
  }
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::SetNormalizedShaftLength(float x, float y, float z)
{
  if (this->NormalizedShaftLength[0] != x || this->NormalizedShaftLength[1] != y ||
    this->NormalizedShaftLength[2] != z)
  {
    this->NormalizedShaftLength[0] = x;
    this->NormalizedShaftLength[1] = y;
    this->NormalizedShaftLength[2] = z;

    this->Modified();

    this->UpdateProps();
  }
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::SetNormalizedTipLength(float x, float y, float z)
{
  if (this->NormalizedTipLength[0] != x || this->NormalizedTipLength[1] != y ||
    this->NormalizedTipLength[2] != z)
  {
    this->NormalizedTipLength[0] = x;
    this->NormalizedTipLength[1] = y;
    this->NormalizedTipLength[2] = z;

    this->Modified();

    this->UpdateProps();
  }
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::SetShaftType(int type)
{
  if (this->ShaftType != type)
  {
    this->ShaftType = type;

    this->Modified();

    this->UpdateProps();
  }
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::SetTipType(int type)
{
  if (this->TipType != type)
  {
    this->TipType = type;

    this->Modified();

    this->UpdateProps();
  }
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::UpdateProps()
{
  this->CylinderSource->SetRadius(this->CylinderRadius);
  this->CylinderSource->SetResolution(this->CylinderResolution);

  this->ConeSource->SetResolution(this->ConeResolution);
  this->ConeSource->SetRadius(this->ConeRadius);

  this->SphereSource->SetThetaResolution(this->SphereResolution);
  this->SphereSource->SetPhiResolution(this->SphereResolution);
  this->SphereSource->SetRadius(this->SphereRadius);

  switch (this->ShaftType)
  {
    case vtkPVAxesActor::CYLINDER_SHAFT:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))
        ->SetInputConnection(this->CylinderSource->GetOutputPort());
      break;
    case vtkPVAxesActor::LINE_SHAFT:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))
        ->SetInputConnection(this->LineSource->GetOutputPort());
      break;
    case vtkPVAxesActor::USER_DEFINED_SHAFT:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))
        ->SetInputData(this->UserDefinedShaft);
  }

  switch (this->TipType)
  {
    case vtkPVAxesActor::CONE_TIP:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))
        ->SetInputConnection(this->ConeSource->GetOutputPort());
      break;
    case vtkPVAxesActor::SPHERE_TIP:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))
        ->SetInputConnection(this->SphereSource->GetOutputPort());
      break;
    case vtkPVAxesActor::USER_DEFINED_TIP:
      (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))
        ->SetInputData(this->UserDefinedTip);
  }

  (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))->GetInputAlgorithm()->Update();
  (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))->GetInputAlgorithm()->Update();

  float scale[3];
  double bounds[6];

  (vtkPolyDataMapper::SafeDownCast(this->XAxisShaft->GetMapper()))->GetInput()->GetBounds(bounds);

  int i;
  for (i = 0; i < 3; i++)
  {
    scale[i] = this->NormalizedShaftLength[i] * this->TotalLength[i] / (bounds[3] - bounds[2]);
  }

  vtkTransform* xTransform = vtkTransform::New();
  vtkTransform* yTransform = vtkTransform::New();
  vtkTransform* zTransform = vtkTransform::New();

  xTransform->RotateZ(-90);
  zTransform->RotateX(90);

  xTransform->Scale(scale[0], scale[0], scale[0]);
  yTransform->Scale(scale[1], scale[1], scale[1]);
  zTransform->Scale(scale[2], scale[2], scale[2]);

  xTransform->Translate(-(bounds[0] + bounds[1]) / 2, -bounds[2], -(bounds[4] + bounds[5]) / 2);
  yTransform->Translate(-(bounds[0] + bounds[1]) / 2, -bounds[2], -(bounds[4] + bounds[5]) / 2);
  zTransform->Translate(-(bounds[0] + bounds[1]) / 2, -bounds[2], -(bounds[4] + bounds[5]) / 2);

  this->XAxisShaft->SetUserTransform(xTransform);
  this->YAxisShaft->SetUserTransform(yTransform);
  this->ZAxisShaft->SetUserTransform(zTransform);

  xTransform->Delete();
  yTransform->Delete();
  zTransform->Delete();

  (vtkPolyDataMapper::SafeDownCast(this->XAxisTip->GetMapper()))->GetInput()->GetBounds(bounds);

  xTransform = vtkTransform::New();
  yTransform = vtkTransform::New();
  zTransform = vtkTransform::New();

  xTransform->RotateZ(-90);
  zTransform->RotateX(90);

  xTransform->Scale(this->TotalLength[0], this->TotalLength[0], this->TotalLength[0]);
  yTransform->Scale(this->TotalLength[1], this->TotalLength[1], this->TotalLength[1]);
  zTransform->Scale(this->TotalLength[2], this->TotalLength[2], this->TotalLength[2]);

  xTransform->Translate(0, (1.0 - this->NormalizedTipLength[0]), 0);
  yTransform->Translate(0, (1.0 - this->NormalizedTipLength[1]), 0);
  zTransform->Translate(0, (1.0 - this->NormalizedTipLength[2]), 0);

  xTransform->Scale(
    this->NormalizedTipLength[0], this->NormalizedTipLength[0], this->NormalizedTipLength[0]);

  yTransform->Scale(
    this->NormalizedTipLength[1], this->NormalizedTipLength[1], this->NormalizedTipLength[1]);

  zTransform->Scale(
    this->NormalizedTipLength[2], this->NormalizedTipLength[2], this->NormalizedTipLength[2]);

  xTransform->Translate(-(bounds[0] + bounds[1]) / 2, -bounds[2], -(bounds[4] + bounds[5]) / 2);
  yTransform->Translate(-(bounds[0] + bounds[1]) / 2, -bounds[2], -(bounds[4] + bounds[5]) / 2);
  zTransform->Translate(-(bounds[0] + bounds[1]) / 2, -bounds[2], -(bounds[4] + bounds[5]) / 2);

  this->XAxisTip->SetUserTransform(xTransform);
  this->YAxisTip->SetUserTransform(yTransform);
  this->ZAxisTip->SetUserTransform(zTransform);

  xTransform->Delete();
  yTransform->Delete();
  zTransform->Delete();

  this->XAxisVectorText->SetText(this->XAxisLabelText);
  this->YAxisVectorText->SetText(this->YAxisLabelText);
  this->ZAxisVectorText->SetText(this->ZAxisLabelText);

  float avgScale = (this->TotalLength[0] + this->TotalLength[1] + this->TotalLength[2]) / 15;

  this->XAxisShaft->GetBounds(bounds);
  this->XAxisLabel->SetScale(avgScale, avgScale, avgScale);
  this->XAxisLabel->SetPosition(bounds[0] + this->XAxisLabelPosition * (bounds[1] - bounds[0]),
    bounds[2] - (bounds[3] - bounds[2]) * 2.0, bounds[5] + (bounds[5] - bounds[4]) / 2.0);

  this->YAxisShaft->GetBounds(bounds);
  this->YAxisLabel->SetScale(avgScale, avgScale, avgScale);
  this->YAxisLabel->SetPosition((bounds[0] + bounds[1]) / 2,
    bounds[2] + this->YAxisLabelPosition * (bounds[3] - bounds[2]),
    bounds[5] + (bounds[5] - bounds[4]) / 2.0);

  this->ZAxisShaft->GetBounds(bounds);
  this->ZAxisLabel->SetScale(avgScale, avgScale, avgScale);
  this->ZAxisLabel->SetPosition(bounds[0], bounds[2] - (bounds[3] - bounds[2]) * 2.0,
    bounds[4] + this->ZAxisLabelPosition * (bounds[5] - bounds[4]));

  this->XAxisLabel->SetVisibility(this->XAxisVisibility);
  this->XAxisShaft->SetVisibility(this->XAxisVisibility);
  this->XAxisTip->SetVisibility(this->XAxisVisibility);

  this->YAxisLabel->SetVisibility(this->YAxisVisibility);
  this->YAxisShaft->SetVisibility(this->YAxisVisibility);
  this->YAxisTip->SetVisibility(this->YAxisVisibility);

  this->ZAxisLabel->SetVisibility(this->ZAxisVisibility);
  this->ZAxisShaft->SetVisibility(this->ZAxisVisibility);
  this->ZAxisTip->SetVisibility(this->ZAxisVisibility);
}

//-----------------------------------------------------------------------------
void vtkPVAxesActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "UserDefinedShaft: ";
  if (this->UserDefinedShaft)
  {
    os << this->UserDefinedShaft << endl;
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "UserDefinedTip: ";
  if (this->UserDefinedTip)
  {
    os << this->UserDefinedTip << endl;
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "XAxisLabelText: " << (this->XAxisLabelText ? this->XAxisLabelText : "(none)")
     << endl;
  os << indent << "YAxisLabelText: " << (this->YAxisLabelText ? this->YAxisLabelText : "(none)")
     << endl;
  os << indent << "ZAxisLabelText: " << (this->ZAxisLabelText ? this->ZAxisLabelText : "(none)")
     << endl;
  os << indent << "XAxisLabelPosition: " << this->XAxisLabelPosition << endl;
  os << indent << "YAxisLabelPosition: " << this->YAxisLabelPosition << endl;
  os << indent << "ZAxisLabelPosition: " << this->ZAxisLabelPosition << endl;

  os << indent << "SphereRadius: " << this->SphereRadius << endl;
  os << indent << "SphereResolution: " << this->SphereResolution << endl;
  os << indent << "CylinderRadius: " << this->CylinderRadius << endl;
  os << indent << "CylinderResolution: " << this->CylinderResolution << endl;
  os << indent << "ConeRadius: " << this->ConeRadius << endl;
  os << indent << "ConeResolution: " << this->ConeResolution << endl;

  os << indent << "NormalizedShaftLength: " << this->NormalizedShaftLength[0] << ","
     << this->NormalizedShaftLength[1] << "," << this->NormalizedShaftLength[2] << endl;
  os << indent << "NormalizedTipLength: " << this->NormalizedTipLength[0] << ","
     << this->NormalizedTipLength[1] << "," << this->NormalizedTipLength[2] << endl;
  os << indent << "TotalLength: " << this->TotalLength[0] << "," << this->TotalLength[1] << ","
     << this->TotalLength[2] << endl;
}
