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
vtkStandardNewMacro(vtkSMScalarBarActorProxy);
vtkCxxRevisionMacro(vtkSMScalarBarActorProxy, "1.1.2.1");
//-----------------------------------------------------------------------------
vtkSMScalarBarActorProxy::vtkSMScalarBarActorProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMScalarBarActorProxy::~vtkSMScalarBarActorProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects(numObjects);

  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(this->GetProperty("TitleTextProperty"));
  pp->AddProxy(this->GetSubProxy("TitleTextProperty"));

  pp = vtkSMProxyProperty::SafeDownCast(this->GetProperty("LabelTextProperty"));
  pp->AddProxy(this->GetSubProxy("LabelTextProperty"));

  this->UpdateVTKObjects();
}


//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::SetPosition(double x, double y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->GetNumberOfIDs(); i++)
    {
    vtkClientServerID id = this->GetID(i);
    stream << vtkClientServerStream::Invoke
      << id 
      << "GetPositionCoordinate"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetValue" << x  << y
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::SetPosition2(double x, double y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  for (unsigned int i=0; i < this->GetNumberOfIDs(); i++)
    {
    vtkClientServerID id = this->GetID(i);
    stream << vtkClientServerStream::Invoke
      << id 
      << "GetPosition2Coordinate"
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult
      << "SetValue" << x  << y
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarActorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
