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
#include "vtkProcessModule.h"
#include "vtkCommand.h"
#include "vtkClientServerStream.h"
#include "vtkPointWidget.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMPointWidgetProxy);
vtkCxxRevisionMacro(vtkSMPointWidgetProxy, "1.14");

//----------------------------------------------------------------------------
vtkSMPointWidgetProxy::vtkSMPointWidgetProxy()
{
  this->Position[0] = this->Position[1] = this->Position[2] = 0;
}

//----------------------------------------------------------------------------
vtkSMPointWidgetProxy::~vtkSMPointWidgetProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::CreateVTKObjects()
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(); 
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID id = this->GetID();
  stream << vtkClientServerStream::Invoke 
         << id << "AllOff" 
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  vtkClientServerID id = this->GetID();
  str << vtkClientServerStream::Invoke 
      << id
      << "SetPosition" 
      << this->Position[0]
      << this->Position[1]
      << this->Position[2]
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers,str);
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  vtkPointWidget *widget = vtkPointWidget::SafeDownCast(wdg);
  if ( !widget )
    {
    vtkErrorMacro( "This is not a point widget" );
    return;
    }
  double val[3];
  widget->GetPosition(val); 
  if (event != vtkCommand::PlaceWidgetEvent || !this->IgnorePlaceWidgetChanges)
    {
    //Update ivars to reflect the VTK object values
    this->SetPosition(val);
    vtkSMDoubleVectorProperty* pos = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("Position"));
    if (pos)
      {
      pos->SetElements(val);
      }
    }
  this->Superclass::ExecuteEvent(wdg,event,p);
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMPointWidgetProxy::SaveState(vtkPVXMLElement* root)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position"));
  if (dvp)
    {
    dvp->SetElements(this->Position);
    }
  return this->Superclass::SaveState(root);
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Position: " << this->Position[0] << ", " << this->Position[1]
    << ", " << this->Position[2] << endl;
}
