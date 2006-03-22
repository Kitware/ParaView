/*=========================================================================

  Program:   ParaView
  Module:    vtkPickBoxWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPickBoxWidget.h"
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


vtkStandardNewMacro(vtkPickBoxWidget);
vtkCxxRevisionMacro(vtkPickBoxWidget, "1.2");



//----------------------------------------------------------------------------
vtkPickBoxWidget::vtkPickBoxWidget()
{
  this->EventCallbackCommand->SetCallback(vtkPickBoxWidget::ProcessEvents);
  this->RenderModuleProxy = 0;
  this->MouseControlToggle = 0;
// ATTRIBUTE EDITOR
  this->LastPickPosition[0] = this->LastPickPosition[1] = this->LastPickPosition[2] = 0;

}

//----------------------------------------------------------------------------
vtkPickBoxWidget::~vtkPickBoxWidget()
{
  this->SetRenderModuleProxy(NULL);

}

void vtkPickBoxWidget::PlaceWidget(double bds[6])
{
  this->Superclass::PlaceWidget(bds);

  this->PrevPickedPoint[0] = 0.5*(bds[0]+bds[1]);
  this->PrevPickedPoint[1] = 0.5*(bds[2]+bds[3]);
  this->PrevPickedPoint[2] = 0.5*(bds[4]+bds[5]);

}

//----------------------------------------------------------------------------
void vtkPickBoxWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RenderModuleProxy: (" << this->RenderModuleProxy << ")\n";
  os << indent << "SetMouseControlToggle" << this->GetMouseControlToggle() << endl;
}


//----------------------------------------------------------------------------
void vtkPickBoxWidget::SetEnabled(int enabling)
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
    i->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand, 
                   this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    }

  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------------
void vtkPickBoxWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                       unsigned long event,
                                       void* clientdata, 
                                       void* vtkNotUsed(calldata))
{
  vtkPickBoxWidget* self 
    = reinterpret_cast<vtkPickBoxWidget *>( clientdata );

//  this->Superclass::ProcessEvents(object, event, clientdata, calldata);

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
    case vtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown();
      break;
    case vtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp();
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
void vtkPickBoxWidget::OnChar()
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

void vtkPickBoxWidget::PickInternal(int x, int y)
{
  float zbuff = this->RenderModuleProxy->GetZBufferValue(x, y);
  double pt[4];
  this->ComputeDisplayToWorld(double(x),double(y),double(zbuff),pt);

  this->Translate(this->PrevPickedPoint, pt);

  this->PrevPickedPoint[0] = pt[0];
  this->PrevPickedPoint[1] = pt[1];
  this->PrevPickedPoint[2] = pt[2];
}

void vtkPickBoxWidget::OnLeftButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y) || (this->CurrentRenderer->IsInViewport(X, Y) && !this->MouseControlToggle))
    {
    this->State = vtkBoxWidget::Outside;
    return;
    }
  
  vtkAssemblyPath *path;
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path != NULL )
    {
    this->State = vtkBoxWidget::Moving;
    this->HighlightFace(
      this->HighlightHandle(path->GetFirstNode()->GetViewProp()));
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
    this->ValidPick = 1;
    }
  else
    {
    this->HexPicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->HexPicker->GetPath();
    if ( path != NULL )
      {
      this->State = vtkBoxWidget::Moving;
      this->HexPicker->GetPickPosition(this->LastPickPosition);
      this->ValidPick = 1;
      if ( !this->Interactor->GetShiftKey() )
        {
        this->HighlightHandle(NULL);
        this->HighlightFace(this->HexPicker->GetCellId());
        }
      else
        {
        this->CurrentHandle = this->Handle[6];
        this->HighlightOutline(1);
        }
      }
    else
      {
      //this->HighlightFace(this->HighlightHandle(NULL));
// ATTRIBUTE EDITOR
      if(this->MouseControlToggle)
        {
        this->State = vtkBoxWidget::Moving;
        }
      else
        {
        this->State = vtkBoxWidget::Outside;
        return;
        }
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
  this->Interactor->Render();
}

void vtkPickBoxWidget::OnRightButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y) || (this->CurrentRenderer->IsInViewport(X, Y) && !this->MouseControlToggle))
    {
    this->State = vtkBoxWidget::Outside;
    return;
    }
  
  vtkAssemblyPath *path;
  this->HandlePicker->Pick(X,Y,0.0,this->CurrentRenderer);
  path = this->HandlePicker->GetPath();
  if ( path != NULL )
    {
    this->State = vtkBoxWidget::Scaling;
    this->HighlightOutline(1);
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
    this->ValidPick = 1;
    }
  else
    {
    this->HexPicker->Pick(X,Y,0.0,this->CurrentRenderer);
    path = this->HexPicker->GetPath();
    if ( path != NULL )
      {
      this->State = vtkBoxWidget::Scaling;
      this->HighlightOutline(1);
      this->HexPicker->GetPickPosition(this->LastPickPosition);
      this->ValidPick = 1;
      }
    else
      {
      if(this->MouseControlToggle)
        {
        this->State = vtkBoxWidget::Scaling;
        this->CurrentHandle = NULL;
        }
      else
        {
        this->State = vtkBoxWidget::Outside;
        return;
        }
      }
    }
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
  this->Interactor->Render();
}


void vtkPickBoxWidget::OnMouseMove()
{
  // See whether we're active
  if ( (this->State == vtkBoxWidget::Outside ) || 
       this->State == vtkBoxWidget::Start )
    {
    return;
    }
  
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  this->ComputeWorldToDisplay(this->LastPickPosition[0], this->LastPickPosition[1],
                              this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
                              double(this->Interactor->GetLastEventPosition()[1]),
                              z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);


  // Process the motion
  if ( this->State == vtkBoxWidget::Moving )
    {
    // Okay to process
    if ( this->CurrentHandle )
      {
      if ( this->RotationEnabled && this->CurrentHandle == this->HexFace )
        {
        camera->GetViewPlaneNormal(vpn);
        this->Rotate(X, Y, prevPickPoint, pickPoint, vpn);
        }
      else if ( this->TranslationEnabled && this->CurrentHandle == this->Handle[6] )
        {
        this->Translate(prevPickPoint, pickPoint);

        this->PrevPickedPoint[0] = pickPoint[0];
        this->PrevPickedPoint[1] = pickPoint[1];
        this->PrevPickedPoint[2] = pickPoint[2];
        }
      else if ( this->TranslationEnabled && this->ScalingEnabled ) 
        {
        if ( this->CurrentHandle == this->Handle[0] )
          {
          this->MoveMinusXFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[1] )
          {
          this->MovePlusXFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[2] )
          {
          this->MoveMinusYFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[3] )
          {
          this->MovePlusYFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[4] )
          {
          this->MoveMinusZFace(prevPickPoint, pickPoint);
          }
        else if ( this->CurrentHandle == this->Handle[5] )
          {
          this->MovePlusZFace(prevPickPoint, pickPoint);
          }
        }
      }
    else if( this->RotationEnabled && this->MouseControlToggle)
      {
      camera->GetViewPlaneNormal(vpn);
      this->Rotate(X, Y, prevPickPoint, pickPoint, vpn);
      }
    }
  else if ( this->ScalingEnabled && this->State == vtkBoxWidget::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, X, Y);
    }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
  this->Interactor->Render();
}
