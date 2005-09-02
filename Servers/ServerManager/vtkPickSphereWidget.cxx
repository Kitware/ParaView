/*=========================================================================

  Program:   ParaView
  Module:    vtkPickSphereWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPickSphereWidget.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"

#include "vtkDataSet.h"
#include "vtkTransform.h"

#include "vtkSMRenderModuleProxy.h"


vtkStandardNewMacro(vtkPickSphereWidget);
vtkCxxRevisionMacro(vtkPickSphereWidget, "1.2");


//----------------------------------------------------------------------------
vtkPickSphereWidget::vtkPickSphereWidget()
{
  this->EventCallbackCommand->SetCallback(vtkPickSphereWidget::ProcessEvents);
  this->RenderModuleProxy = 0;
// ATTRIBUTE EDITOR
  this->LastPickPosition[0] = this->LastPickPosition[1] = this->LastPickPosition[2] = 0;
  this->MouseControlToggle = 0;

}

//----------------------------------------------------------------------------
vtkPickSphereWidget::~vtkPickSphereWidget()
{
  this->SetRenderModuleProxy(NULL);
}

void vtkPickSphereWidget::PlaceWidget(double bds[6])
{
  this->Superclass::PlaceWidget(bds);

  this->PrevPickedPoint[0] = 0.5*(bds[0]+bds[1]);
  this->PrevPickedPoint[1] = 0.5*(bds[2]+bds[3]);
  this->PrevPickedPoint[2] = 0.5*(bds[4]+bds[5]);

}


//----------------------------------------------------------------------------
void vtkPickSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RenderModuleProxy: (" << this->RenderModuleProxy << ")\n";
  os << indent << "SetMouseControlToggle" << this->GetMouseControlToggle() << endl;
}


//----------------------------------------------------------------------------
void vtkPickSphereWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling && ! this->Enabled)
    {
    // listen for the following events
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::KeyPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MouseMoveEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    }

  this->Superclass::SetEnabled(enabling);

}

//----------------------------------------------------------------------------
void vtkPickSphereWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                       unsigned long event,
                                       void* clientdata, 
                                       void* vtkNotUsed(calldata))
{
  vtkPickSphereWidget* self = reinterpret_cast<vtkPickSphereWidget *>( clientdata );

//  vtkSphereWidget::ProcessEvents(object, event, clientdata, calldata);

  //look for char and delete events
  switch(event)
    {
    case vtkCommand::CharEvent:
      self->OnChar();
      break;
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown();
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkPickSphereWidget::OnChar()
{
  if(!this->Enabled || !this->CurrentRenderer)
    {
    return;
    }

  if (this->Interactor->GetKeyCode() == 'r' ||
      this->Interactor->GetKeyCode() == 'R' )
    {
    if (this->RenderModuleProxy == NULL)
      {
      vtkErrorMacro("Cannot pick without a render module.");
      return;
      }
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];

    this->PickInternal(X,Y);

    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    this->Interactor->Render();

    return;
    }
}

void vtkPickSphereWidget::PickInternal(int x, int y)
{
  float zbuff = this->RenderModuleProxy->GetZBufferValue(x, y);
  double pt[4];
  this->ComputeDisplayToWorld(double(x),double(y),double(zbuff),pt);

  this->Translate(this->PrevPickedPoint, pt);

  this->PrevPickedPoint[0] = pt[0];
  this->PrevPickedPoint[1] = pt[1];
  this->PrevPickedPoint[2] = pt[2];
}


void vtkPickSphereWidget::OnLeftButtonDown()
{
  if (!this->Interactor)
    {
    return;
    }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y) || (this->CurrentRenderer->IsInViewport(X, Y) && !this->MouseControlToggle))
    {
    this->State = vtkSphereWidget::Outside;
    return;
    }
  
  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then try to pick the sphere.
  vtkAssemblyPath *path;
  this->Picker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->Picker->GetPath();
  if ( path == NULL )
    {
    this->State = vtkSphereWidget::Outside;
    return;
    }
  else if (path->GetFirstNode()->GetViewProp() == this->SphereActor )
    {
    this->State = vtkSphereWidget::Moving;
    this->HighlightSphere(1);
    }
  else if (path->GetFirstNode()->GetViewProp() == this->HandleActor )
    {
    this->State = vtkSphereWidget::Positioning;
    this->HighlightHandle(1);
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}


void vtkPickSphereWidget::OnMouseMove()
{
  // See whether we're active
  if ( this->State == vtkSphereWidget::Outside || 
       this->State == vtkSphereWidget::Start )
    {
    return;
    }
  
  if (!this->Interactor)
    {
    return;
    }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z;

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  camera->GetFocalPoint(focalPoint);
  this->ComputeWorldToDisplay(focalPoint[0], focalPoint[1],
                              focalPoint[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                              double(this->Interactor->GetLastEventPosition()[1]),
                              z, 
                              prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if ( this->State == vtkSphereWidget::Moving )
    {
    this->PickInternal(X,Y);
    // this->Translate(prevPickPoint, pickPoint);
    }
  else if ( this->State == vtkSphereWidget::Scaling )
    {
    this->ScaleSphere(prevPickPoint, pickPoint, X, Y);
    }
  else if ( this->State == vtkSphereWidget::Positioning )
    {
    this->MoveHandle(prevPickPoint, pickPoint, X, Y);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  this->Interactor->Render();
}


void vtkPickSphereWidget::OnRightButtonDown()
{
  if (!this->Interactor)
    {
    return;
    }

  this->State = vtkSphereWidget::Scaling;

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y) || (this->CurrentRenderer->IsInViewport(X, Y) && !this->MouseControlToggle))
    {
    this->State = vtkSphereWidget::Outside;
    return;
    }
  
  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  vtkAssemblyPath *path;
  this->Picker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->Picker->GetPath();
  if ( path == NULL && !this->MouseControlToggle )
    {
    this->State = vtkSphereWidget::Outside;
    this->HighlightSphere(0);
    return;
    }
  else
    {
    this->HighlightSphere(1);
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->Interactor->Render();
}
