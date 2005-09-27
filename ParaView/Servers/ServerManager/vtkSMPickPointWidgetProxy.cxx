/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPickPointWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPickPointWidgetProxy.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkProcessModule.h"
#include "vtkPointWidget.h"


vtkStandardNewMacro(vtkSMPickPointWidgetProxy);
vtkCxxRevisionMacro(vtkSMPickPointWidgetProxy, "1.3");

//-----------------------------------------------------------------------------
vtkSMPickPointWidgetProxy::vtkSMPickPointWidgetProxy()
{
  this->EventTag = 0;
  this->Interactor = 0;
  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this);
  this->EventCallbackCommand->SetCallback(vtkSMPickPointWidgetProxy::ProcessEvents);
}

//-----------------------------------------------------------------------------
vtkSMPickPointWidgetProxy::~vtkSMPickPointWidgetProxy()
{
  this->EventCallbackCommand->Delete();
}

//-----------------------------------------------------------------------------
/*static*/
void vtkSMPickPointWidgetProxy::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                          unsigned long event,
                                          void* clientdata, 
                                          void* vtkNotUsed(calldata))
{
  vtkSMPickPointWidgetProxy* self = reinterpret_cast<vtkSMPickPointWidgetProxy*>(
    clientdata);
  if (!self)
    {
    vtkGenericWarningMacro("ProcessEvents received from unknown object.");
    return;
    }

  switch (event) 
    {
  case vtkCommand::CharEvent:
    self->OnChar();
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkSMPickPointWidgetProxy::OnChar()
{
  if (!this->ObjectsCreated || this->GetNumberOfIDs() < (unsigned int)1)
    {
    vtkErrorMacro("LineWidgetProxy not created yet.");
    return;
    }

  vtkRenderer* ren = this->CurrentRenderModuleProxy->GetRenderer();

  if (ren && this->Interactor->GetKeyCode() == 'p' ||
    this->Interactor->GetKeyCode() == 'P' )
    {
    if (this->CurrentRenderModuleProxy == NULL)
      {
      vtkErrorMacro("Cannot pick without a render module.");
      return;
      }
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    float Z = this->CurrentRenderModuleProxy->GetZBufferValue(X, Y);

    if (Z == 1.0) 
      {      

      //missed, search around in image space until we hit something
      int Xnew = X;
      int Ynew = Y;
      float Znew = Z;
      bool missed = true;
      bool OOBLeft = false;
      bool OOBRight = false;
      bool OOBDown = false;
      bool OOBUp = false;
      int winSize[2];
      int keepsearching =
        this->CurrentRenderModuleProxy->GetServerRenderWindowSize(winSize);
      int incr = 0;
      while (missed && keepsearching)
        {
        incr++;

        if (incr <= X) 
          {          
          Znew = this->CurrentRenderModuleProxy->GetZBufferValue(X-incr, Y);
          if (Znew < Z) 
            {
            Xnew = X-incr;
            Ynew = Y;
            Z = Znew;          
            missed = false;
            }
          }
        else 
          OOBLeft = true;

        if (X+incr < winSize[0]) 
          {
          Znew = this->CurrentRenderModuleProxy->GetZBufferValue(X+incr, Y);
          if (Znew < Z) 
            {
            Xnew = X+incr;
            Ynew = Y;
            Z = Znew; 
            missed = false;
            }
          }
        else
          OOBRight = true;

        if (incr <= Y) 
          {
          Znew = this->CurrentRenderModuleProxy->GetZBufferValue(X, Y-incr);
          if (Znew < Z) 
            {
            Xnew = X;
            Ynew = Y-incr;
            Z = Znew;
            missed = false;
            }
          }
        else
          OOBDown = true;

        if (Y+incr < winSize[1]) 
          {
          Znew = this->CurrentRenderModuleProxy->GetZBufferValue(X, Y+incr);
          if (Znew < Z) 
            {
            Xnew = X; 
            Ynew = Y+incr;
            Z = Znew;
            missed = false;
            }
          }
        else
          OOBUp = true;

        if (OOBLeft && OOBRight && OOBDown && OOBUp) keepsearching = 0;
        }
      X = Xnew;
      Y = Ynew;
      }
    
    double pt[4];
    
    // ComputeDisplayToWorld
    ren->SetDisplayPoint(double(X), double(Y), Z);
    ren->DisplayToWorld();
    ren->GetWorldPoint(pt);

    this->SetPosition(pt);
    this->UpdateVTKObjects(); // This will push down the values on to the
      // server objects (and client objects).
    this->InvokeEvent(vtkCommand::WidgetModifiedEvent); //So that the GUI
      // knows that the widget has been modified.
    this->Interactor->Render();
    }
}
//-----------------------------------------------------------------------------
void vtkSMPickPointWidgetProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->Superclass::AddToRenderModule(rm);
  if (this->Interactor || !this->ObjectsCreated || this->GetNumberOfIDs() < 1)
    {
    // already added to a render module.
    return;
    }
  this->Interactor = rm->GetInteractor();
  
  if (this->Interactor)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPointWidget* wdg = vtkPointWidget::SafeDownCast(
      pm->GetObjectFromID(this->GetID(0)));
    
    this->EventTag = this->Interactor->AddObserver(vtkCommand::CharEvent,
      this->EventCallbackCommand, wdg->GetPriority()); 
    }
}


//-----------------------------------------------------------------------------
void vtkSMPickPointWidgetProxy::RemoveFromRenderModule(
  vtkSMRenderModuleProxy* rm)
{
  this->Superclass::RemoveFromRenderModule(rm);

  if (this->Interactor && this->EventTag)
    {
    this->Interactor->RemoveObserver(this->EventTag);
    this->EventTag = 0;
    }
  this->Interactor = 0;
}

//-----------------------------------------------------------------------------
void vtkSMPickPointWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
