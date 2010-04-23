/*=========================================================================

  Program:   ParaView
  Module:    vtkSMConnectionCleanerProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMConnectionCleanerProxy.h"


#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMConnectionCleanerProxy);
//-----------------------------------------------------------------------------
vtkSMConnectionCleanerProxy::vtkSMConnectionCleanerProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMConnectionCleanerProxy::~vtkSMConnectionCleanerProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMConnectionCleanerProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated || this->GetID().IsNull())
    {
    return;
    }
 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID()
         << "GetActiveRemoteConnection"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID()
    << "GetConnectionID" 
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke
    << this->GetID() << "SetConnectionID"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(), this->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMConnectionCleanerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
