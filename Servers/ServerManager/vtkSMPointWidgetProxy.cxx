/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPointWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPointWidgetProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkKWEvent.h"
#include "vtkClientServerStream.h"
#include "vtkPickPointWidget.h"

vtkStandardNewMacro(vtkSMPointWidgetProxy);
vtkCxxRevisionMacro(vtkSMPointWidgetProxy, "1.1");

//----------------------------------------------------------------------------
vtkSMPointWidgetProxy::vtkSMPointWidgetProxy()
{
  this->Position[0] = this->Position[1] = this->Position[2] = 0;
  this->SetVTKClassName("vtkPickPointWidget");
}

//----------------------------------------------------------------------------
vtkSMPointWidgetProxy::~vtkSMPointWidgetProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke << id << "AllOff" 
                  << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::SetPosition(double x, double y, double z)
{ 
  this->Position[0] = x;
  this->Position[1] = y;
  this->Position[2] = z;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0;cc<this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << id
                    << "SetPosition" << x << y << z 
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  vtkPickPointWidget *widget = vtkPickPointWidget::SafeDownCast(wdg);
  if ( !widget )
    {
    vtkErrorMacro( "This is not a point widget" );
    return;
    }
  double val[3];
  widget->GetPosition(val); 
  this->SetPosition(val[0], val[1], val[2]);
  this->Superclass::ExecuteEvent(wdg,event,p);
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::SaveInBatchScript(ofstream *file)
{
  this->Superclass::SaveInBatchScript(file);
  for (unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    *file << endl;
    *file << "  [$pvTemp" << id.ID << " GetProperty Position] "
      << "SetElements3 "
      << this->Position[0] << " "
      << this->Position[1] << " "
      << this->Position[2] 
      << endl;

    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects" << endl;
    *file << endl;
    }
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Position: " << this->Position[0] << ", " << this->Position[1]
    << ", " << this->Position[2] << endl;
}
