/*=========================================================================

  Program:   ParaView
  Module:    vtkRMPointWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRMPointWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkKWEvent.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkRMPointWidget);
vtkCxxRevisionMacro(vtkRMPointWidget, "1.1");

//----------------------------------------------------------------------------
vtkRMPointWidget::vtkRMPointWidget()
{
  this->LastAcceptedPosition[0] = this->LastAcceptedPosition[1] =
    this->LastAcceptedPosition[2] = 0;
  this->Position[0] = this->Position[1] = this->Position[2] = 0;
}

//----------------------------------------------------------------------------
vtkRMPointWidget::~vtkRMPointWidget()
{
}

void vtkRMPointWidget::Create(vtkPVProcessModule *pm, vtkClientServerID rendererID, vtkClientServerID interactorID)
{
  this->Widget3DID = pm->NewStreamObject("vtkPickPointWidget");
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID << "AllOff" 
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->Superclass::Create(pm,rendererID,interactorID);
}

void vtkRMPointWidget::GetPosition(double pt[3])
{
  if (pt == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  pt[0] = this->Position[0];
  pt[1] = this->Position[1];
  pt[2] = this->Position[2]; 
}

//----------------------------------------------------------------------------
void vtkRMPointWidget::SetPosition(double x, double y, double z)
{ 
  this->Position[0] = x;
  this->Position[1] = y;
  this->Position[2] = z;
  if ( this->Widget3DID.ID )
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->Widget3DID 
                    << "SetPosition" << x << y << z << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMPointWidget::UpdateVTKObject(vtkClientServerID ObjectID, char *VariableName)  
{
  // Accept point
  char acceptCmd[1024];
  if ( VariableName && ObjectID.ID )
    {    
    sprintf(acceptCmd, "Set%s", VariableName);
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke << ObjectID
                    << acceptCmd 
                    << this->Position[0]
                    << this->Position[1]
                    << this->Position[2]
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::DATA_SERVER);
    }
  
  this->SetLastAcceptedPosition(this->Position[0],
                                this->Position[1],
                                this->Position[2]);
}
//----------------------------------------------------------------------------
void vtkRMPointWidget::ResetInternal()
{
  this->SetPosition(this->LastAcceptedPosition[0],
                    this->LastAcceptedPosition[1],
                    this->LastAcceptedPosition[2]);
}

void vtkRMPointWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Last Accepted Position: " << this->LastAcceptedPosition[0]
    << ", " << this->LastAcceptedPosition[1] << ", " 
    << this->LastAcceptedPosition[2] << endl;
  os << indent << "Position: " << this->Position[0] << ", " << this->Position[1]
    << ", " << this->Position[2] << endl;
}
