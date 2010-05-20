/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarActorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMScalarBarActorProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyIterator.h"

vtkStandardNewMacro(vtkSMScalarBarActorProxy);
//-----------------------------------------------------------------------------
vtkSMScalarBarActorProxy::vtkSMScalarBarActorProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMScalarBarActorProxy::~vtkSMScalarBarActorProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  vtkSMProxy* labelTextProperty = this->GetSubProxy("LabelTextProperty");
  vtkSMProxy* titleTextProperty = this->GetSubProxy("TitleTextProperty");
  
 
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "SetLabelTextProperty" << labelTextProperty->GetID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "SetTitleTextProperty" << titleTextProperty->GetID()
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->GetServers(), stream);

  /*
  // Let's set bold/shadow/italic on the text properties.
  vtkSMIntVectorProperty* ivp;
  vtkSMDoubleVectorProperty* dvp;

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("LabelBold"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("LabelShadow"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("LabelItalic"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("TitleBold"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("TitleShadow"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("TitleItalic"));
  ivp->SetElement(0, 1);

  // We set the position only if the properties
  // are not already modified.
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position"));
  //dvp->SetElements2(0.87, 0.25);
  dvp->Modified();

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position2"));
  //dvp->SetElements2(0.13, 0.5);
  dvp->Modified();
  this->UpdateVTKObjects();
  */
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::SetPosition(double x, double y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID id = this->GetID();
  stream << vtkClientServerStream::Invoke
         << id 
         << "GetPositionCoordinate"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult
         << "SetValue" << x  << y
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::SetPosition2(double x, double y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID id = this->GetID();
  stream << vtkClientServerStream::Invoke
         << id 
         << "GetPosition2Coordinate"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult
         << "SetValue" << x  << y
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
