/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUniformGridVolumeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUniformGridVolumeRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMUniformGridVolumeRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMUniformGridVolumeRepresentationProxy::vtkSMUniformGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMUniformGridVolumeRepresentationProxy::~vtkSMUniformGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::CreateVTKObjects()
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

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "AddVolumeMapper"
         << "Fixed point"
         << this->GetSubProxy("VolumeFixedPointRayCastMapper")->GetID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "AddVolumeMapper"
         << "GPU"
         << this->GetSubProxy("VolumeGPURayCastMapper")->GetID()
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->Servers, stream);
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

