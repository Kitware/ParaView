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
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMPointWidgetProxy);
vtkCxxRevisionMacro(vtkSMPointWidgetProxy, "1.3");

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
void vtkSMPointWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  for(cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    str << vtkClientServerStream::Invoke 
                    << id
                    << "SetPosition" 
                    << this->Position[0]
                    << this->Position[1]
                    << this->Position[2]
                    << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
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
  //Update ivars to reflect the VTK object values
  this->SetPosition(val[0], val[1], val[2]);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
  this->Superclass::ExecuteEvent(wdg,event,p);
}

//----------------------------------------------------------------------------
void vtkSMPointWidgetProxy::SaveState(const char* name, ostream* file, 
  vtkIndent indent)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position"));
  if (dvp)
    {
    dvp->SetElements(this->Position);
    }
  this->Superclass::SaveState(name,file,indent);
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
