/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImageTextureProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMImageTextureProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
vtkStandardNewMacro(vtkSMImageTextureProxy);
vtkCxxRevisionMacro(vtkSMImageTextureProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMImageTextureProxy::vtkSMImageTextureProxy()
{
  this->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
vtkSMImageTextureProxy::~vtkSMImageTextureProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMImageTextureProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  vtkSMProxy* reader = this->GetSubProxy("Source");
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << reader->GetID()
          << "GetOutputPort"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->GetID()
          << "SetInputConnection"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
    this->Servers, stream);
}

//----------------------------------------------------------------------------
void vtkSMImageTextureProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


