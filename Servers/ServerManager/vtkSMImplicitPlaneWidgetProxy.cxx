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
#include "vtkCommand.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMImplicitPlaneWidgetProxy);
vtkCxxRevisionMacro(vtkSMImplicitPlaneWidgetProxy, "1.7.4.1");

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
  
  vtkClientServerStream stream;
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    
    stream << vtkClientServerStream::Invoke << id
           << "OutlineTranslationOff"
           << vtkClientServerStream::End;
    pm->SendStream(this->GetServers(), stream, 1);
    stream << vtkClientServerStream::Invoke << id
           << "GetPlaneProperty"
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke 
           << vtkClientServerStream::LastResult 
           << "SetOpacity" 
           << opacity 
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << id
           << "GetSelectedPlaneProperty" 
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke 
           << vtkClientServerStream::LastResult 
           << "SetOpacity" 
           << opacity 
                    << vtkClientServerStream::End;
    pm->SendStream(this->GetServers(), stream, 1);
    }
  this->SetDrawPlane(0);
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{

  vtkImplicitPlaneWidget *widget = vtkImplicitPlaneWidget::SafeDownCast(wdg);
  if ( !widget )
    {
    return;
    }
  double center[3],normal[3];
  widget->GetOrigin(center); 
  widget->GetNormal(normal);
  if (event == vtkCommand::PlaceWidgetEvent && !this->IgnorePlaceWidgetChanges)
    {
    //this case is here only since the vtkImplicitPlaneWidget doesn;t behave as 
    //expected. The center is not repositioned properly (to the center of the bounds).
    //Hence, we do that explicitly.
    center[0] = (this->Bounds[0] + this->Bounds[1]) /2;
    center[1] = (this->Bounds[2] + this->Bounds[3]) /2;
    center[2] = (this->Bounds[4] + this->Bounds[5]) /2;
    // we do not accept the normal suggestion, since vtkImplicitPlaneWidget::PlaceWidget
    // totally resets the normal, which is not necessary. Hence, we will ignore it.
    normal[0] = this->Normal[0];
    normal[1] = this->Normal[1];
    normal[2] = this->Normal[2];
    }
  if (event != vtkCommand::PlaceWidgetEvent || !this->IgnorePlaceWidgetChanges)
    {
    //Just set the iVars
    this->SetCenter(center);
    this->SetNormal(normal);
    }
  if (!widget->GetDrawPlane() && event == vtkCommand::InteractionEvent)
    { 
    this->SetDrawPlane(1);
    }
  this->Superclass::ExecuteEvent(wdg,event,p);
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  for(unsigned int cc=0; cc<this->GetNumberOfIDs();cc++)
    { 
    vtkClientServerID id = this->GetID(cc);
    str << vtkClientServerStream::Invoke 
        << id << "SetOrigin" 
        << this->Center[0] 
        << this->Center[1] 
        << this->Center[2]
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        << id << "SetNormal"
        << this->Normal[0]
        << this->Normal[1]
        << this->Normal[2]
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke 
        << id << "SetDrawPlane" 
        << this->DrawPlane
        << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    }
}

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::SaveState(const char* name, ostream* file, vtkIndent indent)
{
  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Center"));
  if (dvp)
    {
    dvp->SetElements(this->Center);
    }
  else
    {
    vtkErrorMacro("Failed to find property 'Center'");
    }
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Normal"));
  if (dvp)
    {
    dvp->SetElements(this->Normal);
    }
  else
    {
    vtkErrorMacro("Failed to find property 'Normal'");
    }

  vtkSMIntVectorProperty * ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("DrawPlane"));
  if (ivp)
    {
    ivp->SetElements1(this->DrawPlane);
    }
  else
    {
    vtkErrorMacro("Failed to find property 'DrawPlane'");
    }
  this->Superclass::SaveState(name,file,indent);
}


//----------------------------------------------------------------------------
/*
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
*/

//----------------------------------------------------------------------------
void vtkSMImplicitPlaneWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Center: " << this->Center[0] << ", " << this->Center[1] 
    << "," << this->Center[2] << endl;
  os << indent << "Normal: " << this->Normal[0] << ", " << this->Normal[1]
    << "," << this->Normal[2] << endl;
  os << indent << "DrawPlane: " << this->DrawPlane << endl;
}
