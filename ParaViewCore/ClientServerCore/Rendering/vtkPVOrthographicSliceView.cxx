/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOrthographicSliceView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOrthographicSliceView.h"

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPVCenterAxesActor.h"
#include "vtkPVChangeOfBasisHelper.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderer.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"

#include <cassert>

class vtkPVOrthographicSliceViewInteractorStyle : public vtkPVInteractorStyle
{
public:
  static vtkPVOrthographicSliceViewInteractorStyle* New();
  vtkTypeMacro(vtkPVOrthographicSliceViewInteractorStyle, vtkPVInteractorStyle);

  vtkSetObjectMacro(OrthographicInteractorStyle, vtkPVInteractorStyle);
  vtkSetObjectMacro(PrimaryInteractorStyle, vtkPVInteractorStyle);
  vtkSetObjectMacro(PrimaryRenderer, vtkRenderer);

protected:
  vtkPVOrthographicSliceViewInteractorStyle():
    PrimaryInteractorStyle(NULL),
    OrthographicInteractorStyle(NULL),
    PrimaryRenderer(NULL)
    {
    }
  ~vtkPVOrthographicSliceViewInteractorStyle()
    {
    this->SetPrimaryInteractorStyle(NULL);
    this->SetOrthographicInteractorStyle(NULL);
    this->SetPrimaryRenderer(NULL);
    }

  virtual vtkCameraManipulator* FindManipulator(int button, int shift, int control)
    {
    if (this->CurrentRenderer == this->PrimaryRenderer)
      {
      return this->PrimaryInteractorStyle->FindManipulator(button, shift, control);
      }
    return this->OrthographicInteractorStyle->FindManipulator(button, shift, control);
    }

  vtkPVInteractorStyle* PrimaryInteractorStyle;
  vtkPVInteractorStyle* OrthographicInteractorStyle;
  vtkRenderer* PrimaryRenderer;

private:
  vtkPVOrthographicSliceViewInteractorStyle(const vtkPVOrthographicSliceViewInteractorStyle&);
  void operator=(const vtkPVOrthographicSliceViewInteractorStyle&);
};

vtkStandardNewMacro(vtkPVOrthographicSliceViewInteractorStyle);


vtkStandardNewMacro(vtkPVOrthographicSliceView);
//----------------------------------------------------------------------------
vtkPVOrthographicSliceView::vtkPVOrthographicSliceView()
  : Renderers(),
  OrthographicInteractorStyle(),
  SlicePositionAxes2D(),
  SlicePositionAxes3D()
{
  this->CenterAxes->SetVisibility(0);
  this->SliceIncrements[0] = this->SliceIncrements[1] = this->SliceIncrements[2] = 1.0;
  this->SlicePosition[0] = this->SlicePosition[1] = this->SlicePosition[2] = 0.0;

  vtkRenderWindow* window = this->GetRenderWindow();
  for (int cc=0; cc < 3; cc++)
    {
    this->Renderers[cc]->SetBackground(0.5, 0.5, 0.5);
    this->Renderers[cc]->GetActiveCamera()->SetParallelProjection(1);
    window->AddRenderer(this->Renderers[cc].GetPointer());
    this->SlicePositionAxes2D[cc]->SetComputeNormals(0);
    this->SlicePositionAxes2D[cc]->SetPickable(0);
    this->SlicePositionAxes2D[cc]->SetUseBounds(1);
    this->SlicePositionAxes2D[cc]->SetScale(10, 10, 10);
    this->Renderers[cc]->AddActor(this->SlicePositionAxes2D[cc].GetPointer());
    }

  this->SlicePositionAxes3D->SetComputeNormals(0);
  this->SlicePositionAxes3D->SetPickable(0);
  this->SlicePositionAxes3D->SetUseBounds(1);
  this->SlicePositionAxes3D->SetScale(10, 10, 10);
  this->GetRenderer()->AddActor(this->SlicePositionAxes3D.GetPointer());

  this->Renderers[YZ_PLANE]->GetActiveCamera()->SetPosition(1, 0, 0);
  this->Renderers[YZ_PLANE]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  this->Renderers[YZ_PLANE]->GetActiveCamera()->SetViewUp(0, 1, 0);

  this->Renderers[ZX_PLANE]->GetActiveCamera()->SetPosition(0, 1, 0);
  this->Renderers[ZX_PLANE]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  this->Renderers[ZX_PLANE]->GetActiveCamera()->SetViewUp(1, 0, 0);

  this->Renderers[XY_PLANE]->GetActiveCamera()->SetPosition(0, 0, 1);
  this->Renderers[XY_PLANE]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  this->Renderers[XY_PLANE]->GetActiveCamera()->SetViewUp(0, 1, 0);

  if (this->Interactor)
    {
    this->Interactor->SetInteractorStyle(this->OrthographicInteractorStyle.GetPointer());
    this->OrthographicInteractorStyle->SetPrimaryInteractorStyle(this->ThreeDInteractorStyle);
    this->OrthographicInteractorStyle->SetOrthographicInteractorStyle(this->TwoDInteractorStyle);
    this->OrthographicInteractorStyle->SetPrimaryRenderer(this->GetRenderer());

    this->Interactor->AddObserver(vtkCommand::MouseWheelForwardEvent,
      this, &vtkPVOrthographicSliceView::OnMouseWheelForwardEvent);
    this->Interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent,
      this, &vtkPVOrthographicSliceView::OnMouseWheelBackwardEvent);
    }

  this->SetSlicePosition(0, 0, 0);
}

//----------------------------------------------------------------------------
vtkPVOrthographicSliceView::~vtkPVOrthographicSliceView()
{
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetInteractionMode(int mode)
{
  this->Superclass::SetInteractionMode(mode);
  if (this->Interactor &&
    this->Interactor->GetInteractorStyle() != this->OrthographicInteractorStyle.GetPointer())
    {
    switch (this->InteractionMode)
      {
    case INTERACTION_MODE_3D:
    case INTERACTION_MODE_2D:
      this->OrthographicInteractorStyle->SetPrimaryInteractorStyle(
        vtkPVInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle()));
      this->Interactor->SetInteractorStyle(this->OrthographicInteractorStyle.GetPointer());
      break;

    default:
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::ResetCamera()
{
  for (int cc=0; cc < 3; cc++)
    {
    this->SlicePositionAxes2D[cc]->SetUseBounds(0);
    this->Renderers[cc]->ResetCamera();
    this->SlicePositionAxes2D[cc]->SetUseBounds(1);
    this->Renderers[cc]->ResetCameraClippingRange();
    }
  this->Superclass::ResetCamera();
}
//----------------------------------------------------------------------------
vtkRenderer* vtkPVOrthographicSliceView::GetRenderer(int index)
{
  assert(index>=0 && index <=2);
  return this->Renderers[index].GetPointer();
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::Initialize(unsigned int id)
{
  if (this->Identifier == id)
    {
    return;
    }

  this->Superclass::Initialize(id);

  double lowerRight[] = {0.5, 0, 1, 0.5};
  vtkRenderer* renderer = this->GetRenderer();
  renderer->SetViewport(lowerRight);
  this->SynchronizedWindows->UpdateRendererViewport(id, renderer, lowerRight);
  this->NonCompositedRenderer->SetViewport(lowerRight);
  this->SynchronizedWindows->UpdateRendererViewport(
    id, this->NonCompositedRenderer, lowerRight);

  double topRight[]={0.5, 0.5, 1, 1};
  this->Renderers[SIDE_VIEW]->SetViewport(topRight);
  this->SynchronizedWindows->AddRenderer(id, this->Renderers[SIDE_VIEW].GetPointer(), topRight);

  double topLeft[]={0, 0.5, 0.5, 1};
  this->Renderers[TOP_VIEW]->SetViewport(topLeft);
  this->SynchronizedWindows->AddRenderer(id, this->Renderers[TOP_VIEW].GetPointer(), topLeft);

  double lowerLeft[]={0, 0, 0.5, 0.5};
  this->Renderers[FRONT_VIEW]->SetViewport(lowerLeft);
  this->SynchronizedWindows->AddRenderer(id, this->Renderers[FRONT_VIEW].GetPointer(), lowerLeft);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::SetSlicePosition(double x, double y, double z)
{
  this->SetNumberOfXSlices(1);
  this->SetXSlices(&x);
  this->SetNumberOfYSlices(1);
  this->SetYSlices(&y);
  this->SetNumberOfZSlices(1);
  this->SetZSlices(&z);

  this->SlicePositionAxes2D[YZ_PLANE]->SetPosition(x+0.0001, y, z);
  this->SlicePositionAxes2D[ZX_PLANE]->SetPosition(x, y+0.0001, z);
  this->SlicePositionAxes2D[XY_PLANE]->SetPosition(x, y, z+0.0001);
  this->SlicePositionAxes3D->SetPosition(x, y, z);

  this->SlicePosition[0] = x;
  this->SlicePosition[1] = y;
  this->SlicePosition[2] = z;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::UpdateCenterAxes()
{
  vtkBoundingBox bbox(this->GeometryBounds);
  double widths[3];
  bbox.GetLengths(widths);
  widths[0] *= 2;
  widths[1] *= 2;
  widths[2] *= 2;
  this->SlicePositionAxes2D[0]->SetScale(widths);
  this->SlicePositionAxes2D[1]->SetScale(widths);
  this->SlicePositionAxes2D[2]->SetScale(widths);
  this->SlicePositionAxes3D->SetScale(widths);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::ResetCameraClippingRange()
{
  this->Superclass::ResetCameraClippingRange();
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::AboutToRenderOnLocalProcess(bool interactive)
{
  if (this->ModelTransformationMatrix->GetMTime() > this->ModelTransformationMatrixUpdateTime)
    {
    this->SlicePositionAxes2D[0]->SetUserMatrix(this->ModelTransformationMatrix.GetPointer());
    this->SlicePositionAxes2D[1]->SetUserMatrix(this->ModelTransformationMatrix.GetPointer());
    this->SlicePositionAxes2D[2]->SetUserMatrix(this->ModelTransformationMatrix.GetPointer());
    this->SlicePositionAxes3D->SetUserMatrix(this->ModelTransformationMatrix.GetPointer());
    this->ModelTransformationMatrixUpdateTime.Modified();
    }

  // Update camera directions.
  vtkVector3d axis[3];
  vtkPVChangeOfBasisHelper::GetBasisVectors(
    this->ModelTransformationMatrix.GetPointer(),
    axis[0], axis[1], axis[2]);
  for (int cc=0; cc < 3; cc++)
    {
    vtkCamera* camera = this->Renderers[cc]->GetActiveCamera();

    double fp[3];
    camera->GetFocalPoint(fp);

    double focaldistance = camera->GetDistance();
    axis[cc].Normalize();
    camera->SetPosition(
      fp[0] + axis[cc].GetData()[0] * focaldistance,
      fp[1] + axis[cc].GetData()[1] * focaldistance,
      fp[2] + axis[cc].GetData()[2] * focaldistance);
    }
  this->Superclass::AboutToRenderOnLocalProcess(interactive);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::Update()
{
  this->SlicePositionAxes3D->SetUseBounds(0);
  this->Superclass::Update();
  this->SlicePositionAxes3D->SetUseBounds(1);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::OnMouseWheelForwardEvent()
{
  vtkRenderer* ren = this->Interactor->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0],
    this->Interactor->GetEventPosition()[1]);

  double pos[3];
  this->GetSlicePosition(pos);
  for (int cc=0; cc < 3; cc++)
    {
    if (ren == this->Renderers[cc].GetPointer())
      {
      pos[cc] += this->SliceIncrements[cc];
      }
    }
  this->InvokeEvent(vtkCommand::MouseWheelForwardEvent, &pos);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::OnMouseWheelBackwardEvent()
{
  vtkRenderer* ren = this->Interactor->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0],
    this->Interactor->GetEventPosition()[1]);

  double pos[3];
  this->GetSlicePosition(pos);
  for (int cc=0; cc < 3; cc++)
    {
    if (ren == this->Renderers[cc].GetPointer())
      {
      pos[cc] -= this->SliceIncrements[cc];
      }
    }
  this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, pos);
}

//----------------------------------------------------------------------------
void vtkPVOrthographicSliceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
