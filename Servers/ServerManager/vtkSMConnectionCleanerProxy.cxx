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
vtkCxxRevisionMacro(vtkSMConnectionCleanerProxy, "1.1");
//-----------------------------------------------------------------------------
vtkSMConnectionCleanerProxy::vtkSMConnectionCleanerProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMConnectionCleanerProxy::~vtkSMConnectionCleanerProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMConnectionCleanerProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);
  if (!this->ObjectsCreated || this->GetNumberOfIDs() == 0)
    {
    return;
    }
 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  stream << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID()
    << "GetConnectionID" 
    << pm->GetConnectionClientServerID(this->GetConnectionID())
    << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke
    << this->GetID(0) << "SetConnectionID"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(), this->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMConnectionCleanerProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
