/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointHandleRepresentationSphere.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointHandleRepresentationSphere.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkDiskSource.h"
#include "vtkGlyph3D.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkWindow.h"

vtkStandardNewMacro(vtkPointHandleRepresentationSphere);

vtkCxxSetObjectMacro(vtkPointHandleRepresentationSphere, Property, vtkProperty);
vtkCxxSetObjectMacro(vtkPointHandleRepresentationSphere, SelectedProperty, vtkProperty);

//----------------------------------------------------------------------
vtkPointHandleRepresentationSphere::vtkPointHandleRepresentationSphere()
{
  // Initialize state
  this->InteractionState = vtkHandleRepresentation::Outside;

  // Represent the position of the cursor
  this->FocalPoint = vtkPoints::New();
  this->FocalPoint->SetNumberOfPoints(1);
  this->FocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  this->FocalData = vtkPolyData::New();
  this->FocalData->SetPoints(this->FocalPoint);

  // The transformation of the cursor will be done via vtkGlyph3D
  // By default a vtkSphereSource will be used to define the cursor shape
  vtkSphereSource* sphere = vtkSphereSource::New();
  sphere->SetThetaResolution(12);
  sphere->Update();
  this->CursorShape = sphere->GetOutput();
  this->CursorShape->Register(this);
  sphere->Delete();

  this->Glypher = vtkGlyph3D::New();
  this->Glypher->SetInputData(this->FocalData);
  this->Glypher->SetSourceData(this->CursorShape);
  this->Glypher->SetVectorModeToVectorRotationOff();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(10.0);

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  // The size of the hot spot
  this->WaitingForMotion = 0;
  this->ConstraintAxis = -1;

  this->Scalar = VTK_DOUBLE_MAX;

  this->AddCircleAroundSphere = 0;
  this->DiskActor = NULL;
  this->DiskMapper = NULL;
  this->DiskGlypher = NULL;
}

//----------------------------------------------------------------------
vtkPointHandleRepresentationSphere::~vtkPointHandleRepresentationSphere()
{
  this->FocalPoint->Delete();
  this->FocalData->Delete();

  this->CursorShape->Delete();
  this->Glypher->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();

  this->Property->Delete();
  this->SelectedProperty->Delete();

  if (this->DiskGlypher)
  {
    this->DiskGlypher->Delete();
  }
  if (this->DiskMapper)
  {
    this->DiskMapper->Delete();
  }
  if (this->DiskActor)
  {
    this->DiskActor->Delete();
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::SetCursorShape(vtkPolyData* shape)
{
  if (shape != this->CursorShape)
  {
    if (this->CursorShape)
    {
      this->CursorShape->Delete();
    }
    this->CursorShape = shape;
    if (this->CursorShape)
    {
      this->CursorShape->Register(this);
    }
    this->Glypher->SetSourceData(this->CursorShape);
    this->Modified();
  }
}

//----------------------------------------------------------------------
vtkPolyData* vtkPointHandleRepresentationSphere::GetCursorShape()
{
  return this->CursorShape;
}

//-------------------------------------------------------------------------
double* vtkPointHandleRepresentationSphere::GetBounds()
{
  return NULL;
}

//-------------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::SetDisplayPosition(double p[3])
{
  this->Superclass::SetDisplayPosition(p);
  this->FocalPoint->SetPoint(0, p);
  this->FocalPoint->Modified();
}

//-------------------------------------------------------------------------
int vtkPointHandleRepresentationSphere::ComputeInteractionState(
  int X, int Y, int vtkNotUsed(modify))
{
  double pos[3], xyz[3];
  this->FocalPoint->GetPoint(0, pos);
  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  xyz[2] = pos[2];

  this->VisibilityOn();
  double tol2 = this->Tolerance * this->Tolerance;
  if (vtkMath::Distance2BetweenPoints(xyz, pos) <= tol2)
  {
    this->InteractionState = vtkHandleRepresentation::Nearby;
  }
  else
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
    if (this->ActiveRepresentation)
    {
      this->VisibilityOff();
    }
  }

  return this->InteractionState;
}

//-------------------------------------------------------------------------
int vtkPointHandleRepresentationSphere::DetermineConstraintAxis(int constraint, double eventPos[2])
{
  // Look for trivial cases: either not constrained or already constrained
  if (!this->Constrained)
  {
    return -1;
  }
  else if (constraint >= 0 && constraint < 3)
  {
    return constraint;
  }

  // Okay, figure out constraint based on mouse motion
  double dpos[2];
  dpos[0] = fabs(eventPos[0] - this->StartEventPosition[0]);
  dpos[1] = fabs(eventPos[1] - this->StartEventPosition[1]);

  return (dpos[0] > dpos[1] ? 0 : 1);
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkPointHandleRepresentationSphere::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  this->ConstraintAxis = -1;
  this->WaitCount = 0;
  if (this->Constrained)
  {
    this->WaitingForMotion = 1;
  }
  else
  {
    this->WaitingForMotion = 0;
  }
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkPointHandleRepresentationSphere::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if (this->InteractionState == vtkHandleRepresentation::Selecting ||
    this->InteractionState == vtkHandleRepresentation::Translating)
  {
    if (!this->WaitingForMotion || this->WaitCount++ > 1)
    {

      this->ConstraintAxis = this->DetermineConstraintAxis(this->ConstraintAxis, eventPos);
      this->Translate(eventPos);
    }
  }

  else if (this->InteractionState == vtkHandleRepresentation::Scaling)
  {
    this->Scale(eventPos);
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];

  this->Modified();
}

//----------------------------------------------------------------------
// Translate everything
void vtkPointHandleRepresentationSphere::Translate(const double* eventPos)
{
  double pos[3], dpos[2];
  this->FocalPoint->GetPoint(0, pos);
  dpos[0] = eventPos[0] - pos[0];
  dpos[1] = eventPos[1] - pos[1];

  if (this->ConstraintAxis >= 0)
  {
    pos[this->ConstraintAxis] += dpos[this->ConstraintAxis];
  }
  else
  {
    pos[0] += dpos[0];
    pos[1] += dpos[1];
  }
  this->SetDisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::Scale(const double eventPos[2])
{
  // Get the current scale factor
  double sf = this->Glypher->GetScaleFactor();

  // Compute the scale factor
  int* size = this->Renderer->GetSize();
  double dPos = static_cast<double>(eventPos[1] - this->LastEventPosition[1]);
  sf *= (1.0 + 2.0 * (dPos / size[1])); // scale factor of 2.0 is arbitrary

  // Scale the handle
  this->Glypher->SetScaleFactor(sf);
  if (this->AddCircleAroundSphere && this->DiskGlypher)
  {
    this->DiskGlypher->SetScaleFactor(sf);
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::Highlight(int highlight)
{
  if (highlight)
  {
    this->Actor->SetProperty(this->SelectedProperty);
    if (this->AddCircleAroundSphere && this->DiskActor)
    {
      this->DiskActor->GetProperty()->SetColor(1.0, 0.0, 1.0);
    }
  }
  else
  {
    this->Actor->SetProperty(this->Property);
    if (this->AddCircleAroundSphere && this->DiskActor)
    {
      this->DiskActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
    }
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = vtkProperty::New();
  this->SelectedProperty->SetColor(0.0, 1.0, 0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetActiveCamera() &&
        this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime) ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime))
  {
    double p[3];
    this->GetDisplayPosition(p);
    this->FocalPoint->SetPoint(0, p);
    this->FocalPoint->Modified();
    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::ShallowCopy(vtkProp* prop)
{
  vtkPointHandleRepresentationSphere* rep = vtkPointHandleRepresentationSphere::SafeDownCast(prop);
  if (rep)
  {
    this->SetCursorShape(rep->GetCursorShape());
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::GetActors(vtkPropCollection* pc)
{
  this->Actor->GetActors(pc);
  if (this->AddCircleAroundSphere && this->DiskActor)
  {
    this->DiskActor->GetActors(pc);
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
  if (this->DiskActor)
  {
    this->DiskActor->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------
int vtkPointHandleRepresentationSphere::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->BuildRepresentation();
  int res = this->Actor->RenderOpaqueGeometry(viewport);
  if (res == 1 && this->AddCircleAroundSphere && this->DiskActor)
  {
    return this->DiskActor->RenderOpaqueGeometry(viewport);
  }
  return res;
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::SetAddCircleAroundSphere(int flag)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting AddCircleAroundSphere to "
                << flag);
  if (this->AddCircleAroundSphere != flag)
  {
    this->AddCircleAroundSphere = flag;
    this->Modified();

    if (this->AddCircleAroundSphere)
    {
      if (!this->DiskActor)
      {
        this->CreateDefaultDiskSource();
      }
      else
      {
        this->DiskActor->SetVisibility(1);
      }
    }
    else
    {
      if (this->DiskActor)
      {
        this->DiskActor->SetVisibility(0);
      }
    }
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::CreateDefaultDiskSource()
{
  vtkDiskSource* disk = vtkDiskSource::New();
  disk->SetCircumferentialResolution(36);
  disk->SetRadialResolution(5);
  disk->SetInnerRadius(0.5);
  disk->SetOuterRadius(0.65);
  vtkProperty* diskProperty = vtkProperty::New();
  diskProperty->SetColor(1.0, 1.0, 1.0);
  diskProperty->SetLineWidth(0.5);

  this->DiskGlypher = vtkGlyph3D::New();
  this->DiskGlypher->SetInputData(this->FocalData);
  this->DiskGlypher->SetSourceConnection(disk->GetOutputPort());
  this->DiskGlypher->SetVectorModeToVectorRotationOff();
  this->DiskGlypher->ScalingOn();
  this->DiskGlypher->SetScaleModeToDataScalingOff();
  this->DiskGlypher->SetScaleFactor(this->Glypher->GetScaleFactor());

  this->DiskMapper = vtkPolyDataMapper::New();
  this->DiskMapper->SetInputConnection(this->DiskGlypher->GetOutputPort());

  this->DiskActor = vtkActor::New();
  this->DiskActor->SetMapper(this->DiskMapper);
  this->DiskActor->SetProperty(diskProperty);

  diskProperty->Delete();
  disk->Delete();
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentationSphere::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->Property)
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if (this->SelectedProperty)
  {
    os << indent << "Selected Property: " << this->SelectedProperty << "\n";
  }
  else
  {
    os << indent << "Selected Property: (none)\n";
  }

  if (this->CursorShape)
  {
    os << indent << "Cursor Shape: " << this->CursorShape << "\n";
  }
  else
  {
    os << indent << "Cursor Shape: (none)\n";
  }

  os << indent << "Scalar: " << this->Scalar << endl;
  os << indent << "AddCircleAroundSphere: " << this->AddCircleAroundSphere << endl;
}
