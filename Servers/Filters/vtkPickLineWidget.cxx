/*=========================================================================

  Program:   ParaView
  Module:    vtkPickLineWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPickLineWidget.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"

#include "vtkPVRenderModule.h"


vtkStandardNewMacro(vtkPickLineWidget);
vtkCxxRevisionMacro(vtkPickLineWidget, "1.1");
vtkCxxSetObjectMacro(vtkPickLineWidget,RenderModule,vtkPVRenderModule);



//----------------------------------------------------------------------------
vtkPickLineWidget::vtkPickLineWidget()
{
  this->EventCallbackCommand->SetCallback(vtkPickLineWidget::ProcessEvents);
  this->RenderModule = 0;
  this->LastPicked = 0;
}

//----------------------------------------------------------------------------
vtkPickLineWidget::~vtkPickLineWidget()
{
  this->SetRenderModule(NULL);
}

//----------------------------------------------------------------------------
void vtkPickLineWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RenderModule: (" << this->RenderModule << ")\n";
}


//----------------------------------------------------------------------------
void vtkPickLineWidget::SetEnabled(int enabling)
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
    }

  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------------
void vtkPickLineWidget::ProcessEvents(vtkObject* object, 
                                       unsigned long event,
                                       void* clientdata, 
                                       void* calldata)
{
  vtkInteractorObserver* self 
    = reinterpret_cast<vtkInteractorObserver *>( clientdata );

  vtkLineWidget::ProcessEvents(object, event, clientdata, calldata);

  //look for char and delete events
  switch(event)
    {
    case vtkCommand::CharEvent:
      self->OnChar();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkPickLineWidget::OnChar()
{
  if (this->Interactor->GetKeyCode() == 'p' || 
      this->Interactor->GetKeyCode() == 'P' )
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

    if (this->LastPicked == 0)
      { // Choose the closest point.
      double *pt1 = this->LineSource->GetPoint1();
      double *pt2 = this->LineSource->GetPoint2();
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
      
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    }
}



