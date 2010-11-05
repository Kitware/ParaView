/*=========================================================================

  Program:   ParaView
  Module:    vtkPMUniformGridVolumeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMUniformGridVolumeRepresentationProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPMUniformGridVolumeRepresentationProxy);
//----------------------------------------------------------------------------
vtkPMUniformGridVolumeRepresentationProxy::vtkPMUniformGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkPMUniformGridVolumeRepresentationProxy::~vtkPMUniformGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkPMUniformGridVolumeRepresentationProxy::CreateVTKObjects(
  vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }
  if (!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetVTKObjectID()
         << "AddVolumeMapper"
         << "Fixed point"
         << this->GetSubProxyHelper(
           "VolumeFixedPointRayCastMapper")->GetVTKObjectID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetVTKObjectID()
         << "AddVolumeMapper"
         << "GPU"
         << this->GetSubProxyHelper("VolumeGPURayCastMapper")->GetVTKObjectID()
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  return true;
}

//----------------------------------------------------------------------------
void vtkPMUniformGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
