/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAxesWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVAxesWidget.h"

#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkPVAxesActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

vtkStandardNewMacro(vtkPVAxesWidget);
vtkCxxRevisionMacro(vtkPVAxesWidget, "1.1.2.3");

vtkCxxSetObjectMacro(vtkPVAxesWidget, AxesActor, vtkPVAxesActor);
vtkCxxSetObjectMacro(vtkPVAxesWidget, ParentRenderer, vtkRenderer);

class vtkPVAxesWidgetObserver : public vtkCommand
{
public:
  static vtkPVAxesWidgetObserver *New()
    {return new vtkPVAxesWidgetObserver;};
  
  vtkPVAxesWidgetObserver()
    {
      this->AxesWidget = 0;
    }
  
  virtual void Execute(vtkObject* wdg, unsigned long event, void *calldata)
    {
      if (this->AxesWidget)
        {
        this->AxesWidget->ExecuteEvent(wdg, event, calldata);
        }
    }
  
  vtkPVAxesWidget *AxesWidget;
};

vtkPVAxesWidget::vtkPVAxesWidget()
{
  this->EventCallbackCommand->SetCallback(vtkPVAxesWidget::ProcessEvents);
  
  this->Observer = vtkPVAxesWidgetObserver::New();
  this->Observer->AxesWidget = this;
  this->Renderer = vtkRenderer::New();
  this->Renderer->SetViewport(0.0, 0.0, 0.2, 0.2);
  this->Renderer->SetLayer(0);
  this->Renderer->InteractiveOff();
  this->Priority = 0.55;
  this->AxesActor = vtkPVAxesActor::New();
  this->Renderer->AddActor(this->AxesActor);
  
  this->ParentRenderer = NULL;
  
  this->Moving = 0;
  this->MouseCursorState = vtkPVAxesWidget::Outside;

  this->StartTag = 0;
  
  this->Interactive = 1;
  
  this->Outline = vtkPolyData::New();
  this->Outline->Allocate();
  vtkPoints *points = vtkPoints::New();
  vtkIdType ptIds[5];
  ptIds[4] = ptIds[0] = points->InsertNextPoint(1, 1, 0);
  ptIds[1] = points->InsertNextPoint(2, 1, 0);
  ptIds[2] = points->InsertNextPoint(2, 2, 0);
  ptIds[3] = points->InsertNextPoint(1, 2, 0);
  this->Outline->SetPoints(points);
  this->Outline->InsertNextCell(VTK_POLY_LINE, 5, ptIds);
  vtkCoordinate *tcoord = vtkCoordinate::New();
  tcoord->SetCoordinateSystemToDisplay();
  vtkPolyDataMapper2D *mapper = vtkPolyDataMapper2D::New();
  mapper->SetInput(this->Outline);
  mapper->SetTransformCoordinate(tcoord);
  this->OutlineActor = vtkActor2D::New();
  this->OutlineActor->SetMapper(mapper);
  this->OutlineActor->SetPosition(0, 0);
  this->OutlineActor->SetPosition2(1, 1);
  
  points->Delete();
  mapper->Delete();
  tcoord->Delete();
}

vtkPVAxesWidget::~vtkPVAxesWidget()
{
  this->Observer->Delete();
  this->SetParentRenderer(NULL);
  this->Renderer->Delete();
  this->AxesActor->Delete();
  this->OutlineActor->Delete();
  this->Outline->Delete();
}

void vtkPVAxesWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
    {
    vtkErrorMacro("The interactor must be set prior to enabling/disabling widget");
    }
  
  if (enabling)
    {
    if (this->Enabled)
      {
      return;
      }
    if (!this->ParentRenderer)
      {
      vtkErrorMacro("The parent renderer must be set prior to enabling this widget");
      return;
      }

    this->Enabled = 1;
    
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::MouseMoveEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent,
                   this->EventCallbackCommand, this->Priority);
    
    this->ParentRenderer->GetRenderWindow()->AddRenderer(this->Renderer);
    this->ParentRenderer->SetLayer(1);
    this->ParentRenderer->GetRenderWindow()->SetNumberOfLayers(2);
    this->AxesActor->SetVisibility(1);
    this->ParentRenderer->GetRenderWindow()->Render();
    this->ParentRenderer->AddObserver(vtkCommand::StartEvent, this->Observer);
    
    this->InvokeEvent(vtkCommand::EnableEvent, NULL);
    }
  else
    {
    if (!this->Enabled)
      {
      return;
      }
    
    this->Enabled = 0;
    this->Interactor->RemoveObserver(this->EventCallbackCommand);
    
    this->AxesActor->SetVisibility(0);
    if (this->ParentRenderer)
      {
      this->ParentRenderer->GetRenderWindow()->RemoveRenderer(this->Renderer);
      this->ParentRenderer->GetRenderWindow()->Render();
      this->ParentRenderer->RemoveObserver(vtkCommand::StartEvent);
      }
    
    this->InvokeEvent(vtkCommand::DisableEvent, NULL);
    }
}

void vtkPVAxesWidget::ExecuteEvent(vtkObject *vtkNotUsed(o),
                                   unsigned long vtkNotUsed(event),
                                   void *vtkNotUsed(calldata))
{
  if (!this->ParentRenderer)
    {
    return;
    }
  
  vtkCamera *cam = this->ParentRenderer->GetActiveCamera();
  float pos[3], fp[3], viewup[3];
  cam->GetPosition(pos);
  cam->GetFocalPoint(fp);
  cam->GetViewUp(viewup);
  
  cam = this->Renderer->GetActiveCamera();
  cam->SetPosition(pos);
  cam->SetFocalPoint(fp);
  cam->SetViewUp(viewup);
  this->Renderer->ResetCamera();
  
  this->SquareRenderer();
}

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
  
  int *parentSize = this->ParentRenderer->GetSize();
  
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  float xNorm = x / (float)parentSize[0];
  float yNorm = y / (float)parentSize[1];
  
  float pos[4];
  this->Renderer->GetViewport(pos);
  
  int pState = this->MouseCursorState;
  
  if (xNorm > pos[0] && xNorm < pos[2] && yNorm > pos[1] && yNorm < pos[3])
    {
    this->MouseCursorState = vtkPVAxesWidget::Inside;
    }
  else if (fabs(xNorm-pos[0]) < .02 && fabs(yNorm-pos[3]) < .02)
    {
    this->MouseCursorState = vtkPVAxesWidget::TopLeft;
    }
  else if (fabs(xNorm-pos[2]) < .02 && fabs(yNorm-pos[3]) < .02)
    {
    this->MouseCursorState = vtkPVAxesWidget::TopRight;
    }
  else if (fabs(xNorm-pos[0]) < .02 && fabs(yNorm-pos[1]) < .02)
    {
    this->MouseCursorState = vtkPVAxesWidget::BottomLeft;
    }
  else if (fabs(xNorm-pos[2]) < .02 && fabs(yNorm-pos[1]) < .02)
    {
    this->MouseCursorState = vtkPVAxesWidget::BottomRight;
    }
  else
    {
    this->MouseCursorState = vtkPVAxesWidget::Outside;
    }

  if (this->MouseCursorState == vtkPVAxesWidget::Outside)
    {
    this->Renderer->RemoveActor(this->OutlineActor);
    }
  else
    {
    this->Renderer->AddActor(this->OutlineActor);
    }
  this->Interactor->Render();
  
  if (pState == this->MouseCursorState)
    {
    return;
    }
  
  this->SetMouseCursor(this->MouseCursorState);
}

void vtkPVAxesWidget::SetMouseCursor(int cursorState)
{
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

void vtkPVAxesWidget::ProcessEvents(vtkObject* vtkNotUsed(object),
                                    unsigned long event,
                                    void *clientdata,
                                    void* vtkNotUsed(calldata))
{
  vtkPVAxesWidget *self =
    reinterpret_cast<vtkPVAxesWidget*>(clientdata);

  if (!self->GetInteractive())
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
  this->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
}

void vtkPVAxesWidget::OnButtonRelease()
{
  if (this->MouseCursorState == vtkPVAxesWidget::Outside)
    {
    return;
    }
  
  this->Moving = 0;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent, NULL);
}

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
    this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    }
  else
    {
    this->UpdateCursorIcon();
    }
}

void vtkPVAxesWidget::MoveWidget()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  
  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];

  this->StartPosition[0] = x;
  this->StartPosition[1] = y;

  int *size = this->ParentRenderer->GetSize();
  float dxNorm = dx / (float)size[0];
  float dyNorm = dy / (float)size[1];
  
  float *vp = this->Renderer->GetViewport();
  
  float newPos[4];
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
    this->StartPosition[0] = (int)(size[0] - size[0] * (vp[2]-vp[0]));
    newPos[0] = 1 - (vp[2]-vp[0]);
    newPos[2] = 1;
    }
  if (newPos[3] > 1)
    {
    this->StartPosition[1] = (int)(size[1] - size[1]*(vp[3]-vp[1]));
    newPos[1] = 1 - (vp[3]-vp[1]);
    newPos[3] = 1;
    }

  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

void vtkPVAxesWidget::ResizeTopLeft()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  
  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];
  
  int *size = this->ParentRenderer->GetSize();
  float dxNorm = dx / (float)size[0];
  float dyNorm = dy / (float)size[1];
  
  int useX;
  float change;
  float absDx = fabs(dxNorm);
  float absDy = fabs(dyNorm);
  
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
  
  float *vp = this->Renderer->GetViewport();
  
  this->StartPosition[0] = x;
  this->StartPosition[1] = y;
  
  float newPos[4];
  newPos[0] = useX ? vp[0] + change : vp[0] - change;
  newPos[1] = vp[1];
  newPos[2] = vp[2];
  newPos[3] = useX ? vp[3] - change : vp[3] + change;
  
  if (newPos[0] < 0)
    {
    this->StartPosition[0] = 0;
    newPos[0] = 0;
    }
  if (newPos[0] >= newPos[2]-0.01)
    {
    newPos[0] = newPos[2] - 0.01;
    }
  if (newPos[3] > 1)
    {
    this->StartPosition[1] = size[1];
    newPos[3] = 1;
    }
  if (newPos[3] <= newPos[1]+0.01)
    {
    newPos[3] = newPos[1] + 0.01;
    }
  
  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

void vtkPVAxesWidget::ResizeTopRight()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  
  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];
  
  int *size = this->ParentRenderer->GetSize();
  float dxNorm = dx / (float)size[0];
  float dyNorm = dy / (float)size[1];

  float change;
  float absDx = fabs(dxNorm);
  float absDy = fabs(dyNorm);
  
  if (absDx > absDy)
    {
    change = dxNorm;
    }
  else
    {
    change = dyNorm;
    }
  
  float *vp = this->Renderer->GetViewport();
  
  this->StartPosition[0] = x;
  this->StartPosition[1] = y;
  
  float newPos[4];
  newPos[0] = vp[0];
  newPos[1] = vp[1];
  newPos[2] = vp[2] + change;
  newPos[3] = vp[3] + change;
  
  if (newPos[2] > 1)
    {
    this->StartPosition[0] = size[0];
    newPos[2] = 1;
    }
  if (newPos[2] <= newPos[0]+0.01)
    {
    newPos[2] = newPos[0] + 0.01;
    }
  if (newPos[3] > 1)
    {
    this->StartPosition[1] = size[1];
    newPos[3] = 1;
    }
  if (newPos[3] <= newPos[1]+0.01)
    {
    newPos[3] = newPos[1] + 0.01;
    }
  
  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

void vtkPVAxesWidget::ResizeBottomLeft()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  
  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];
  
  int *size = this->ParentRenderer->GetSize();
  float dxNorm = dx / (float)size[0];
  float dyNorm = dy / (float)size[1];
  float *vp = this->Renderer->GetViewport();
  
  float change;
  float absDx = fabs(dxNorm);
  float absDy = fabs(dyNorm);
  
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
  
  float newPos[4];
  newPos[0] = vp[0] + change;
  newPos[1] = vp[1] + change;
  newPos[2] = vp[2];
  newPos[3] = vp[3];
  
  if (newPos[0] < 0)
    {
    this->StartPosition[0] = 0;
    newPos[0] = 0;
    }
  if (newPos[0] >= newPos[2]-0.01)
    {
    newPos[0] = newPos[2] - 0.01;
    }
  if (newPos[1] < 0)
    {
    this->StartPosition[1] = 0;
    newPos[1] = 0;
    }
  if (newPos[1] >= newPos[3]-0.01)
    {
    newPos[1] = newPos[3] - 0.01;
    }
  
  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

void vtkPVAxesWidget::ResizeBottomRight()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  
  int dx = x - this->StartPosition[0];
  int dy = y - this->StartPosition[1];
  
  int *size = this->ParentRenderer->GetSize();
  float dxNorm = dx / (float)size[0];
  float dyNorm = dy / (float)size[1];
  
  float *vp = this->Renderer->GetViewport();
  
  int useX;
  float change;
  float absDx = fabs(dxNorm);
  float absDy = fabs(dyNorm);
  
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
  
  float newPos[4];
  newPos[0] = vp[0];
  newPos[1] = useX ? vp[1] - change : vp[1] + change;
  newPos[2] = useX ? vp[2] + change : vp[2] - change;
  newPos[3] = vp[3];
  
  if (newPos[2] > 1)
    {
    this->StartPosition[0] = size[0];
    newPos[2] = 1;
    }
  if (newPos[2] <= newPos[0]+0.01)
    {
    newPos[2] = newPos[0] + 0.01;
    }
  if (newPos[1] < 0)
    {
    this->StartPosition[1] = 0;
    newPos[1] = 0;
    }
  if (newPos[1] >= newPos[3]-0.01)
    {
    newPos[1] = newPos[3]-0.01;
    }
  
  this->Renderer->SetViewport(newPos);
  this->Interactor->Render();
}

void vtkPVAxesWidget::SquareRenderer()
{
  int *size = this->Renderer->GetSize();
  float vp[4];
  this->Renderer->GetViewport(vp);
  
  float deltaX = vp[2] - vp[0];
  float newDeltaX = size[1] * deltaX / (float)size[0];
  vp[2] = vp[0] + newDeltaX;
  
  this->Renderer->SetViewport(vp);
  
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);
  
  vtkPoints *points = this->Outline->GetPoints();
  points->SetPoint(0, vp[0]+1, vp[1]+1, 0);
  points->SetPoint(1, vp[2]-1, vp[1]+1, 0);
  points->SetPoint(2, vp[2]-1, vp[3]-1, 0);
  points->SetPoint(3, vp[0]+1, vp[3]-1, 0);
}

void vtkPVAxesWidget::SetInteractive(int state)
{
  if (this->Interactive != state)
    {
    this->Interactive = state;
    }
  
  if (!state)
    {
    this->OnButtonRelease();
    this->MouseCursorState = vtkPVAxesWidget::Outside;
    this->Renderer->RemoveActor(this->OutlineActor);
    if (this->Interactor)
      {
      this->SetMouseCursor(this->MouseCursorState);
      this->Interactor->Render();
      }
    }
}

void vtkPVAxesWidget::SetOutlineColor(float r, float g, float b)
{
  this->OutlineActor->GetProperty()->SetColor(r, g, b);
  if (this->Interactor)
    {
    this->Interactor->Render();
    }
}

float* vtkPVAxesWidget::GetOutlineColor()
{
  return this->OutlineActor->GetProperty()->GetColor();
}

void vtkPVAxesWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "AxesActor: " << this->AxesActor << endl;
  os << indent << "Interactive: " << this->Interactive << endl;
}
