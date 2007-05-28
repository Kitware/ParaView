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

#include "vtkLineWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"


vtkStandardNewMacro(vtkSMLineWidgetProxy);
vtkCxxRevisionMacro(vtkSMLineWidgetProxy, "1.15");
//----------------------------------------------------------------------------
vtkSMLineWidgetProxy::vtkSMLineWidgetProxy()
{
  this->Point1[0] = -0.5;
  this->Point1[1] = this->Point1[2] = 0;
  this->Point2[0] = 0.5;
  this->Point2[1] = this->Point2[2] = 0;
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
  vtkClientServerID id = this->GetID();
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
  pm->SendStream(this->ConnectionID, this->Servers,str);
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::CreateVTKObjects()
{
  if(this->ObjectsCreated)
    {
    return;
    }
  
  //superclass will create the stream objects
  this->Superclass::CreateVTKObjects();
  
  //now do additional initialization on the streamobjects
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID id = this->GetID();
  stream << vtkClientServerStream::Invoke <<  id
         << "SetAlignToNone" << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  vtkLineWidget* widget = vtkLineWidget::SafeDownCast(wdg);
  if (!widget)
    {
    return;
    }
  double point1[3];
  double point2[3];
  //Update the iVars to reflect the VTK object state
  widget->GetPoint1(point1);
  widget->GetPoint2(point2);
  if (event != vtkCommand::PlaceWidgetEvent || !this->IgnorePlaceWidgetChanges)
    {
    this->SetPoint1(point1);
    vtkSMDoubleVectorProperty* pt1 = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("Point1"));
    if (pt1)
      {
      pt1->SetElements(point1);
      }
    this->SetPoint2(point2);
    vtkSMDoubleVectorProperty* pt2 = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("Point2"));
    if (pt2)
      {
      pt2->SetElements(point2);
      }
    }
  this->Superclass::ExecuteEvent(wdg, event, p);
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMLineWidgetProxy::SaveState(vtkPVXMLElement* root)
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
  return this->Superclass::SaveState(root);
}

//----------------------------------------------------------------------------
void vtkSMLineWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Point1: " << this->Point1[0] << "," <<
    this->Point1[1] << "," << this->Point1[2] << endl;
  os << indent << "Point2: " << this->Point2[0] << "," <<
    this->Point2[1] << "," << this->Point2[2] << endl;
}
