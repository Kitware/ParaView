/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPickLineWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPickLineWidgetProxy.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkProcessModule.h"
#include "vtkLineWidget.h"

vtkStandardNewMacro(vtkSMPickLineWidgetProxy);
vtkCxxRevisionMacro(vtkSMPickLineWidgetProxy, "1.1.2.1");
//-----------------------------------------------------------------------------
vtkSMPickLineWidgetProxy::vtkSMPickLineWidgetProxy()
{
  this->EventTag = 0;
  this->Interactor = 0;
  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this);
  this->EventCallbackCommand->SetCallback(vtkSMPickLineWidgetProxy::ProcessEvents);
  this->LastPicked = 0;
   
}

//-----------------------------------------------------------------------------
vtkSMPickLineWidgetProxy::~vtkSMPickLineWidgetProxy()
{
  this->EventCallbackCommand->Delete();
}

//-----------------------------------------------------------------------------
/*static*/
void vtkSMPickLineWidgetProxy::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                          unsigned long event,
                                          void* clientdata, 
                                          void* vtkNotUsed(calldata))
{
  cout << "vtkSMPickLineWidgetProxy::ProcessEvents" << endl;
  vtkSMPickLineWidgetProxy* self = reinterpret_cast<vtkSMPickLineWidgetProxy*>(
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
void vtkSMPickLineWidgetProxy::OnChar()
{
  if (!this->ObjectsCreated || this->GetNumberOfIDs() < (unsigned int)1)
    {
    vtkErrorMacro("LineWidgetProxy not created yet.");
    return;
    }

  vtkRenderer* ren = this->CurrentRenderModuleProxy->GetRenderer();
  
  if (this->Interactor->GetKeyCode() == 'p' || 
    this->Interactor->GetKeyCode() == 'P' )
    {
    if (this->CurrentRenderModuleProxy == NULL)
      {
      vtkErrorMacro("Cannot pick without a render module.");
      return;
      }
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    float z = this->CurrentRenderModuleProxy->GetZBufferValue(X, Y);
    double pt[4];

    // ComputeDisplayToWorld
    ren->SetDisplayPoint(double(X), double(Y), z);
    ren->DisplayToWorld();
    ren->GetWorldPoint(pt);

    if (this->LastPicked == 0)
      { // Choose the closest point.
      const double *pt1 = this->GetPoint1();
      const double *pt2 = this->GetPoint2();
      double d1, d2, tmp[3];
      tmp[0] = pt1[0]-pt[0]; 
      tmp[1] = pt1[1]-pt[1]; 
      tmp[2] = pt1[2]-pt[2];
      d1 = tmp[0]*tmp[0] + tmp[1]*tmp[1] + tmp[2]*tmp[2];
      tmp[0] = pt2[0]-pt[0]; 
      tmp[1] = pt2[1]-pt[1]; 
      tmp[2] = pt2[2]-pt[2];
      d2 = tmp[0]*tmp[0] + tmp[1]*tmp[1] + tmp[2]*tmp[2];
      this->LastPicked = 1;
      if (d2 < d1)
        {
        this->LastPicked = 2;
        }
      }
    else
      { // toggle point
      if (this->LastPicked == 1)
        {
        this->LastPicked = 2;
        }
      else
        {
        this->LastPicked = 1;
        }
      }

    if (this->LastPicked == 1)
      {
      this->SetPoint1(pt[0], pt[1], pt[2]);
      }
    else
      {
      this->SetPoint2(pt[0], pt[1], pt[2]);
      }
    this->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMPickLineWidgetProxy::SetInteractor(vtkSMProxy* irenProxy)
{
  /*
  if (this->Interactor && this->EventTag)
    {
    this->Interactor->RemoveObserver(this->EventTag);
    this->EventTag = 0;
    }
  this->Interactor = 0;
  if (!irenProxy || irenProxy->GetNumberOfIDs() < 1)
    {
    return;
    }
 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  this->Interactor = vtkRenderWindowInteractor::SafeDownCast(
    pm->GetObjectFromID(irenProxy->GetID(0)));
  
  if (this->Interactor)
    {
    vtkLineWidget* wdg = vtkLineWidget::SafeDownCast(
      pm->GetObjectFromID(this->GetID(0)));
    
    this->EventTag = this->Interactor->AddObserver(vtkCommand::KeyPressEvent,
      this->EventCallbackCommand, wdg->GetPriority()); // TODO: Need to obtain the prority from the client widget
      // and add it accordingly.
    }
    */
  this->Superclass::SetInteractor(irenProxy);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMPickLineWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
