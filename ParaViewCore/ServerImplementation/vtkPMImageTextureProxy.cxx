/*=========================================================================

  Program:   ParaView
  Module:    vtkPMImageTextureProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMImageTextureProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPMImageTextureProxy);
//----------------------------------------------------------------------------
vtkPMImageTextureProxy::vtkPMImageTextureProxy()
{
}

//----------------------------------------------------------------------------
vtkPMImageTextureProxy::~vtkPMImageTextureProxy()
{
}

//----------------------------------------------------------------------------
bool vtkPMImageTextureProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }

  if (!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  // Do the binding between the SubProxy source to the local input
  vtkPMProxy* reader = this->GetSubProxyHelper("Source");

  if(reader)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << reader->GetVTKObject()
           << "GetOutputPort"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << "SetInputConnection"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;

    if (!this->Interpreter->ProcessStream(stream))
      {
      return false;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPMImageTextureProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
