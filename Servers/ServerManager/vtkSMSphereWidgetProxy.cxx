/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSphereWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSphereWidgetProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkKWEvent.h"
#include "vtkSphereWidget.h"
vtkStandardNewMacro(vtkSMSphereWidgetProxy);
vtkCxxRevisionMacro(vtkSMSphereWidgetProxy, "1.1");

//----------------------------------------------------------------------------
vtkSMSphereWidgetProxy::vtkSMSphereWidgetProxy()
{
  this->Radius = 0.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->SetVTKClassName("vtkSphereWidget");
}

//----------------------------------------------------------------------------
vtkSMSphereWidgetProxy::~vtkSMSphereWidgetProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::SetCenter(double x, double y, double z)
{
  this->Center[0] = x;
  this->Center[1] = y;
  this->Center[2] = z;
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke << id
                    << "SetCenter" << x << y << z
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::SetRadius(double radius)
{
  this->Radius = radius;
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke << id
                  << "SetRadius" << this->Radius
                  << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  vtkSphereWidget *widget = vtkSphereWidget::SafeDownCast(wdg);
  if ( !widget )
    {
    return;
    }
  double val[3];
  widget->GetCenter(val); 
  this->SetCenter(val[0], val[1], val[2]);
  double rad = widget->GetRadius();
  this->SetRadius(rad);
  this->Superclass::ExecuteEvent(wdg, event, p);
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::SaveInBatchScript(ofstream *file)
{
  this->Superclass::SaveInBatchScript(file);
  for (unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    *file << "  [$pvTemp" << id.ID << " GetProperty Center] "
      << "SetElements3 "
      << this->Center[0] << " "
      << this->Center[1] << " "
      << this->Center[2] 
      << endl;

    *file << "  [$pvTemp" << id.ID << " GetProperty Radius] "
      << "SetElements1 "
      << this->Radius
      << endl;
    
    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects" << endl;
    *file << endl;
    }
}
//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Center: " << this->Center[0]
        << ", " << this->Center[1] << ", " <<this->Center[2] << endl;
  os << indent << "Radius: " << this->Radius << endl;
}
