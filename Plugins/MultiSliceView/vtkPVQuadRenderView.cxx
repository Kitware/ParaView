/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVQuadRenderView.h"

#include "vtk3DWidgetRepresentation.h"
#include "vtkAbstractWidget.h"
#include "vtkCamera.h"
#include "vtkCameraManipulator.h"
#include "vtkCommand.h"
#include "vtkHandleRepresentation.h"
#include "vtkHandleWidget.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkPointSource.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkWeakPointer.h"

#include <sstream>

class vtkQuadInternal;
vtkQuadInternal* QuadInternal;
//============================================================================
class vtkPVQuadRenderView::vtkQuadInternal
{
public:
  vtkQuadInternal(vtkPVQuadRenderView* parent)
  {
    this->Owner = parent;
    this->ObserverId = 0;

    this->Owner->GetOrthoRenderView(TOP_LEFT)->GetNonCompositedRenderer()->AddActor(this->CoordinatesX.GetPointer());
    this->Owner->GetOrthoRenderView(TOP_RIGHT)->GetNonCompositedRenderer()->AddActor(this->CoordinatesY.GetPointer());
    this->Owner->GetOrthoRenderView(BOTTOM_LEFT)->GetNonCompositedRenderer()->AddActor(this->CoordinatesZ.GetPointer());
//    this->CoordinatesX->SetPosition(.95,.05);
//    this->CoordinatesX->GetTextProperty()->SetJustificationToRight();
//    this->CoordinatesX->GetTextProperty()->SetVerticalJustificationToBottom();
//    this->CoordinatesY->SetPosition(.05,.05);
//    this->CoordinatesY->GetTextProperty()->SetJustificationToLeft();
//    this->CoordinatesY->GetTextProperty()->SetVerticalJustificationToBottom();
//    this->CoordinatesZ->SetPosition(.95,.95);
//    this->CoordinatesZ->GetTextProperty()->SetJustificationToRight();
//    this->CoordinatesZ->GetTextProperty()->SetVerticalJustificationToTop();
  }

  void SetSliceOriginSource(vtkPointSource* source)
  {
    if(this->ObserverId && this->SliceOriginSource)
      {
      this->SliceOriginSource->RemoveObserver(this->ObserverId);
      this->ObserverId = 0;
      }

    this->SliceOriginSource = source;
    if(this->SliceOriginSource)
      {
      this->ObserverId =
          this->SliceOriginSource->AddObserver(
            vtkCommand::AnyEvent, this->Owner,
            &vtkPVQuadRenderView::WidgetCallback);
      }
  }

  void UpdateSliceOrigin(double* origin)
  {
    std::stringstream x,y,z;
    x << origin[0];
    y << origin[1];
    z << origin[2];
    this->CoordinatesX->SetInput(x.str().c_str());
    this->CoordinatesY->SetInput(y.str().c_str());
    this->CoordinatesZ->SetInput(z.str().c_str());
  }

private:
  vtkPVQuadRenderView* Owner;
  unsigned long ObserverId;
  vtkWeakPointer<vtkPointSource> SliceOriginSource;
  vtkNew<vtkTextActor> CoordinatesX;
  vtkNew<vtkTextActor> CoordinatesY;
  vtkNew<vtkTextActor> CoordinatesZ;
};

//============================================================================
vtkStandardNewMacro(vtkPVQuadRenderView);
//----------------------------------------------------------------------------
vtkPVQuadRenderView::vtkPVQuadRenderView()
{
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView = vtkSmartPointer<vtkPVRenderView>::New();
    this->OrthoViews[cc].RenderView->GetRenderer()->GetActiveCamera()->ParallelProjectionOn();
    this->OrthoViews[cc].RenderView->SetInteractionMode(INTERACTION_MODE_2D);
    this->OrthoViews[cc].RenderView->SetCenterAxesVisibility(false);
    }

  // Create a single slice for each axis by default
  for(int i=0; i < 3; ++i)
    {
    this->SetNumberOfSlice(i, 1);
    this->SetSlice(i, 0, 0);
    }

  this->QuadInternal = new vtkQuadInternal(this);
}

//----------------------------------------------------------------------------
vtkPVQuadRenderView::~vtkPVQuadRenderView()
{
  delete this->QuadInternal;
  this->QuadInternal = NULL;
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::Initialize(unsigned int id)
{
  if (this->Identifier == id)
    {
    // already initialized
    return;
    }
  this->Superclass::Initialize(id);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->Initialize(id+cc+1);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetPosition(int posx, int posy)
{
  if (this->Identifier == 0)
    {
    this->Superclass::SetPosition(posx, posy);
    return;
    }

  const int spacing = 2;

  int size[2];
  this->OrthoViews[TOP_LEFT].RenderView->GetSize(size);

  this->OrthoViews[TOP_LEFT].RenderView->SetPosition(posx, posy);
  this->OrthoViews[BOTTOM_LEFT].RenderView->SetPosition(posx, posy + size[1] + spacing);
  this->OrthoViews[TOP_RIGHT].RenderView->SetPosition(posx + size[0] + spacing, posy);
  this->Superclass::SetPosition(posx + size[0] + spacing, posy + size[1] + spacing);
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVQuadRenderView::GetOrthoViewWindow(
  vtkPVQuadRenderView::ViewTypes type)
{
  return this->OrthoViews[type].RenderView->GetRenderWindow();
}

//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVQuadRenderView::GetOrthoRenderView(ViewTypes type)
{
  return this->OrthoViews[type].RenderView;
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetOrthoSize(ViewTypes type, int x, int y)
{
  this->OrthoViews[type].RenderView->SetSize(x, y);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::Render(bool interactive, bool skip_rendering)
{
  this->Superclass::Render(interactive, skip_rendering);

  for (int cc=0; (cc < 3) && !skip_rendering && !this->GetMakingSelection(); cc++)
    {
    if (interactive)
      {
      this->OrthoViews[cc].RenderView->InteractiveRender();
      }
    else
      {
      this->OrthoViews[cc].RenderView->StillRender();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::ResetCamera()
{
  this->Superclass::ResetCamera();
  double bounds[6];
  this->GeometryBounds.GetBounds(bounds);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->ResetCamera(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::ResetCamera(double bounds[6])
{
  this->Superclass::ResetCamera(bounds);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->ResetCamera(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewNormalTopLeft(double x, double y, double z)
{
  vtkPVRenderView* view = this->GetOrthoRenderView(TOP_LEFT);
  vtkCamera* camera = view->GetActiveCamera();
  double newCameraPosition[3];
  camera->GetFocalPoint(newCameraPosition);
  newCameraPosition[0] += x;
  newCameraPosition[1] += y;
  newCameraPosition[2] += z;
  camera->SetPosition(newCameraPosition);

  // Make sure the viewUp is not // at view normal
  double* viewUp = camera->GetViewUp();
  double* normal = camera->GetViewPlaneNormal();
  if ( fabs(vtkMath::Dot(viewUp,normal)) > 0.999 )
    {
    // Need to change viewUp
    camera->SetViewUp(-viewUp[2], viewUp[0], viewUp[1]);
    }

  // Update slice X
  this->SetSliceXNormal(x,y,z);

  this->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewNormalTopRight(double x, double y, double z)
{
  vtkPVRenderView* view = this->GetOrthoRenderView(TOP_RIGHT);
  vtkCamera* camera = view->GetActiveCamera();
  double newCameraPosition[3] = {0,0,0};
  camera->GetFocalPoint(newCameraPosition);
  newCameraPosition[0] += x;
  newCameraPosition[1] += y;
  newCameraPosition[2] += z;
  camera->SetPosition(newCameraPosition);

  // Make sure the viewUp is not // at view normal
  double* viewUp = camera->GetViewUp();
  double* normal = camera->GetViewPlaneNormal();
  if ( fabs(vtkMath::Dot(viewUp,normal)) > 0.999 )
    {
    // Need to change viewUp
    camera->SetViewUp(-viewUp[2], viewUp[0], viewUp[1]);
    }

  // Update slice Y
  this->SetSliceYNormal(x,y,z);

  this->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewNormalBottomLeft(double x, double y, double z)
{
  vtkPVRenderView* view = this->GetOrthoRenderView(BOTTOM_LEFT);
  vtkCamera* camera = view->GetActiveCamera();
  double newCameraPosition[3] = {0,0,0};
  camera->GetFocalPoint(newCameraPosition);
  newCameraPosition[0] += x;
  newCameraPosition[1] += y;
  newCameraPosition[2] += z;
  camera->SetPosition(newCameraPosition);

  // Make sure the viewUp is not // at view normal
  double* viewUp = camera->GetViewUp();
  double* normal = camera->GetViewPlaneNormal();
  if ( fabs(vtkMath::Dot(viewUp,normal)) > 0.999 )
    {
    // Need to change viewUp
    camera->SetViewUp(-viewUp[2], viewUp[0], viewUp[1]);
    }

  // Update slice Z
  this->SetSliceZNormal(x,y,z);

  this->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewUpTopLeft(double x, double y, double z)
{
  this->GetOrthoRenderView(TOP_LEFT)->GetActiveCamera()->SetViewUp(x,y,z);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewUpTopRight(double x, double y, double z)
{
  this->GetOrthoRenderView(TOP_RIGHT)->GetActiveCamera()->SetViewUp(x,y,z);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetViewUpBottomLeft(double x, double y, double z)
{
  this->GetOrthoRenderView(BOTTOM_LEFT)->GetActiveCamera()->SetViewUp(x,y,z);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::Add2DManipulator(vtkCameraManipulator* val)
{
  this->Superclass::Add2DManipulator(val);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->Add2DManipulator(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::RemoveAll2DManipulators()
{
  this->Superclass::RemoveAll2DManipulators();
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->RemoveAll2DManipulators();
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::Add3DManipulator(vtkCameraManipulator* val)
{
  this->Superclass::Add3DManipulator(val);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->Add3DManipulator(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::RemoveAll3DManipulators()
{
  this->Superclass::RemoveAll3DManipulators();
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->RemoveAll3DManipulators();
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetBackground(double r, double g, double b)
{
  this->Superclass::SetBackground(r,g,b);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetBackground(r,g,b);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetBackground2(double r, double g, double b)
{
  this->Superclass::SetBackground2(r,g,b);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetBackground2(r,g,b);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetBackgroundTexture(vtkTexture* val)
{
  this->Superclass::SetBackgroundTexture(val);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetBackgroundTexture(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetGradientBackground(int val)
{
  this->Superclass::SetGradientBackground(val);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetGradientBackground(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetTexturedBackground(int val)
{
  this->Superclass::SetTexturedBackground(val);
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView->SetTexturedBackground(val);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::AddRepresentationToTopLeft(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(TOP_LEFT)->AddRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::AddRepresentationToTopRight(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(TOP_RIGHT)->AddRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::AddRepresentationToBottomLeft(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(BOTTOM_LEFT)->AddRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::RemoveRepresentationToTopLeft(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(TOP_LEFT)->RemoveRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::RemoveRepresentationToTopRight(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(TOP_RIGHT)->RemoveRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::RemoveRepresentationToBottomLeft(vtkDataRepresentation* rep)
{
  this->GetOrthoRenderView(BOTTOM_LEFT)->RemoveRepresentation(rep);
}
//----------------------------------------------------------------------------
void vtkPVQuadRenderView::SetSliceOriginSource(vtkPointSource* source)
{
  this->QuadInternal->SetSliceOriginSource(source);
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::WidgetCallback(vtkObject* src, unsigned long, void*)
{
  vtkPointSource* source = vtkPointSource::SafeDownCast(src);
  if(source)
    {
    double* center = source->GetCenter();
    for(int i=0;i<3;++i)
      {
      this->SetSliceOrigin(i, center[0], center[1], center[2]);
      }
    this->QuadInternal->UpdateSliceOrigin(center);
    }
}

//----------------------------------------------------------------------------
void vtkPVQuadRenderView::Update()
{
  this->Superclass::Update();
  for(int i=0; i < 3; ++i)
    {
    this->GetOrthoRenderView(i)->CopyViewUpdateOptions(this);
    }
}
