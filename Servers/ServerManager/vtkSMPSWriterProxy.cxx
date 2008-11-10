/*=========================================================================

Program:   ParaView
Module:    vtkSMPSWriterProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPSWriterProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMPSWriterProxy);
vtkCxxRevisionMacro(vtkSMPSWriterProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMPSWriterProxy::vtkSMPSWriterProxy()
{
  this->FileNameMethod = 0;
}

//-----------------------------------------------------------------------------
vtkSMPSWriterProxy::~vtkSMPSWriterProxy()
{
  this->SetFileNameMethod(0);
}

//-----------------------------------------------------------------------------
void vtkSMPSWriterProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkSMSourceProxy* writer = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Writer"));
  if (!writer)
    {
    vtkErrorMacro("Missing subproxy: Writer");
    return;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << this->GetID() << "SetWriter" << writer->GetID() 
         << vtkClientServerStream::End;
  if (this->GetFileNameMethod())
    {
    stream << vtkClientServerStream::Invoke
           << this->GetID() 
           << "SetFileNameMethod" 
           << this->GetFileNameMethod()
           << vtkClientServerStream::End;
    }

  if (this->GetSubProxy("PreGatherHelper"))
    {
    stream << vtkClientServerStream::Invoke
           << this->GetID()
           << "SetPreGatherHelper"
           << this->GetSubProxy("PreGatherHelper")->GetID()
           << vtkClientServerStream::End;
    }

  if (this->GetSubProxy("PostGatherHelper"))
    {
    stream << vtkClientServerStream::Invoke
           << this->GetID()
           << "SetPostGatherHelper"
           << this->GetSubProxy("PostGatherHelper")->GetID()
           << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//---------------------------------------------------------------------------
int vtkSMPSWriterProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  const char* setFileNameMethod = element->GetAttribute("file_name_method");
  if(setFileNameMethod)
    {
    this->SetFileNameMethod(setFileNameMethod);
    }

  return this->Superclass::ReadXMLAttributes(pm, element);
}

//-----------------------------------------------------------------------------
void vtkSMPSWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
