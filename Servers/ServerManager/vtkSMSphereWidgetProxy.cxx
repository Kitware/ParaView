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

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSphereWidget.h"

vtkStandardNewMacro(vtkSMSphereWidgetProxy);
vtkCxxRevisionMacro(vtkSMSphereWidgetProxy, "1.12");

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
    pm->SendStream(this->ConnectionID ,this->Servers,str);
    }
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
  double rad = widget->GetRadius();
  widget->GetCenter(val); 
  
  if (event != vtkCommand::PlaceWidgetEvent || !this->IgnorePlaceWidgetChanges)
    {
    this->SetCenter(val);
    this->SetRadius(rad);
    }
  this->Superclass::ExecuteEvent(wdg, event, p);
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSphereWidgetProxy::SaveState(vtkPVXMLElement* root)
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
  return this->Superclass::SaveState(root);
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

  *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty Center] "
        << "SetElements3 "
        << this->Center[0] << " "
        << this->Center[1] << " "
        << this->Center[2] 
        << endl;
  
  *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty Radius] "
        << "SetElements1 "
        << this->Radius
        << endl;
  
  *file << "  $pvTemp" << this->GetSelfIDAsString() << " UpdateVTKObjects" 
        << endl;
  *file << endl;
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Center: " << this->Center[0]
        << ", " << this->Center[1] << ", " <<this->Center[2] << endl;
  os << indent << "Radius: " << this->Radius << endl;
}
