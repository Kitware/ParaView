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
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMLineWidgetProxy);
vtkCxxRevisionMacro(vtkSMLineWidgetProxy, "1.4");

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
void vtkSMLineWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  unsigned int numObjects = this->GetNumberOfIDs();
  for (cc=0; cc < numObjects; cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    str << vtkClientServerStream::Invoke 
        <<  id
        << "SetPoint1" << this->Point1[0] << this->Point1[1] 
        <<  this->Point1[2]
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        <<  id
        << "SetPoint2" << this->Point2[0] << this->Point2[1] 
        <<  this->Point2[2]
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        << this->GetID(cc)
        << "SetResolution" << this->Resolution
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        << id
        << "SetAlignToNone" <<  vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
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
  //Update the iVars to reflect the VTK object state
  widget->GetPoint1(val);
  this->SetPoint1(val);
  widget->GetPoint2(val);
  this->SetPoint2(val);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
  this->Superclass::ExecuteEvent(wdg, event, p);
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::SaveState(const char* name,ostream* file, 
  vtkIndent indent)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Point1"));
  if (dvp)
    {
    dvp->SetElements(this->Point1);
    }
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Point2"));
  if (dvp)
    {
    dvp->SetElements(this->Point2);
    }
  this->Superclass::SaveState(name,file,indent);
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
