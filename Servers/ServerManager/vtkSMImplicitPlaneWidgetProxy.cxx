/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImplicitPlaneWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMImplicitPlaneWidgetProxy.h"

#include "vtkImplicitPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkKWEvent.h"
#include "vtkCommand.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMImplicitPlaneWidgetProxy);
vtkCxxRevisionMacro(vtkSMImplicitPlaneWidgetProxy, "1.2");

//----------------------------------------------------------------------------
vtkSMImplicitPlaneWidgetProxy::vtkSMImplicitPlaneWidgetProxy()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->Normal[0] = this->Normal[2] = 0.0;
  this->Normal[1] = 1.0;
  this->DrawPlane = 0;
  this->SetVTKClassName("vtkImplicitPlaneWidget");
}

//----------------------------------------------------------------------------
vtkSMImplicitPlaneWidgetProxy::~vtkSMImplicitPlaneWidgetProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  float opacity = 1.0;
  if (pm->GetNumberOfPartitions() == 1)
    { 
    opacity = .25;
    }
  
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    
    pm->GetStream() << vtkClientServerStream::Invoke << id
                    << "OutlineTranslationOff"
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    pm->GetStream() << vtkClientServerStream::Invoke << id
                    << "GetPlaneProperty"
                    << vtkClientServerStream::End
                    << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult 
                    << "SetOpacity" 
                    << opacity 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << id
                    << "GetSelectedPlaneProperty" 
                    << vtkClientServerStream::End
                    << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult 
                    << "SetOpacity" 
                    << opacity 
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->SetDrawPlane(0);
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::PlaceWidget(double bds[6])
{
  this->Superclass::PlaceWidget(bds);
  this->SetCenter((bds[0]+bds[1])/2,
    (bds[2]+bds[3])/2, (bds[4]+bds[5])/2);
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  vtkImplicitPlaneWidget *widget = vtkImplicitPlaneWidget::SafeDownCast(wdg);
  if ( !widget )
    {
    return;
    }
  double val[3];
  widget->GetOrigin(val); 
  this->SetCenter(val[0], val[1], val[2]);
  widget->GetNormal(val);
  this->SetNormal(val[0], val[1], val[2]);
  if (!widget->GetDrawPlane() && event == vtkCommand::InteractionEvent)
    { 
    this->SetDrawPlane(1);
    }
  this->Superclass::ExecuteEvent(wdg,event,p);
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::SetCenter(double x, double y, double z)
{
  this->Center[0] = x;
  this->Center[1] = y;
  this->Center[2] = z;

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0; cc<this->GetNumberOfIDs();cc++)
    { 
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << id << "SetOrigin" << x << y << z
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::SetNormal(double x, double y, double z)
{
  this->Normal[0] = x;
  this->Normal[1] = y;
  this->Normal[2] = z;

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0; cc<this->GetNumberOfIDs();cc++)
    { 
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << id << "SetNormal" << x << y << z
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    } 
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::SetDrawPlane(int val)
{
  this->DrawPlane = val;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0; cc<this->GetNumberOfIDs();cc++)
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->GetID(cc)<< "SetDrawPlane" << val
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::SaveInBatchScript(ofstream *file)
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

    *file << "  [$pvTemp" << id.ID << " GetProperty Normal] "
      << "SetElements3 "
      << this->Normal[0] << " "
      << this->Normal[1] << " "
      << this->Normal[2] 
      << endl;
    *file << "  [$pvTemp" << id.ID << " GetProperty DrawPlane] "
      << "SetElements1 " << this->DrawPlane 
      << endl;
    
    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects" << endl;
    *file << endl;
    }
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Center: " << this->Center[0] << ", " << this->Center[1] 
    << "," << this->Center[2] << endl;
  os << indent << "Normal: " << this->Normal[0] << ", " << this->Normal[1]
    << "," << this->Normal[2] << endl;
}
