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

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVQuadRenderView);
//----------------------------------------------------------------------------
vtkPVQuadRenderView::vtkPVQuadRenderView()
{
  for (int cc=0; cc < 3; cc++)
    {
    this->OrthoViews[cc].RenderView = vtkSmartPointer<vtkPVRenderView>::New();
    this->OrthoViews[cc].RenderView->GetRenderer()->GetActiveCamera()->ParallelProjectionOn();
    this->OrthoViews[cc].RenderView->GetRenderer()->SetBackground(
      0.1*cc, 0.2*cc, 0.4*cc);
    this->OrthoViews[cc].RenderView->SetInteractionMode(INTERACTION_MODE_2D);
    this->OrthoViews[cc].RenderView->SetCenterAxesVisibility(true);
    }

  // Create a single slice for each axis by default
  for(int i=0; i < 3; ++i)
    {
    this->SetNumberOfSlice(i, 1);
    this->SetSlice(i, 0, 0);
    }
}

//----------------------------------------------------------------------------
vtkPVQuadRenderView::~vtkPVQuadRenderView()
{
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
