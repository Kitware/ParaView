/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLineWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLineWidgetProxy.h"

#include "vtkPickLineWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkKWEvent.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkSMLineWidgetProxy);
vtkCxxRevisionMacro(vtkSMLineWidgetProxy, "1.2");

//----------------------------------------------------------------------------
vtkSMLineWidgetProxy::vtkSMLineWidgetProxy()
{
  this->Resolution = 1;
  this->Point1[0] = -0.5;
  this->Point1[1] = this->Point1[2] = 0;
  this->Point2[0] = 0.5;
  this->Point2[1] = this->Point2[2] = 0;
  this->SetVTKClassName("vtkPickLineWidget");
}

//----------------------------------------------------------------------------
vtkSMLineWidgetProxy::~vtkSMLineWidgetProxy()
{

}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::SetPoint1(double x, double y, double z)
{
  this->Point1[0] = x;
  this->Point1[1] = y;
  this->Point1[2] = z;
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke 
                  <<  id
                  << "SetPoint1" << this->Point1[0] << this->Point1[1] 
                  <<  this->Point1[2]
                  << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                  << id
                  << "SetAlignToNone" <<  vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::SetPoint2(double x, double y, double z)
{
  this->Point2[0] = x;
  this->Point2[1] = y;
  this->Point2[2] = z;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke 
                  <<  id
                  << "SetPoint2" << this->Point2[0] << this->Point2[1] 
                  <<  this->Point2[2]
                  << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                  << id
                  << "SetAlignToNone" <<  vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::SetResolution(int res)
{
  this->Resolution = res;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0;cc <this->GetNumberOfIDs(); cc++)
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->GetID(cc)
                    << "SetResolution" << this->Resolution
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  //superclass will create the stream objects
  this->Superclass::CreateVTKObjects(numObjects);
  //now do additional initialization on the streamobjects
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    pm->GetStream() << vtkClientServerStream::Invoke <<  id
                    << "SetAlignToNone" << vtkClientServerStream::End;
    pm->SendStream(this->GetServers());
    }
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  vtkPickLineWidget* widget = vtkPickLineWidget::SafeDownCast(wdg);
  if (!widget)
    {
    return;
    }
  double val[3];
  widget->GetPoint1(val);
  this->SetPoint1(val[0],val[1],val[2]);
  
  widget->GetPoint2(val);
  this->SetPoint2(val[0],val[1],val[2]);
  this->Superclass::ExecuteEvent(wdg, event, p);
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::SaveInBatchScript(ofstream *file)
{
  this->Superclass::SaveInBatchScript(file);
  for (unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    *file << endl;
    
    *file << "  [$pvTemp" << id.ID << " GetProperty Point1] "
      << "SetElements3 "
      << this->Point1[0] << " "
      << this->Point1[1] << " "
      << this->Point1[2] 
      << endl;

    *file << "  [$pvTemp" << id.ID << " GetProperty Point2] "
      << "SetElements3 "
      << this->Point2[0] << " "
      << this->Point2[1] << " "
      << this->Point2[2] 
      << endl;

    *file << "  [$pvTemp" << id.ID << " GetProperty Resolution] "
      << "SetElements1 "
      << this->Resolution 
      << endl;

    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects" << endl;
    *file << endl;
    }
}
//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Point1: " << this->Point1[0] << "," <<
    this->Point1[1] << "," << this->Point1[2] << endl;
  os << indent << "Point2: " << this->Point2[0] << "," <<
    this->Point2[1] << "," << this->Point2[2] << endl;
  os << indent << "Resolution: " << this->Resolution << endl;
}
