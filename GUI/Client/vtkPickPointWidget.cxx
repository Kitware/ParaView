/*=========================================================================

  Program:   ParaView
  Module:    vtkPickPointWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPickPointWidget.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"

#include "vtkPVRenderModule.h"


vtkStandardNewMacro(vtkPickPointWidget);
vtkCxxRevisionMacro(vtkPickPointWidget, "1.1");
vtkCxxSetObjectMacro(vtkPickPointWidget,RenderModule,vtkPVRenderModule);



//----------------------------------------------------------------------------
vtkPickPointWidget::vtkPickPointWidget()
{
  this->RenderModule = 0;
}

//----------------------------------------------------------------------------
vtkPickPointWidget::~vtkPickPointWidget()
{
  this->SetRenderModule(NULL);
}

//----------------------------------------------------------------------------
void vtkPickPointWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RenderModule: (" << this->RenderModule << ")\n";
}



void vtkPickPointWidget::OnLeftButtonDown()
{
  if (this->Interactor->GetShiftKey() )
    {
    if (this->RenderModule == NULL)
      {
      vtkErrorMacro("Cannot pick without a render module.");
      return;
      }
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    float z = this->RenderModule->GetZBufferValue(X, Y);
    double pt[4];
    this->ComputeDisplayToWorld(double(X),double(Y),double(z),pt);
    this->Cursor3D->SetFocalPoint(pt);
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    return;
    }

  this->Superclass::OnLeftButtonDown();
}

void vtkPickPointWidget::OnRightButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
    {
    this->State = vtkPointWidget::Outside;
    return;
    }
  
  vtkAssemblyPath *path;
  this->CursorPicker->Pick(X,Y,0.0,this->CurrentRenderer);

  path = this->CursorPicker->GetPath();
  if ( path != NULL )
    {
    this->State = vtkPointWidget::Moving;
    this->Highlight(1);
    // Move along cameras view axis.
    // Allthough this is a constraint,
    // this is probably not the best place to mark the state.
    this->ConstraintAxis = -2;
    this->LastY = Y;
    }
  else
    {
    this->State = vtkPointWidget::Outside;
    this->Highlight(0);
    return;
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPickPointWidget::OnRightButtonUp()
{
  if ( this->State == vtkPointWidget::Outside ||
       this->State == vtkPointWidget::Start )
    {
    return;
    }

  this->State = vtkPointWidget::Start;
  this->Highlight(0);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

void vtkPickPointWidget::OnMouseMove()
{
  if (this->ConstraintAxis != -2)
    {
    this->Superclass::OnMouseMove();
    return;
    }

  // See whether we're active
  if ( this->State == vtkPointWidget::Outside || 
       this->State == vtkPointWidget::Start )
    {
    return;
    }
  
  // Compute the fraction fo the viewport that we moved (-1 to 1).
  float factor;
  int* renSize = this->CurrentRenderer->GetSize();
  int Y = this->Interactor->GetEventPosition()[1];
  if (this->LastY == Y)
    {
    return;
    }
  factor = ((float)(Y) - (float)(this->LastY))/(float)(renSize[1]);
  this->LastY = Y;
  // Compute the new point.
  // It might be better to use bounds to compute magnitude of movement.
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }
  double focalPoint[4], pickPoint[4], prevPickPoint[4], pos[3];
  double z;
  camera->GetPosition(pos);
  // Compute the two points defining the motion vector
  this->ComputeWorldToDisplay(this->LastPickPosition[0], this->LastPickPosition[1],
                              this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),double(this->Interactor->GetLastEventPosition()[1]),
                              z, prevPickPoint);
  pickPoint[0] = prevPickPoint[0] + factor*(pos[0]-prevPickPoint[0]);
  pickPoint[1] = prevPickPoint[1] + factor*(pos[1]-prevPickPoint[1]);
  pickPoint[2] = prevPickPoint[2] + factor*(pos[2]-prevPickPoint[2]);

  // Process the motion
  if ( this->State == vtkPointWidget::Moving )
    {
    if ( !this->WaitingForMotion || this->WaitCount++ > 3 )
      {
      this->MoveFocus(prevPickPoint, pickPoint);
      }
    else
      {
      return; //avoid the extra render
      }
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  this->Interactor->Render();
}


