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
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMSphereWidgetProxy);
vtkCxxRevisionMacro(vtkSMSphereWidgetProxy, "1.2");

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
void vtkSMSphereWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  unsigned int numObjects = this->GetNumberOfIDs();
  for (cc=0; cc < numObjects; cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    str << vtkClientServerStream::Invoke << id
        << "SetCenter" 
        << this->Center[0]
        << this->Center[1]
        << this->Center[2]
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke << id
        << "SetRadius" << this->Radius
        << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
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
  //Update iVars to reflect the state of the VTK object
  double val[3];
  widget->GetCenter(val); 
  this->SetCenter(val);
    
  double rad = widget->GetRadius();
  this->SetRadius(rad);
  
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
  this->Superclass::ExecuteEvent(wdg, event, p);
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::SaveState(const char* name, ostream* file, 
  vtkIndent indent)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Center"));
  if (dvp)
    {
    dvp->SetElements(this->Center);
    }
    
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Radius"));
  if (dvp)
    {
    dvp->SetElements1(this->Radius);
    }
  this->Superclass::SaveState(name,file,indent);
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
