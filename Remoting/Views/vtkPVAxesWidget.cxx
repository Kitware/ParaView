/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAxesWidget.h"

#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkObjectFactory.h"
#include "vtkPVAxesActor.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <cmath>

vtkStandardNewMacro(vtkPVAxesWidget);

vtkCxxSetObjectMacro(vtkPVAxesWidget, AxesActor, vtkPVAxesActor);
//----------------------------------------------------------------------------
vtkPVAxesWidget::vtkPVAxesWidget()
{
  this->StartEventObserverId = 0;

  this->EventCallbackCommand->SetCallback(vtkPVAxesWidget::ProcessEvents);

  this->Renderer = vtkRenderer::New();
  this->Renderer->SetViewport(0.0, 0.0, 0.2, 0.2);

  // Since Layer==1, the renderer is treated as transparent and
  // vtkOpenGLRenderer::Clear() won't clear the color-buffer.
  this->Renderer->SetLayer(vtkPVAxesWidget::RendererLayer);
  // Leaving Erase==1 ensures that the depth buffer is cleared. This ensures
  // that the orientation widget always stays on top of the rendered scene.
  this->Renderer->EraseOn();

  this->Renderer->InteractiveOff();
  this->Priority = 0.55;
  this->AxesActor = vtkPVAxesActor::New();
  this->AxesActor->SetVisibility(1);
  this->Renderer->AddActor(this->AxesActor);

  this->ParentRenderer = nullptr;

  this->Moving = 0;
  this->MouseCursorState = vtkPVAxesWidget::Outside;

  this->Outline = vtkPolyData::New();
  this->Outline->Allocate();
  vtkPoints* points = vtkPoints::New();
  vtkIdType ptIds[5];
  ptIds[4] = ptIds[0] = points->InsertNextPoint(1, 1, 0);
  ptIds[1] = points->InsertNextPoint(2, 1, 0);
  ptIds[2] = points->InsertNextPoint(2, 2, 0);
  ptIds[3] = points->InsertNextPoint(1, 2, 0);
  this->Outline->SetPoints(points);
  this->Outline->InsertNextCell(VTK_POLY_LINE, 5, ptIds);
  vtkCoordinate* tcoord = vtkCoordinate::New();
  tcoord->SetCoordinateSystemToDisplay();
  vtkPolyDataMapper2D* mapper = vtkPolyDataMapper2D::New();
  mapper->SetInputData(this->Outline);
  mapper->SetTransformCoordinate(tcoord);
  this->OutlineActor = vtkActor2D::New();
  this->OutlineActor->SetMapper(mapper);
  this->OutlineActor->SetPosition(0, 0);
  this->OutlineActor->SetPosition2(1, 1);
  this->Renderer->AddActor(this->OutlineActor);
  this->OutlineActor->SetVisibility(0);

  points->Delete();
  mapper->Delete();
  tcoord->Delete();
}

//----------------------------------------------------------------------------
vtkPVAxesWidget::~vtkPVAxesWidget()
{
  this->SetInteractor(nullptr);
  this->SetParentRenderer(nullptr);

  this->AxesActor->Delete();
  this->OutlineActor->Delete();
  this->Outline->Delete();
  this->Renderer->Delete();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SetInteractor(vtkRenderWindowInteractor* iren)
{
  if (iren == this->GetInteractor())
  {
    return;
  }

  if (vtkRenderWindowInteractor* current = this->GetInteractor())
  {
    current->RemoveObserver(this->EventCallbackCommand);
  }
  this->Superclass::SetInteractor(iren);
  if (iren)
  {
    iren->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);
    iren->AddObserver(vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);
    iren->AddObserver(
      vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
  }
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SetParentRenderer(vtkRenderer* ren)
{
  if (this->ParentRenderer == ren)
  {
    return;
  }

  if (this->ParentRenderer)
  {
    if (this->ParentRenderer->GetRenderWindow())
    {
      this->ParentRenderer->GetRenderWindow()->RemoveRenderer(this->Renderer);
    }
    this->ParentRenderer->RemoveObserver(this->StartEventObserverId);
  }

  vtkSetObjectBodyMacro(ParentRenderer, vtkRenderer, ren);

  if (this->ParentRenderer)
  {
    if (this->ParentRenderer->GetRenderWindow())
    {
      this->ParentRenderer->GetRenderWindow()->AddRenderer(this->Renderer);
    }
    this->StartEventObserverId = this->ParentRenderer->AddObserver(
      vtkCommand::StartEvent, this, &vtkPVAxesWidget::UpdateCameraFromParentRenderer);
  }

  this->UpdateCameraFromParentRenderer();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::UpdateCameraFromParentRenderer()
{
  if (!this->ParentRenderer)
  {
    return;
  }

  this->SquareRenderer();

  vtkCamera* cam = this->ParentRenderer->GetActiveCamera();
  double pos[3], fp[3], viewup[3];
  cam->GetPosition(pos);
  cam->GetFocalPoint(fp);
  cam->GetViewUp(viewup);

  cam = this->Renderer->GetActiveCamera();
  cam->SetPosition(pos);
  cam->SetFocalPoint(fp);
  cam->SetViewUp(viewup);
  this->Renderer->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::UpdateCursorIcon()
{
  if (!this->Enabled)
  {
    this->SetMouseCursor(vtkPVAxesWidget::Outside);
    return;
  }

  if (this->Moving)
  {
    return;
  }

  int* parentSize = this->ParentRenderer->GetSize();

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  double xNorm = x / (double)parentSize[0];
  double yNorm = y / (double)parentSize[1];

  double pos[4];
  this->Renderer->GetViewport(pos);

  int pState = this->MouseCursorState;

  if (xNorm > pos[0] && xNorm < pos[2] && yNorm > pos[1] && yNorm < pos[3])
  {
    this->MouseCursorState = vtkPVAxesWidget::Inside;
  }
  else if (fabs(xNorm - pos[0]) < .02 && fabs(yNorm - pos[3]) < .02)
  {
    this->MouseCursorState = vtkPVAxesWidget::TopLeft;
  }
  else if (fabs(xNorm - pos[2]) < .02 && fabs(yNorm - pos[3]) < .02)
  {
    this->MouseCursorState = vtkPVAxesWidget::TopRight;
  }
  else if (fabs(xNorm - pos[0]) < .02 && fabs(yNorm - pos[1]) < .02)
  {
    this->MouseCursorState = vtkPVAxesWidget::BottomLeft;
  }
  else if (fabs(xNorm - pos[2]) < .02 && fabs(yNorm - pos[1]) < .02)
  {
    this->MouseCursorState = vtkPVAxesWidget::BottomRight;
  }
  else
  {
    this->MouseCursorState = vtkPVAxesWidget::Outside;
  }

  if (pState == this->MouseCursorState)
  {
    return;
  }

  if (this->MouseCursorState == vtkPVAxesWidget::Outside)
  {
    this->OutlineActor->SetVisibility(0);
  }
  else
  {
    this->OutlineActor->SetVisibility(1);
  }
  this->Interactor->Render();

  this->SetMouseCursor(this->MouseCursorState);
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SetMouseCursor(int cursorState)
{
  if (!this->Interactor)
  {
    return;
  }
  switch (cursorState)
  {
    case vtkPVAxesWidget::Outside:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
      break;
    case vtkPVAxesWidget::Inside:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZEALL);
      break;
    case vtkPVAxesWidget::TopLeft:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZENW);
      break;
    case vtkPVAxesWidget::TopRight:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZENE);
      break;
    case vtkPVAxesWidget::BottomLeft:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZESW);
      break;
    case vtkPVAxesWidget::BottomRight:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZESE);
      break;
  }
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::ProcessEvents(
  vtkObject* vtkNotUsed(object), unsigned long event, void* clientdata, void* vtkNotUsed(calldata))
{
  vtkPVAxesWidget* self = reinterpret_cast<vtkPVAxesWidget*>(clientdata);

  if (!self->GetEnabled() || !self->GetVisibility())
  {
    return;
  }

  switch (event)
  {
    case vtkCommand::LeftButtonPressEvent:
      self->OnButtonPress();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnButtonRelease();
      break;
  }
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::OnButtonPress()
{
  if (this->MouseCursorState == vtkPVAxesWidget::Outside)
  {
    return;
  }

  this->SetMouseCursor(this->MouseCursorState);

  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];

  this->Moving = 1;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::OnButtonRelease()
{
  if (this->MouseCursorState == vtkPVAxesWidget::Outside)
  {
    return;
  }

  this->Moving = 0;
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent, nullptr);
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::OnMouseMove()
{
  if (this->Moving)
  {
    switch (this->MouseCursorState)
    {
      case vtkPVAxesWidget::Inside:
        this->MoveWidget();
        break;
      case vtkPVAxesWidget::TopLeft:
        this->ResizeTopLeft();
        break;
      case vtkPVAxesWidget::TopRight:
        this->ResizeTopRight();
        break;
      case vtkPVAxesWidget::BottomLeft:
        this->ResizeBottomLeft();
        break;
      case vtkPVAxesWidget::BottomRight:
        this->ResizeBottomRight();
        break;
    }

    this->UpdateCursorIcon();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
  }
  else
  {
    this->UpdateCursorIcon();
  }
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::MoveWidget()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];

  this->StartPosition[0] = x;
  this->StartPosition[1] = y;

  int* size = this->ParentRenderer->GetSize();
  double dxNorm = dx / (double)size[0];
  double dyNorm = dy / (double)size[1];

  double* vp = this->Renderer->GetViewport();

  double newPos[4];
  newPos[0] = vp[0] + dxNorm;
  newPos[1] = vp[1] + dyNorm;
  newPos[2] = vp[2] + dxNorm;
  newPos[3] = vp[3] + dyNorm;

  if (newPos[0] < 0)
  {
    this->StartPosition[0] = 0;
    newPos[0] = 0;
    newPos[2] = vp[2] - vp[0];
  }
  if (newPos[1] < 0)
  {
    this->StartPosition[1] = 0;
    newPos[1] = 0;
    newPos[3] = vp[3] - vp[1];
  }
  if (newPos[2] > 1)
  {
    this->StartPosition[0] = (int)(size[0] - size[0] * (vp[2] - vp[0]));
    newPos[0] = 1 - (vp[2] - vp[0]);
    newPos[2] = 1;
  }
  if (newPos[3] > 1)
  {
    this->StartPosition[1] = (int)(size[1] - size[1] * (vp[3] - vp[1]));
    newPos[1] = 1 - (vp[3] - vp[1]);
    newPos[3] = 1;
  }

  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::ResizeTopLeft()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];

  int* size = this->ParentRenderer->GetSize();
  double dxNorm = dx / (double)size[0];
  double dyNorm = dy / (double)size[1];

  int useX;
  double change;
  double absDx = fabs(dxNorm);
  double absDy = fabs(dyNorm);

  if (absDx > absDy)
  {
    change = dxNorm;
    useX = 1;
  }
  else
  {
    change = dyNorm;
    useX = 0;
  }

  double* vp = this->Renderer->GetViewport();

  this->StartPosition[0] = x;
  this->StartPosition[1] = y;

  double newPos[4];
  newPos[0] = useX ? vp[0] + change : vp[0] - change;
  newPos[1] = vp[1];
  newPos[2] = vp[2];
  newPos[3] = useX ? vp[3] - change : vp[3] + change;

  if (newPos[0] < 0)
  {
    this->StartPosition[0] = 0;
    newPos[0] = 0;
  }
  if (newPos[0] >= newPos[2] - 0.01)
  {
    newPos[0] = newPos[2] - 0.01;
  }
  if (newPos[3] > 1)
  {
    this->StartPosition[1] = size[1];
    newPos[3] = 1;
  }
  if (newPos[3] <= newPos[1] + 0.01)
  {
    newPos[3] = newPos[1] + 0.01;
  }

  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::ResizeTopRight()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];

  int* size = this->ParentRenderer->GetSize();
  double dxNorm = dx / (double)size[0];
  double dyNorm = dy / (double)size[1];

  double change;
  double absDx = fabs(dxNorm);
  double absDy = fabs(dyNorm);

  if (absDx > absDy)
  {
    change = dxNorm;
  }
  else
  {
    change = dyNorm;
  }

  double* vp = this->Renderer->GetViewport();

  this->StartPosition[0] = x;
  this->StartPosition[1] = y;

  double newPos[4];
  newPos[0] = vp[0];
  newPos[1] = vp[1];
  newPos[2] = vp[2] + change;
  newPos[3] = vp[3] + change;

  if (newPos[2] > 1)
  {
    this->StartPosition[0] = size[0];
    newPos[2] = 1;
  }
  if (newPos[2] <= newPos[0] + 0.01)
  {
    newPos[2] = newPos[0] + 0.01;
  }
  if (newPos[3] > 1)
  {
    this->StartPosition[1] = size[1];
    newPos[3] = 1;
  }
  if (newPos[3] <= newPos[1] + 0.01)
  {
    newPos[3] = newPos[1] + 0.01;
  }

  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::ResizeBottomLeft()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];

  int* size = this->ParentRenderer->GetSize();
  double dxNorm = dx / (double)size[0];
  double dyNorm = dy / (double)size[1];
  double* vp = this->Renderer->GetViewport();

  double change;
  double absDx = fabs(dxNorm);
  double absDy = fabs(dyNorm);

  if (absDx > absDy)
  {
    change = dxNorm;
  }
  else
  {
    change = dyNorm;
  }

  this->StartPosition[0] = x;
  this->StartPosition[1] = y;

  double newPos[4];
  newPos[0] = vp[0] + change;
  newPos[1] = vp[1] + change;
  newPos[2] = vp[2];
  newPos[3] = vp[3];

  if (newPos[0] < 0)
  {
    this->StartPosition[0] = 0;
    newPos[0] = 0;
  }
  if (newPos[0] >= newPos[2] - 0.01)
  {
    newPos[0] = newPos[2] - 0.01;
  }
  if (newPos[1] < 0)
  {
    this->StartPosition[1] = 0;
    newPos[1] = 0;
  }
  if (newPos[1] >= newPos[3] - 0.01)
  {
    newPos[1] = newPos[3] - 0.01;
  }

  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::ResizeBottomRight()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];

  int* size = this->ParentRenderer->GetSize();
  double dxNorm = dx / (double)size[0];
  double dyNorm = dy / (double)size[1];

  double* vp = this->Renderer->GetViewport();

  int useX;
  double change;
  double absDx = fabs(dxNorm);
  double absDy = fabs(dyNorm);

  if (absDx > absDy)
  {
    change = dxNorm;
    useX = 1;
  }
  else
  {
    change = dyNorm;
    useX = 0;
  }

  this->StartPosition[0] = x;
  this->StartPosition[1] = y;

  double newPos[4];
  newPos[0] = vp[0];
  newPos[1] = useX ? vp[1] - change : vp[1] + change;
  newPos[2] = useX ? vp[2] + change : vp[2] - change;
  newPos[3] = vp[3];

  if (newPos[2] > 1)
  {
    this->StartPosition[0] = size[0];
    newPos[2] = 1;
  }
  if (newPos[2] <= newPos[0] + 0.01)
  {
    newPos[2] = newPos[0] + 0.01;
  }
  if (newPos[1] < 0)
  {
    this->StartPosition[1] = 0;
    newPos[1] = 0;
  }
  if (newPos[1] >= newPos[3] - 0.01)
  {
    newPos[1] = newPos[3] - 0.01;
  }

  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SquareRenderer()
{
  int* size = this->Renderer->GetSize();
  if (size[0] == 0 || size[1] == 0)
  {
    return;
  }

  double vp[4];
  this->Renderer->GetViewport(vp);

  double deltaX = vp[2] - vp[0];
  double newDeltaX = size[1] * deltaX / (double)size[0];
  double deltaY = vp[3] - vp[1];
  double newDeltaY = size[0] * deltaY / (double)size[1];

  if (newDeltaX > 1)
  {
    if (newDeltaY > 1)
    {
      if (size[0] > size[1])
      {
        newDeltaX = size[1] / (double)size[0];
        newDeltaY = 1;
      }
      else
      {
        newDeltaX = 1;
        newDeltaY = size[0] / (double)size[1];
      }
      vp[0] = vp[1] = 0;
      vp[2] = newDeltaX;
      vp[3] = newDeltaY;
    }
    else
    {
      vp[3] = vp[1] + newDeltaY;
      if (vp[3] > 1)
      {
        vp[3] = 1;
        vp[1] = vp[3] - newDeltaY;
      }
    }
  }
  else
  {
    vp[2] = vp[0] + newDeltaX;
    if (vp[2] > 1)
    {
      vp[2] = 1;
      vp[0] = vp[2] - newDeltaX;
    }
  }

  this->Renderer->SetViewport(vp);

  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  vtkPoints* points = this->Outline->GetPoints();
  points->SetPoint(0, vp[0] + 1, vp[1] + 1, 0);
  points->SetPoint(1, vp[2] - 1, vp[1] + 1, 0);
  points->SetPoint(2, vp[2] - 1, vp[3] - 1, 0);
  points->SetPoint(3, vp[0] + 1, vp[3] - 1, 0);
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SetEnabled(int state)
{
  if (this->Enabled != state)
  {
    this->Enabled = state;
  }
  this->Superclass::SetEnabled(state); // funny, superclass doesn't do anything, but hey.
  if (!state)
  {
    this->OnButtonRelease();
    this->UpdateCursorIcon();
    this->OutlineActor->SetVisibility(0);
  }
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SetVisibility(bool val)
{
  this->AxesActor->SetVisibility(val);
  if (this->Enabled && !val)
  {
    // ensures that the outline actor's visibility is updated correctly
    this->SetEnabled(false);
    this->SetEnabled(true);
  }
}

//----------------------------------------------------------------------------
bool vtkPVAxesWidget::GetVisibility()
{
  return (this->AxesActor->GetVisibility() != 0);
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SetOutlineColor(double r, double g, double b)
{
  this->OutlineActor->GetProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
double* vtkPVAxesWidget::GetOutlineColor()
{
  return this->OutlineActor->GetProperty()->GetColor();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SetAxisLabelColor(double r, double g, double b)
{
  this->AxesActor->GetXAxisLabelProperty()->SetColor(r, g, b);
  this->AxesActor->GetYAxisLabelProperty()->SetColor(r, g, b);
  this->AxesActor->GetZAxisLabelProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
double* vtkPVAxesWidget::GetAxisLabelColor()
{
  return this->AxesActor->GetXAxisLabelProperty()->GetColor();
}

//----------------------------------------------------------------------------
vtkRenderer* vtkPVAxesWidget::GetParentRenderer()
{
  return this->ParentRenderer;
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::SetViewport(double minX, double minY, double maxX, double maxY)
{
  this->Renderer->SetViewport(minX, minY, maxX, maxY);
}

//----------------------------------------------------------------------------
double* vtkPVAxesWidget::GetViewport()
{
  return this->Renderer->GetViewport();
}

//----------------------------------------------------------------------------
void vtkPVAxesWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AxesActor: " << this->AxesActor << endl;
}
